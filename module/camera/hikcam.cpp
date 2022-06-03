#include "hikcam.h"
#include"MvCameraControl.h"
#include<string>
HikCamera::HikCamera(int device_name)
{
    HikCamera::init(device_name, MV_TRIGGER_MODE_ON, CFG_CAM_FRAME_RATE_CONTROL, MV_TRIGGER_SOURCE_LINE0,
        MV_TRIGGER_RISINGEDGE, CFG_CAM_ANTI_SHAKE_TIME, CFG_CAM_GEV, CFG_CAM_EXPOSURE_TIME, CFG_CAM_GAIN, CFG_CAM_CACHE_CAPACITY);
}

HikCamera::HikCamera(int device_name,
    MV_CAM_TRIGGER_MODE trigger_mode,
    bool frame_rate_control,
    MV_CAM_TRIGGER_SOURCE trigger_source,
    MV_CAM_TRIGGER_ACTIVATION trigger_polarity,
    int anti_shake_time,
    int GEV,
    float exposure_time,
    float gain,
    unsigned int cache_capacity)
{
    HikCamera::init(device_name, trigger_mode, frame_rate_control, trigger_source, trigger_polarity,
        anti_shake_time, GEV, exposure_time, gain, cache_capacity);
}

HikCamera::~HikCamera()
{
    HikCamera::release();
}

//回调取图函数
void __stdcall HikCamera::ImageCallBackEx(unsigned char * pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser)
{
    HikCamera *ph = (HikCamera*) pUser;
    std::string msg = "";

    //取到图
    if (pFrameInfo)
    {
        ph->stImageInfo = *pFrameInfo;
        printf("GetOneFrame, Width[%d], Height[%d], nFrameNum[%d]\n", 
            pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->nFrameNum);
        
        ph->g_bIsGetImage = true;

        //10ns精度的时间戳
        ph->cam_get_time = static_cast<unsigned long long>(ph->stImageInfo.nDevTimeStampHigh) << 32 | static_cast<unsigned long long>(ph->stImageInfo.nDevTimeStampLow);
        MVCC_INTVALUE_EX mvcc;
        MV_CC_GetIntValueEx(ph,"GevTimestampValue",&mvcc);
        unsigned long long timeStamp_tree =mvcc.nCurValue;//获取相机属性树时间戳
        unsigned long long diff;//时间戳差值
        if(timeStamp_tree>(ph->cam_get_time)){
            diff=(timeStamp_tree-(ph->cam_get_time));
        }else{
            diff=((ph->cam_get_time)-timeStamp_tree);
        }
        std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());//获取当前时间点
        std::time_t timestamp_host = tp.time_since_epoch().count(); //计算距离1970-1-1,00:00的时间长度
        string logInfo="系统时间戳 = "+to_string(timestamp_host)+", 出图时间 = "+to_string(diff)+"\n";
        LOG_PLUS(logInfo, LOGGER_INFO);
        
        //1.超过变成1
        if(ph->cam_frame_count == std::numeric_limits<unsigned int>::max()) ph->cam_frame_count = 0;
        ph->cam_frame_count++;

        int old_frame_count = ph->cam_frame_count;
        // 源头帧号检测，我们能知道的是nFrameNum是永远正确的
        if (ph->cam_frame_count % 65535 != ph->stImageInfo.nFrameNum) {
            //1.cam_frame_count不为0，并且是65535的整数倍，而nFrameNum是为65535，说明帧号是一样的，没有产生网络丢帧，除此以外都是存在丢帧的
            if(ph->cam_frame_count != 0 && ph->cam_frame_count % 65535 == 0 && ph->stImageInfo.nFrameNum == 65535){

            }else{
                //2.比如超过int的2直接赋值为2
                if(std::numeric_limits<unsigned int>::max()-ph->cam_frame_count < ph->stImageInfo.nFrameNum - ph->cam_frame_count % 65535){
                    ph->cam_frame_count = ph->stImageInfo.nFrameNum - ph->cam_frame_count % 65535 - (std::numeric_limits<unsigned int>::max()-ph->cam_frame_count);
                }else{
                    ph->cam_frame_count += ph->stImageInfo.nFrameNum - ph->cam_frame_count % 65535;
                }
                
                // TODO: 传输层丢失图片是否需要重启相机（待定）
                //比如可以从丢多少帧重启相机
                msg = format_multi_args("Frame lost interval in (%d,%d)",old_frame_count,ph->cam_frame_count);
                ph->logger.log(msg,LOGGER_ALARM);
            }
        }

        printf("cam_frame_count[%d], nFrameNum[%d]\n", 
           ph->cam_frame_count, ph->stImageInfo.nFrameNum);
    }else{
        //回调取图没有取到，也就是异常，需要告警
        msg = "ImageCallBackEx failed to grap image";
        ph->logger.log(msg,LOGGER_ALARM);
        ph->alarm();
        ph->release();
    }
}

