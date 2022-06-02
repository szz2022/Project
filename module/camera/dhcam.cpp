#include "dhcam.h"

DhCamera::DhCamera(int device_name)
{
    //时间单位是us
    init(device_name,150);
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
    // if(level == ERR_MODE){
    //     string msg = "the application is err_mode,ImageCallBackEx stop to handle next image";
    //     dh->logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_ALARM);
    //     return ;
    // }
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
            // TODO: 传输层丢失图片是否需要重启相机（待定）
            //比如可以从丢多少帧重启相机
            string msg = format_multi_args("the number of device is %d，Frame lost interval in (%lld,%lld)",dh->cur_device_name,old_frame_count,pFrame->nFrameID);
            dh->logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_ALARM);
            except_code_queue->Push(CAM_LOST_FRAME);
            event_number_queue->Push(E8);//触发其他模块异常事件
            // LOG_SOCKET(msg, LOGGER_ALARM);
            return ;
        }

        string msg = format_multi_args("ImageCallBackEx success,the number of device is %d，cam_frame_count[%lld], nFrameNum[%lld]",dh->cur_device_name,old_frame_count,pFrame->nFrameID);
        dh->logger.log_plus(__FILE__,__FUNCTION__,__LINE__,msg,LOGGER_INFO);
        // 还需要添加到对应的相机队列中
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

        // printf("cam_frame_count[%lld], nFrameNum[%lld]\n", 
        //    dh->cam_frame_count, pFrame->nFrameID);
        //记录图片运行日志
        dh->cam_frame_count++;
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

    while(GX_STATUS_SUCCESS != emStatus && retry_cnt > 0)
    {
        retry_cnt--;
        uint32_t ui32DeviceNum = 0;

        // Initialize libary
        emStatus = GXInitLib();

        // 处理设备掉线
        if(emStatus == GX_STATUS_OFFLINE){
            OnDeviceOfflineCallbackEx(this);
            get_error_string(emStatus);
            GXCloseLib();
            continue;
        }

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
        // emStatus = GXSetEnum(g_hDevice, GX_ENUM_TRIGGER_SELECTOR, GX_ENUM_TRIGGER_SELECTOR_FRAME_START);

        // emStatus = GXSetEnum(g_hDevice, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE0);

        // //设置触发模式为 ON
        // emStatus = GXSetEnum(g_hDevice, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON);
        // //设置触发激活方式为上升沿
        // emStatus = GXSetEnum(g_hDevice, GX_ENUM_TRIGGER_ACTIVATION,
        //                    GX_TRIGGER_ACTIVATION_RISINGEDGE);


        /*********************帧率控制******************/
        //使能采集帧率调节模式
        emStatus = GXSetEnum(g_hDevice, GX_ENUM_ACQUISITION_FRAME_RATE_MODE, GX_ACQUISITION_FRAME_RATE_MODE_ON);
        if (emStatus != GX_STATUS_SUCCESS)
        {
            get_error_string(emStatus);
            GXCloseLib();
            continue;
        }

        //设置采集帧率,假设设置为 10.0，用户按照实际需求设置此值
        emStatus = GXSetFloat(g_hDevice, GX_FLOAT_ACQUISITION_FRAME_RATE, 2);
        if (emStatus != GX_STATUS_SUCCESS)
        {
            get_error_string(emStatus);
            GXCloseLib();
            continue;
        }

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