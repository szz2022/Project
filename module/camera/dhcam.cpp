#include "dhcam.h"
#include "../operation/operation.h"
DhCamera::DhCamera(int device_name)
{
    //时间单位是ns
    this->lost_frame_time_thersold = LOST_FRAME_TIME_THERSOLD;
    this->lost_frame_count_thersold = LOST_FRAME_COUNT_THERSOLD;
    init(device_name,5000);
}

DhCamera::~DhCamera()
{
    std::cout << "release" << endl;
    release();
}

void DhCamera::get_error_string(GX_STATUS emErrorStatus)
{
    char *error_info = NULL;
    size_t size = 0;
    GX_STATUS emStatus = GX_STATUS_SUCCESS;
    std::string msg = "";

    // Get length of error description
    emStatus = GXGetLastError(&emErrorStatus, NULL, &size);
    if (emStatus != GX_STATUS_SUCCESS)
    {
        printf("<Error when calling GXGetLastError>\n");
        msg = format_multi_args("Error when calling GXGetLastError");
        logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_ALARM);
        return;
    }

    // Alloc error resources
    error_info = new char[size];
    if (error_info == NULL)
    {
        printf("<Failed to allocate memory>\n");
        msg = format_multi_args("Failed to allocate memory");
        logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_ERROR);
        return;
    }

    // Get error description
    emStatus = GXGetLastError(&emErrorStatus, error_info, &size);
    if (emStatus != GX_STATUS_SUCCESS)
    {
        printf("<Error when calling GXGetLastError>\n");
        msg = format_multi_args("Error when calling GXGetLastError");
        logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_ERROR);
    }
    else
    {
        printf("%s\n", error_info);
        msg = format_multi_args(error_info);
        logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_ERROR);
    }

    // Realease error resources
    if (error_info != NULL)
    {
        delete[] error_info;
        error_info = NULL;
    }
}

void DhCamera::OnDeviceOfflineCallbackEx(void* pUserParam){
    // 收到掉线通知后，写入告警日志
    std::cout << "掉线" << endl;
    LOG_PLUS("大恒相机 "+to_string(((DhCamera*)pUserParam)->cur_device_name)+" 已掉线!", LOGGER_ALARM);
    return;
}

