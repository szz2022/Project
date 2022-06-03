#pragma once
#include "../logger/logger.h"

/**
 * author：lq
 * description：设计思想，通过纯虚函数实现多态机制，通过Camera对象提供对外的统一调用接口
 * **/
class Camera{
public:
    Camera(){}
    //初始化相机参数
    virtual void init(int device_name,float exposure_time)=0;
    //执行相机开始取流，并注册回调函数
    virtual void execute() = 0;
    //释放相机资源
    virtual void release() = 0;
    // virtual void enum_device() = 0; //枚举当前在线设备，返回设备列表
    virtual void run() = 0;
    //告警
    virtual void alarm() = 0;
    //线程调用的函数，启动线程后，使用这个函数来监听相机的回调取图情况，和java的线程的run()是一样的道理
    virtual void work_thread(void* pUser) = 0;
    // bool g_bIsGetImage = false;
    //是否发生丢帧异常
    bool g_lostFrame = false;
    virtual void reset_frame_number() = 0;
    virtual void detect_device_online() = 0;
protected:
    Logger logger = Logger::get_instance();
    //丢帧队列
    std::queue<uint64_t> lost_frame_queue;
    //丢帧区间时间阈值
    uint64_t lost_frame_time_thersold;
    int lost_frame_count_thersold;
};