//线程调用的函数，启动线程后，使用这个函数来监听相机的回调取图情况，和java的线程的run()是一样的道理
void HikCamera::work_thread(void* pUser){
    HikCamera *ph = (HikCamera*) pUser;
    while(1){
        //取图成功
        if(g_bIsGetImage){
            std::cout << "device:" << ph->cur_device_name << "的取图状态：" << ph->g_bIsGetImage << std::endl;
            // printf("GetOneFrame, Width[%d], Height[%d], nFrameNum[%d]\n", 
            // stImageInfo->nWidth, stImageInfo->nHeight, stImageInfo->nFrameNum);
            // if(ph->cur_device_name == 0){
            //     camera_queue0->Push(ph->stImageInfo);
            // }else if(ph->cur_device_name == 1){
            //     camera_queue1->Push(ph->stImageInfo);
            // }else if(ph->cur_device_name == 2){
            //     camera_queue2->Push(ph->stImageInfo);
            // }else if(ph->cur_device_name == 3){
            //     camera_queue3->Push(ph->stImageInfo);
            // }
            //相关逻辑 TODO 把数据推到消息队列中
            ph->g_bIsGetImage = false;
        }
    }
}


//执行相机开始取流，并注册回调函数
void HikCamera::execute(){

}


//向服务端告警
void HikCamera::alarm(){
    //TODO
    std::cout << "alarm" << std::endl;
}