void DhCamera::ImageCallBackEx(GX_FRAME_CALLBACK_PARAM *pFrame)
{
    DhCamera *dh = (DhCamera*) pFrame->pUserParam;
    string msg = "";
    
    if (pFrame->status == GX_FRAME_STATUS_SUCCESS)
    {
        cam_img_data imgObj;
        //对图像进行某些操作
        dh->g_bIsGetImage=true;//是否取到照片设置为TRUE

        // 设置图像信息
        imgObj.nFrameID = pFrame->nFrameID;
        imgObj.nTimestamp = pFrame->nTimestamp;
        imgObj.imgMat = cv::Mat(pFrame->nHeight, pFrame->nWidth, CV_8UC1, (unsigned char *)pFrame->pImgBuf).clone();
        

        struct timespec t;
        clock_gettime(CLOCK_REALTIME, &t);
        uint64_t timestamp_host = t.tv_sec * 1000000000 + t.tv_nsec;//纳秒级别的系统时间戳

        // cout<<"the number of device is:" << dh->cur_device_name << " 系统当前时间戳："<<to_string(timestamp_host)<<"，图像帧时间戳："<<to_string(pFrame->nTimestamp)<<",差值："<<to_string(timestamp_host-pFrame->nTimestamp)<<endl;
        // std::cout << cv::format(imgObj.imgMat, cv::Formatter::FMT_DEFAULT) << std::endl;

        uint64_t old_frame_count = dh->cam_frame_count;
        // 源头帧号检测，我们能知道的是nFrameNum是永远正确的，帧号从0开始
        if (dh->cam_frame_count != pFrame->nFrameID) {
             //当前相机帧时间戳
            uint64_t current_frame_timestamp = pFrame->nTimestamp;
            //插入到丢帧队列中
            dh->lost_frame_queue.push(current_frame_timestamp);
            while(dh->lost_frame_queue.size() > 1){
                //取队列头部时间戳
                uint64_t ear_frame_timestamp = dh->lost_frame_queue.front();
                //在区间时间戳阈值内，则判断丢帧数量是否超出预期
                if(current_frame_timestamp - ear_frame_timestamp <= dh->lost_frame_time_thersold){
                    if(dh->lost_frame_queue.size() >= dh->lost_frame_count_thersold){
                        //告警
                        msg = format_multi_args("the number of frame in time interval is %d",dh->lost_frame_queue.size());
                        dh->logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_ALARM);
                        dh->g_lostFrame = true; //该相机丢帧异常
                        except_code_queue->Push(CAM_LOST_FRAME);
                        event_number_queue->Push(E8);//触发相机模块异常事件，重启相机资源，在业务线程重启
                        while(!dh->lost_frame_queue.empty()) dh->lost_frame_queue.pop();
                    }else{
                        break;
                    }
                }else{
                    dh->lost_frame_queue.pop();
                }
            }
            
            msg = format_multi_args("the number of device is %d，Frame lost interval in (%lld,%lld)",dh->cur_device_name,old_frame_count,pFrame->nFrameID);
            dh->logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_ALARM);
            dh->statis_frame_status();

            //校准帧号
            dh->cam_frame_count = pFrame->nFrameID;
        }

        dh->cam_frame_count++;

        msg = format_multi_args("the number of device is %d 系统当前时间戳 %lld, 图像帧时间戳 %lld, 差值：%lld",dh->cur_device_name,timestamp_host,pFrame->nTimestamp,timestamp_host-pFrame->nTimestamp);
        dh->logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_INFO);
        msg = format_multi_args("ImageCallBackEx success,the number of device is %d，cam_frame_count[%lld], nFrameNum[%lld]",dh->cur_device_name,old_frame_count,pFrame->nFrameID);
        dh->logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_INFO);
        
        // 还需要添加到对应的相机队列中
        if(level != ERR_MODE){
            if(dh->cur_device_name == 1){
                // cout<<"the number of device " << dh->cur_device_name <<" send image frame:" << pFrame->nFrameID << endl;
                camera_queue0->Push(imgObj);
            }else if(dh->cur_device_name == 2){
                camera_queue1->Push(imgObj);
            }else if(dh->cur_device_name == 3){
                camera_queue2->Push(imgObj);
            }else if(dh->cur_device_name == 4){
                camera_queue3->Push(imgObj);
            }
        }
    }else
    {
        //回调取图没有取到，需要告警
        except_code_queue->Push(CAM_IMAGE_CALL_FAILURE);
        event_number_queue->Push(E8);//触发其他模块异常事件
        string msg = format_multi_args("ImageCallBackEx failed to grap image，the pFrame status is：%d,the number of device is %d",pFrame->status,dh->cur_device_name);
        dh->logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_ALARM);
        dh->alarm();
        // dh->release();
    }
}

void DhCamera::OnFeatureCallbackFun(GX_FEATURE_ID featureID, void* pUserParam)
{
    if (featureID == GX_INT_EVENT_EXPOSUREEND)
    {
    //获取此帧曝光结束事件对应的时间戳和帧 ID 等事件数据
    std::cout  << "获取到曝光结束事件" << std::endl;
        // int64_t nFrameID=0;
        // GXGetInt(pUserParam, GX_INT_EVENT_EXPOSUREEND_FRAMEID,&nFrameID);
    }
    return;
}

void DhCamera::detect_device_online()
{
    //以获取厂商名称为例，第一步获取字符串长度，第二步按照获取的长度分配 buffer
    //第三步获取字符串内容
    GX_STATUS status = GX_STATUS_SUCCESS;
    size_t nSize = 0;
    //首先获取字符串允许的最大长度 (此长度包含结束符’\0’)
    status = GXGetStringMaxLength(g_hDevice, GX_STRING_DEVICE_MODEL_NAME,&nSize);
    //根据获取的长度申请内存
    char *pszText = new char[nSize];
    status = GXGetString(g_hDevice, GX_STRING_DEVICE_MODEL_NAME, pszText,&nSize);
    if(status != GX_STATUS_SUCCESS)
    {
        LOG_PLUS("大恒相机 "+to_string(cur_device_name)+" 已掉线!"+to_string(status), LOGGER_ALARM);
    }else{
        LOG_PLUS(to_string(cur_device_name)+" status : "+to_string(status), LOGGER_INFO);
    }
}

