#pragma once

#include "../common/header.h"
#include "camera.h"
#include "../operation/operation.h"
class HikCamera : public Camera
{
public:
    HikCamera(int device_name);
    HikCamera(int device_name,
        MV_CAM_TRIGGER_MODE trigger_mode,
        bool frame_rate_control,
        MV_CAM_TRIGGER_SOURCE trigger_source,
        MV_CAM_TRIGGER_ACTIVATION trigger_polarity,
        int anti_shake_time,
        int GEV,
        float exposure_time,
        float gain,
        unsigned int cache_capacity);
    ~HikCamera();
    
    //回调取图函数
    static void __stdcall ImageCallBackEx(unsigned char * pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser);
    //初始化相机参数
    void init(int device_name,
        MV_CAM_TRIGGER_MODE trigger_mode,
        bool frame_rate_control,
        MV_CAM_TRIGGER_SOURCE trigger_source,
        MV_CAM_TRIGGER_ACTIVATION trigger_polarity,
        int anti_shake_time,
        int GEV,
        float exposure_time,
        float gain,
        unsigned int cache_capacity);
    
    //释放相机资源
    void release();
    void run();

    //执行相机开始取流，并注册回调函数
    void execute();

    //向服务端告警
    void alarm();

    //线程调用的函数，启动线程后，使用这个函数来监听相机的回调取图情况，和java的线程的run()是一样的道理
    void work_thread(void* pUser);

    bool Virtual_input(MV_CAM_VIRTUAL_INPUT type);
    bool Software_trigger();
    float Get_frame_rate();

    bool Dynamic_adjust_exposure(float value);
    bool Dynamic_adjust_gain(float value);

    static void Enum_devices();
    
private:
    //是否获取到图片
    bool g_bIsGetImage = false;
    int nRet = 0x00000001;
    void* handle = nullptr;

    unsigned int nDataSize = 0;
    unsigned char* pData = nullptr;
    int cur_device_name = -1;

    MVCC_INTVALUE stParam;
    MVCC_ENUMVALUE stEnumParam;
    MV_FRAME_OUT_INFO_EX stImageInfo;

    static bool PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo);
    static bool Is_mono_data(unsigned int PixelType);
    static bool Is_color_data(unsigned int PixelType);

    unsigned int cam_frame_count = 0;
    unsigned long long cam_get_time = 0;
};