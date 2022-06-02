#pragma once

#include "hikcam.h"
#include "dhcam.h"

enum CameraType{
    HAIKANG_CAMERA = 1,
    DAHENG_CAMERA = 2
};

/**
 * author：lq
 * description：相机工厂构造类，利用Camera作为顶层接口对象，从而封装底层实现细节
 * **/
class CameraFactory{
public:
    //决定启动哪个相机
    static Camera* getCamera(CameraType type,int device_name)
    {
        switch (type)
        {
        case HAIKANG_CAMERA:
            /* code */
            return new HikCamera(device_name);
        case DAHENG_CAMERA:
            return new DhCamera(device_name);
        default:
            return nullptr;
        }
    }
};