void DhCamera::statis_frame_status()
{
    int64_t nGetFrameCount = 0;//接受帧个数,包括残帧
    int64_t nLostFrameCount = 0;//丢帧个数
    int64_t nInCompleteFrameCount = 0;//残帧个数
    GX_STATUS emStatus = 1;
    emStatus = GXGetInt(g_hDevice, GX_DS_INT_DELIVERED_FRAME_COUNT, &nGetFrameCount);
    emStatus = GXGetInt(g_hDevice, GX_DS_INT_DELIVERED_FRAME_COUNT, &nLostFrameCount);
    emStatus = GXGetInt(g_hDevice, GX_DS_INT_DELIVERED_FRAME_COUNT, &nInCompleteFrameCount);
    string msg = format_multi_args("the number of device is %d, total frame's number is %lld, lost frame's number is %lld,incomplete frame is %lld",cur_device_name,nGetFrameCount,nLostFrameCount,nInCompleteFrameCount);
    logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_ALARM);
}

void DhCamera::work_thread(void *pUser)
{
    if(g_bIsGetImage)
    {

    }
}

void DhCamera::init(int device_name,float exposure_time)
{
    //心跳机制，尝试三次，赋值4因为首先正常连接一次，出现问题后在重试三次
    unsigned int retry_cnt = 4;
    std::string msg = "";
    GX_STATUS emStatus = 1;
    g_lostFrame = false;
    while(GX_STATUS_SUCCESS != emStatus && retry_cnt > 0)
    {
        retry_cnt--;
        uint32_t ui32DeviceNum = 0;

        // Initialize libary
        emStatus = GXInitLib();

        if (emStatus != GX_STATUS_SUCCESS)
        {
            get_error_string(emStatus);
            continue;
        }

        // Get device enumerated number
        emStatus = GXUpdateDeviceList(&ui32DeviceNum, 1000);
        if (emStatus != GX_STATUS_SUCCESS)
        {
            get_error_string(emStatus);
            GXCloseLib();
            continue;
        }

        // If no device found, app exit
        if (ui32DeviceNum <= 0)
        {
            printf("<No device found>\n");
            GXCloseLib();
            emStatus = 1;//这里表示没有找到设备，需要重新置换新的状态
            msg = format_multi_args("No device found");
            logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_ERROR);
            continue;
        }

        printf("the number of device is %d\n", device_name);

        cur_device_name = device_name;

        // select the correspand device_number，选择对应的的相机
        emStatus = GXOpenDeviceByIndex(device_name, &g_hDevice);
        if (emStatus != GX_STATUS_SUCCESS)
        {
            get_error_string(emStatus);
            GXCloseLib();
            continue;
        }

        /************************************参数配置如下*****************************************/
        // //读取当前的设备心跳时间
        // int64_t nValue = 0;
        // emStatus = GXGetInt(g_hDevice, GX_INT_GEV_HEARTBEAT_TIMEOUT, &nValue);
        // //设置设备心跳时间-------API文档强烈推荐用户使用默认值，除非特殊情况才允许用户根据现场环境修改此值
        // emStatus = GXSetInt(g_hDevice, GX_INT_GEV_HEARTBEAT_TIMEOUT, nValue);
        // if (emStatus != GX_STATUS_SUCCESS)
        // {
        //     get_error_string(emStatus);
        //     GXCloseLib();
        // }
        


        //设置触发源
        emStatus = GXSetEnum(g_hDevice, GX_ENUM_TRIGGER_SELECTOR, GX_ENUM_TRIGGER_SELECTOR_FRAME_START);

        emStatus = GXSetEnum(g_hDevice, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE0);

        //设置触发模式为 ON
        emStatus = GXSetEnum(g_hDevice, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON);
        //设置触发激活方式为上升沿
        emStatus = GXSetEnum(g_hDevice, GX_ENUM_TRIGGER_ACTIVATION,
                           GX_TRIGGER_ACTIVATION_RISINGEDGE);


        /*********************帧率控制******************/
        //使能采集帧率调节模式
        // emStatus = GXSetEnum(g_hDevice, GX_ENUM_ACQUISITION_FRAME_RATE_MODE, GX_ACQUISITION_FRAME_RATE_MODE_ON);
        // if (emStatus != GX_STATUS_SUCCESS)
        // {
        //     get_error_string(emStatus);
        //     GXCloseLib();
        //     continue;
        // }

        // //设置采集帧率,假设设置为 10.0，用户按照实际需求设置此值
        // emStatus = GXSetFloat(g_hDevice, GX_FLOAT_ACQUISITION_FRAME_RATE, 2);
        // if (emStatus != GX_STATUS_SUCCESS)
        // {
        //     get_error_string(emStatus);
        //     GXCloseLib();
        //     continue;
        // }

        //获取曝光调节范围
        // double dExposureValue = 150.0;
        // //设置连续自动曝光
        // status = GXSetEnum(hDevice, GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_CONTINUOUS);
        // //设置曝光延迟为 2us
        // status = GXSetFloat(hDevice, GX_FLOAT_EXPOSURE_DELAY, dExposureValue);
        emStatus = GXSetFloat(g_hDevice, GX_FLOAT_EXPOSURE_TIME, exposure_time);
        // //设置曝光时间模式为极小曝光模式
        // status = GXSetEnum(hDevice, GX_ENUM_EXPOSURE_TIME_MODE, GX_EXPOSURE_TIME_MODE_ULTRASHORT);

        // 注册设备掉线回调函数(不支持)
        // emStatus = GXRegisterDeviceOfflineCallback(g_hDevice, this, OnDeviceOfflineCallbackEx, &hCB);
        // if (emStatus != GX_STATUS_SUCCESS)
        // {
        //     get_error_string(emStatus);
        //     GXCloseLib();
        //     continue;
        // }

        // 设置ROI
        emStatus = GXSetInt(g_hDevice, GX_INT_WIDTH, 1440);
        emStatus = GXSetInt(g_hDevice, GX_INT_HEIGHT, 1080);

        // 注册事件回调
        GX_FEATURE_CALLBACK_HANDLE hCB;
        GXRegisterFeatureCallback(g_hDevice, this, OnFeatureCallbackFun, GX_INT_EVENT_EXPOSUREEND, &hCB);

        //注册图像处理回调函数
        emStatus = GXRegisterCaptureCallback(g_hDevice, this, ImageCallBackEx);


        if (emStatus != GX_STATUS_SUCCESS)
        {
            get_error_string(emStatus);
            GXCloseLib();
            continue;
        }

        //发送开采命令
        emStatus = GXSendCommand(g_hDevice, GX_COMMAND_ACQUISITION_START);
        if (emStatus != GX_STATUS_SUCCESS)
        {
            get_error_string(emStatus);
            GXCloseLib();
            continue;
        }

    }
   
    if (emStatus != GX_STATUS_SUCCESS)
    {
        LOG_SOCKET("相机启动重试三次失败", LOGGER_ALARM);
        except_code_queue->Push(CAM_RETRY_FAILURE);
        event_number_queue->Push(E8);//触发其他模块异常事件
        if(g_hDevice != nullptr)
        {
            release();
        }
    }

}

//重置帧号
void DhCamera::reset_frame_number()
{
    //发送停采命令
    GXSendCommand(g_hDevice, GX_COMMAND_ACQUISITION_STOP);

    //发送开采命令
    GXSendCommand(g_hDevice, GX_COMMAND_ACQUISITION_START);
}

//本方法只是把相机包含的资源delete掉，对象的引用还在，无法在回调函数中调用这个方法
void DhCamera::release()
{
    GX_STATUS emStatus = GX_STATUS_SUCCESS;
    //发送停采命令
    emStatus = GXSendCommand(g_hDevice, GX_COMMAND_ACQUISITION_STOP);
    //注销采集回调
    emStatus = GXUnregisterCaptureCallback(g_hDevice);
    // 注销设备掉线事件回调(不支持)
    // emStatus = GXUnregisterDeviceOfflineCallback(g_hDevice, hCB);
    
    emStatus = GXCloseLib();
    if (emStatus != GX_STATUS_SUCCESS)
    {
        get_error_string(emStatus);
    }

    delete g_hDevice;
}

void DhCamera::run()
{
}

void DhCamera::execute()
{
}

void DhCamera::alarm()
{
}