//相机初始化
void HikCamera::init(int device_name,
    MV_CAM_TRIGGER_MODE trigger_mode,
    bool frame_rate_control,
    MV_CAM_TRIGGER_SOURCE trigger_source,
    MV_CAM_TRIGGER_ACTIVATION trigger_polarity,
    int anti_shake_time,
    int GEV,
    float exposure_time,
    float gain,
    unsigned int cache_capacity)
{
    //心跳机制，尝试三次，赋值4因为首先正常连接一次，出现问题后在重试三次
    unsigned int retry_cnt = 4;
    std::string msg = "";
    //不是OK状态并且重试次数大于0，则继续重试
    while(MV_OK != nRet && retry_cnt > 0)
    {
        retry_cnt--;
        cur_device_name = device_name;
        // 枚举设备
        MV_CC_DEVICE_INFO_LIST stDeviceList;
        memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
        nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
        if (MV_OK != nRet) {
            printf("Enum Devices fail! nRet [0x%x]\n", nRet);
            msg = format_multi_args("Enum Devices fail! nRet [0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }
        if (stDeviceList.nDeviceNum != CFG_CAM_CAM_MOUNT_COUNT) {
            printf("@@@Camera[%d] Check: Fail@@@\n", device_name);
            msg = format_multi_args("@@@Camera[%d] Check: Fail@@@",device_name);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 选择设备并创建句柄
        nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[device_name]);
        if (MV_OK != nRet) {
            printf("Create Handle fail! nRet [0x%x]\n", nRet);
            msg = format_multi_args("Create Handle fail! nRet [0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 打开设备
        nRet = MV_CC_OpenDevice(handle);
        if (MV_OK != nRet) {
            printf("Open Device fail! nRet [0x%x]\n", nRet);
            msg = format_multi_args("Open Device fail! nRet [0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }
        printf("@@@Camera[%d] Check: Pass@@@\n", device_name);
        cur_device_name = device_name;

        // 探测网络最佳包大小(只对GigE相机有效)
        if (stDeviceList.pDeviceInfo[device_name]->nTLayerType == MV_GIGE_DEVICE) {
            int nPacketSize = MV_CC_GetOptimalPacketSize(handle);
            if (nPacketSize > 0) {
                nRet = MV_CC_SetIntValue(handle, "GevSCPSPacketSize", nPacketSize);
                if (nRet != MV_OK) {
                    printf("Warning: Set Packet Size fail nRet [0x%x]!\n", nRet);
                    msg = format_multi_args("Warning: Set Packet Size fail nRet [0x%x]!",nRet);
                    logger.log(msg,LOGGER_ERROR);
                    continue;
                }
            }
            else {
                printf("Warning: Get Packet Size fail nRet [0x%x]!\n", nPacketSize);
                msg = format_multi_args("Warning: Get Packet Size fail nRet [0x%x]!",nRet);
                logger.log(msg,LOGGER_ERROR);
                continue;
            }
        }
        
        /************************************参数配置如下*****************************************/

        // 设置心跳超时时间为500ms(防止掉线后失联阻塞)
        nRet = MV_CC_SetIntValue(handle, "GevHeartbeatTimeout", 500);
        if (MV_OK != nRet) {
            printf("Set GevHeartbeatTimeout fail! nRet [0x%x]\n", nRet);
            msg = format_multi_args("Set GevHeartbeatTimeout fail! nRet [0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 设置触发模式为ON
        nRet = MV_CC_SetEnumValue(handle, "TriggerMode", trigger_mode);
        if (MV_OK != nRet) {
            printf("Set Trigger Mode fail! nRet [0x%x]\n", nRet);
            msg = format_multi_args("Set Trigger Mode fail! nRet [0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 关闭帧率控制
        nRet = MV_CC_SetBoolValue(handle, "AcquisitionFrameRateEnable", frame_rate_control);
        if (MV_OK != nRet) {
            printf("Set acquisition frame rate fail! ret[0x%x]\n", nRet);
            msg = format_multi_args("Set acquisition frame rate fail! ret[0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 设置触发源为Line0
        nRet = MV_CC_SetEnumValue(handle, "TriggerSource", trigger_source);
        if (MV_OK != nRet) {
            printf("Set trigger source fail! ret[0x%x]\n", nRet);
            msg = format_multi_args("Set trigger source fail! ret[0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 设置触发源为上升沿
        if (trigger_source == MV_TRIGGER_SOURCE_LINE0 || trigger_source == MV_TRIGGER_SOURCE_LINE1 || 
            trigger_source == MV_TRIGGER_SOURCE_LINE2 || trigger_source == MV_TRIGGER_SOURCE_LINE3) {
            nRet = MV_CC_SetEnumValue(handle, "TriggerActivation", trigger_polarity);
            if (MV_OK != nRet) {
                printf("Set trigger activation fail! ret[0x%x]\n", nRet);
                msg = format_multi_args("Set trigger activation fail! ret[0x%x]",nRet);
                logger.log(msg,LOGGER_ERROR);
                continue;
            }
        }     

        // 设置线路防抖时间为1us
        nRet = MV_CC_SetIntValue(handle, "LineDebouncerTime", anti_shake_time);
        if (MV_OK != nRet) {
            printf("Set line debouncer time fail! ret[0x%x]\n", nRet);
            msg = format_multi_args("Set line debouncer time fail! ret[0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 设置GEV SCPD值
        nRet = MV_CC_SetIntValue(handle, "GevSCPD", GEV);
        if (MV_OK != nRet) {
            printf("Set Gev SCPD fail! ret[0x%x]\n", nRet);
            msg = format_multi_args("Set Gev SCPD fail! ret[0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 设置曝光时间
        nRet = MV_CC_SetEnumValue(handle, "ExposureMode", MV_EXPOSURE_MODE_TIMED);
        if (MV_OK != nRet) {
            printf("Set Exposure Mode fail! ret[0x%x]\n", nRet);
            msg = format_multi_args("Set Exposure Mode fail! ret[0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        nRet = MV_CC_SetEnumValue(handle, "ExposureAuto", MV_EXPOSURE_AUTO_MODE_OFF);
        if (MV_OK != nRet) {
            printf("Set Exposure Auto fail! ret[0x%x]\n", nRet);
            msg = format_multi_args("Set Exposure Auto fail! ret[0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        nRet = MV_CC_SetFloatValue(handle, "ExposureTime", exposure_time);
        if (MV_OK != nRet) {
            printf("Set Exposure Time fail! ret[0x%x]\n", nRet);
            msg = format_multi_args("Set Exposure Time fail! ret[0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 设置增益
        nRet = MV_CC_SetEnumValue(handle, "GainAuto", 0);
        if (MV_OK != nRet) {
            printf("Set Gain Auto fail! ret[0x%x]\n", nRet);
            msg = format_multi_args("Set Gain Auto fail! ret[0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        nRet = MV_CC_SetFloatValue(handle, "Gain", gain);
        if (MV_OK != nRet) {
            printf("Set gain fail! ret[0x%x]\n", nRet);
            msg = format_multi_args("Set gain fail! ret[0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 设置触发缓存
        nRet = MV_CC_SetBoolValue(handle, "TriggerCacheEnable", true);
        if (MV_OK != nRet) {
            printf("Set Trigger Cache Enable fail! ret[0x%x]\n", nRet);
            msg = format_multi_args("Set Trigger Cache Enable fail! ret[0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 设置图像缓存节点数量为100
        nRet = MV_CC_SetImageNodeNum(handle, cache_capacity);
        if (MV_OK != nRet) {
            printf("Set image node num fail! ret[0x%x]\n", nRet);
            msg = format_multi_args("Set image node num fail! ret[0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 获取数据包大小
        memset(&stParam, 0, sizeof(MVCC_INTVALUE));
        nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
        if (MV_OK != nRet) {
            printf("Get PayloadSize fail! nRet [0x%x]\n", nRet);
            msg = format_multi_args("Get PayloadSize fail! nRet [0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 获取像素格式
        memset(&stEnumParam, 0, sizeof(MVCC_ENUMVALUE));
        nRet = MV_CC_GetEnumValue(handle, "PixelFormat", &stEnumParam);
        if (MV_OK != nRet) {
            printf("Get pixel format fail! ret[0x%x]\n", nRet);
            msg = format_multi_args("Get pixel format fail! ret[0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        //注册回调取图，用户自定义变量，传this指针过去
        nRet = MV_CC_RegisterImageCallBackEx(handle,&HikCamera::ImageCallBackEx,this);
        if (MV_OK != nRet){
            printf("MV_CC_RegisterImageCallBackEx fail! nRet [%x]\n", nRet);
            continue; 
        }

        // 开始取流
        nRet = MV_CC_StartGrabbing(handle);
        if (MV_OK != nRet) {
            printf("Start grabbing fail! nRet [%x]\n", nRet);
            msg = format_multi_args("Start grabbing fail! nRet [%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }

        // 数据流预处理
        memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
        pData = (unsigned char*)malloc(sizeof(unsigned char) * stParam.nCurValue);
        if (nullptr == pData) {
            printf("Allocate memory fail! nRet [%x]\n", nRet);
            msg = format_multi_args("Allocate memory fail! nRet [%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            continue;
        }
        nDataSize = stParam.nCurValue;
    }
   
    if (nRet != MV_OK) {
        if (handle != nullptr) {
            // MV_CC_DestroyHandle(handle);
            release();
            handle = nullptr;
        }
        //发出告警信息
        msg = format_multi_args("the haikang camera device_name [%d] init failed",device_name);
        logger.log(msg,LOGGER_ALARM);
        alarm();
    }else{
        msg = format_multi_args("the haikang camera device_name [%d] init success!!!",device_name);
        logger.log(msg,LOGGER_INFO);
    }
}

void HikCamera::run()
{
    if (handle != nullptr) {
        nRet = MV_CC_GetOneFrameTimeout(handle, pData, nDataSize, &stImageInfo, 1000);
        if (nRet == MV_OK) {
            // 相机驱动时间
            cam_get_time = static_cast<unsigned long long>(stImageInfo.nDevTimeStampHigh) << 32 | static_cast<unsigned long long>(stImageInfo.nDevTimeStampLow);
            cam_frame_count++;

            // 源头帧号检测
            if (cam_frame_count % 65535 != stImageInfo.nFrameNum) {
                if (stImageInfo.nFrameNum != 65535) {
                    // 修正新的帧号
                    cam_frame_count += stImageInfo.nFrameNum - cam_frame_count % 65535;
                }
                // TODO: 传输层丢失图片是否需要重启相机（待定）
            }
            printf("CAM[%d] Get One Frame, Width[%d], Height[%d], Frame Num[%d], Host time[%ld], Get time[%lld], Lost pack[%d]\n",
                cur_device_name, stImageInfo.nWidth, stImageInfo.nHeight, cam_frame_count, stImageInfo.nHostTimeStamp, cam_get_time, stImageInfo.nLostPacket);
//            src.host_time = stImageInfo.nHostTimeStamp;
//            src.frame_num = cam_frame_count;
//            if (Is_mono_data(stEnumParam.nCurValue))
//                src.img = cv::Mat(stImageInfo.nHeight, stImageInfo.nWidth, CV_8UC1, pData).clone();
//            else if (Is_color_data(stEnumParam.nCurValue))
//                src.img = cv::Mat(stImageInfo.nHeight, stImageInfo.nWidth, CV_8UC3, pData).clone();
        }
    }
    else {
        // TODO:句柄无效后删除资源后重启(待)
        //printf("No cam\n");
        sleep(100);
    }
}

void HikCamera::release()
{
    std::string msg = "";
    do {
        // 停止取流
        nRet = MV_CC_StopGrabbing(handle);
        if (MV_OK != nRet) {
            printf("Stop Grabbing fail! nRet [0x%x]\n", nRet);
            msg = format_multi_args("Stop Grabbing fail! nRet [0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            break;
        }

        // 关闭设备
        nRet = MV_CC_CloseDevice(handle);
        if (MV_OK != nRet) {
            printf("ClosDevice fail! nRet [0x%x]\n", nRet);
            msg = format_multi_args("ClosDevice fail! nRet [0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            break;
        }

        // 销毁句柄
        nRet = MV_CC_DestroyHandle(handle);
        if (MV_OK != nRet) {
            printf("Destroy Handle fail! nRet [0x%x]\n", nRet);
            msg = format_multi_args("Destroy Handle fail! nRet [0x%x]",nRet);
            logger.log(msg,LOGGER_ERROR);
            break;
        }
    } while (false);
    if (nRet != MV_OK) {
        if (handle != nullptr) {
            MV_CC_DestroyHandle(handle);
            handle = nullptr;
        }
    }
}

bool HikCamera::Virtual_input(MV_CAM_VIRTUAL_INPUT type)
{
    nRet = MV_CC_SetEnumValue(handle, "TestPattern", type);
    if (MV_OK != nRet) {
        printf("Modify virtual input fail! nRet [0x%x]\n", nRet);
        return false;
    }
    return true;
}

bool HikCamera::Software_trigger()
{
    nRet = MV_CC_SetCommandValue(handle, "TriggerSoftware");
    if (MV_OK != nRet) {
        printf("Send Trigger Software command fail! nRet [0x%x]\n", nRet);
        return false;
    }
    return true;
}

float HikCamera::Get_frame_rate()
{
    MVCC_FLOATVALUE frame_rate{};
    memset(&frame_rate, 0, sizeof(MVCC_FLOATVALUE));
    nRet = MV_CC_GetFloatValue(handle, "ResultingFrameRate", &frame_rate);
    if (MV_OK != nRet) {
        printf("Get frame rate fail! nRet [0x%x]\n", nRet);
        return -1.0;
    }
    return frame_rate.fCurValue;
    
}

bool HikCamera::Dynamic_adjust_exposure(float value)
{
    nRet = MV_CC_SetFloatValue(handle, "ExposureTime", value);
    if (MV_OK != nRet) {
        printf("Set line Exposure Time fail! ret[0x%x]\n", nRet);
        return false;
    }
    return true;
}

bool HikCamera::Dynamic_adjust_gain(float value)
{
    nRet = MV_CC_SetFloatValue(handle, "Gain", value);
    if (MV_OK != nRet) {
        printf("Set gain fail! ret[0x%x]\n", nRet);
        return false;
    }
    return true;
}

void HikCamera::Enum_devices()
{
    do {
        MV_CC_DEVICE_INFO_LIST stDeviceList;
        memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
        bool nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
        if (MV_OK != nRet) {
            printf("Enum Devices fail! nRet [0x%x]\n", nRet);
            break;
        }
        if (stDeviceList.nDeviceNum > 0) {
            for (int i = 0; i < stDeviceList.nDeviceNum; i++) {
                printf("[device %d]:\n", i);
                MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
                if (nullptr == pDeviceInfo)
                    break;
                PrintDeviceInfo(pDeviceInfo);
            }
        }
    }
    while (false);
}

bool HikCamera::PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo)
{
    if (pstMVDevInfo->nTLayerType == MV_GIGE_DEVICE) {
        unsigned int nIp1 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
        unsigned int nIp2 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
        unsigned int nIp3 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
        unsigned int nIp4 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);

        // 打印当前相机ip和用户自定义名字
        printf("Device Model Name: %s\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chModelName);
        printf("CurrentIp: %d.%d.%d.%d\n", nIp1, nIp2, nIp3, nIp4);
        printf("UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
    }
    else if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE) {
        printf("Device Model Name: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chModelName);
        printf("UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
    }
    else
        printf("Not support.\n");
    return true;
}

bool HikCamera::Is_mono_data(unsigned int PixelType)
{
    return PixelType_Gvsp_Mono8 == PixelType || PixelType_Gvsp_Mono10 == PixelType
        || PixelType_Gvsp_Mono10_Packed == PixelType || PixelType_Gvsp_Mono12 == PixelType
        || PixelType_Gvsp_Mono12_Packed == PixelType;
}

bool HikCamera::Is_color_data(unsigned int PixelType)
{
    return PixelType_Gvsp_BayerGR8 == PixelType || PixelType_Gvsp_BayerRG8 == PixelType
        || PixelType_Gvsp_BayerGB8 == PixelType || PixelType_Gvsp_BayerBG8 == PixelType
        || PixelType_Gvsp_BayerGR10 == PixelType || PixelType_Gvsp_BayerRG10 == PixelType
        || PixelType_Gvsp_BayerGB10 == PixelType || PixelType_Gvsp_BayerBG10 == PixelType
        || PixelType_Gvsp_BayerGR12 == PixelType || PixelType_Gvsp_BayerRG12 == PixelType
        || PixelType_Gvsp_BayerGB12 == PixelType || PixelType_Gvsp_BayerBG12 == PixelType
        || PixelType_Gvsp_BayerGR10_Packed == PixelType || PixelType_Gvsp_BayerRG10_Packed == PixelType
        || PixelType_Gvsp_BayerGB10_Packed == PixelType || PixelType_Gvsp_BayerBG10_Packed == PixelType
        || PixelType_Gvsp_BayerGR12_Packed == PixelType || PixelType_Gvsp_BayerRG12_Packed == PixelType
        || PixelType_Gvsp_BayerGB12_Packed == PixelType || PixelType_Gvsp_BayerBG12_Packed == PixelType
        || PixelType_Gvsp_YUV422_Packed == PixelType || PixelType_Gvsp_YUV422_YUYV_Packed == PixelType;
}
