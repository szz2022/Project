#pragma once

#include "../common/header.h"
#include "GxIAPI.h"
#include "DxImageProc.h"
#include "camera.h"
#include <sys/time.h>
#include "../socket/socket.h"
#include "../socket/socket.h"

#define GX_STATUS_OFFLINE -4
#define LOST_FRAME_COUNT_THERSOLD 10
#define LOST_FRAME_TIME_THERSOLD 10000
class DhCamera : public Camera
{
public:
    DhCamera(int device_name);
    
    ~DhCamera();

    void init(int device_name,float exposure_time);

    // 掉线处理函数
    static void GX_STDC OnDeviceOfflineCallbackEx(void* pUserParam);

    //回调取图函数
    static void  GX_STDC ImageCallBackEx(GX_FRAME_CALLBACK_PARAM* pFrame);
    
    //回调事件函数
    void GX_STDC OnFeatureCallbackFun(GX_FEATURE_ID featureID, void* pUserParam);

    void reset_frame_number();
    // //释放相机资源
    void release();
    void run();

    // //执行相机开始取流，并注册回调函数
    void execute();

    // //向服务端告警
    void alarm();

    // //线程调用的函数，启动线程后，使用这个函数来监听相机的回调取图情况，和java的线程的run()是一样的道理
    void work_thread(void* pUser);
    void detect_device_online();
    
private:
    //是否获取到图片
    bool g_bIsGetImage = false;
    void* handle = nullptr;
    int cur_device_name = -1;

    GX_DEV_HANDLE g_hDevice = NULL;//< Device handle
            
    // GX_EVENT_CALLBACK_HANDLE hCB; // 定义掉线回调函数句柄(不支持)

    uint64_t cam_frame_count = 0;
    unsigned long long cam_get_time = 0;

    void get_error_string(GX_STATUS emErrorStatus);
    void statis_frame_status();
};