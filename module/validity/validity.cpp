#include "validity.h"


bool validity::check_validity()
{
    machine_head_xy calibrate;
    cam_img_data p0;
    cam_img_data p1;
    cam_img_data p2;
    machine_head_xy_queue->Pop(&calibrate, true); //获取串口帧
    camera_queue0->Pop(&p0, true);                //获取图像帧
    camera_queue1->Pop(&p1, true);                 //获取图像帧
    camera_queue2->Pop(&p2, true);                 //获取图像帧

    uint64_t tiStampCalibrate = calibrate.timestamp;           //获取串口帧的时间戳，单位1us
    uint64_t tiStampCalibrate_ns = calibrate.timestamp * 1000; //统一串口帧的时间戳，单位1ns

    uint64_t timeStamp0_ns = p0.nTimestamp; //获取图像帧的时间戳，单位1ns
    uint64_t timeStamp1_ns = p1.nTimestamp; //获取图像帧的时间戳，单位1ns
    uint64_t timeStamp2_ns = p2.nTimestamp; //获取图像帧的时间戳，单位1ns

    uint64_t calibrate_frame_num = calibrate.frame_num; //获取串口帧帧号
    uint64_t p0_FrameNum = p0.nFrameID;                 //获取图像帧帧号
    uint64_t p1_FrameNum = p1.nFrameID;                 //获取图像帧帧号
    uint64_t p2_FrameNum = p2.nFrameID;                 //获取图像帧帧号

    uint64_t threshold_value = 500000000000; //设置时间戳阈值为5ms，即500000ns

    while (1)
    {
        // if (calibrate_frame_num == p0_FrameNum && p0_FrameNum == p1_FrameNum && p1_FrameNum == p2_FrameNum){
        std::string msg = "比对逻辑after--------相机0帧号：" + std::to_string(p0_FrameNum) + " 相机1帧号：" + std::to_string(p1_FrameNum) + +" 串口帧号：" + std::to_string(calibrate_frame_num);
        LOG_PLUS(msg, LOGGER_INFO);
        if (calibrate_frame_num == p0_FrameNum && p0_FrameNum == p1_FrameNum)
        {
            //如果四个帧好相等,判断时间戳的差值是否小于阈值
            if (tiStampCalibrate_ns - timeStamp0_ns < threshold_value || timeStamp0_ns - tiStampCalibrate_ns < threshold_value)
            {
                //如果时间戳的插值，小于阈值，那么将串口帧数据与每帧图像数据组合起来发给后续线程
                //发给预处理线程
                ptreate_queue0->Push(pt_data_pack(p0.imgMat, calibrate));
                ptreate_queue1->Push(pt_data_pack(p1.imgMat, calibrate));
                ptreate_queue2->Push(pt_data_pack(p2.imgMat, calibrate));
            }
            else
            {
                //如果时间戳的插值，大于阈值，那么写入debug日志
                LOG_SOCKET("帧号一致,等于" + to_string(calibrate_frame_num) + "，但时间戳插值超出阈值", LOGGER_ALARM);
            }

            if (tiStampCalibrate_ns - timeStamp1_ns < threshold_value || timeStamp1_ns - tiStampCalibrate_ns < threshold_value)
            {
                //如果时间戳的插值，小于阈值，那么将串口帧数据与每帧图像数据组合起来发给后续线程
                //发给预处理线程
            }
            else
            {
                //如果时间戳的插值，大于阈值，那么写入debug日志
                LOG_SOCKET("帧号一致,等于" + to_string(calibrate_frame_num) + "，但时间戳插值超出阈值", LOGGER_ALARM);
            }

            if (tiStampCalibrate_ns - timeStamp2_ns < threshold_value || timeStamp2_ns - tiStampCalibrate_ns < threshold_value)
            {
                //如果时间戳的插值，小于阈值，那么将串口帧数据与每帧图像数据组合起来发给后续线程
                //发给预处理线程
            }
            else
            {
                //如果时间戳的插值，大于阈值，那么写入debug日志
                LOG_SOCKET("帧号一致,等于" + to_string(calibrate_frame_num) + "，但时间戳插值超出阈值", LOGGER_ALARM);
            }

            machine_head_xy_queue->Pop(&calibrate, true); //获取串口帧
            camera_queue0->Pop(&p0, true);                //获取图像帧
            camera_queue1->Pop(&p1, true);                 //获取图像帧
            camera_queue2->Pop(&p2, true);                 //获取图像帧

            calibrate_frame_num = calibrate.frame_num; //获取串口帧帧号
            p0_FrameNum = p0.nFrameID;                 //获取图像帧帧号
            p1_FrameNum = p1.nFrameID;                 //获取图像帧帧号
            p2_FrameNum = p2.nFrameID;                 //获取图像帧帧号
        }
        else
        {
            //如果四个帧号不一致，调用函数，判断最小的帧是哪个。
            int min_frame_num = compare_frame_num(calibrate_frame_num, p0_FrameNum, p1_FrameNum, p2_FrameNum);
            if (min_frame_num == calibrate_frame_num)
            {
                // LOG("串口帧号最小，为"+to_string(min_frame_num)+"，获取下一帧串口数据",LOGGER_ALARM);
                machine_head_xy_queue->Pop(&calibrate, true); //获取下串口帧
                calibrate_frame_num = calibrate.frame_num;    //获取新的串口帧帧号
            }
            else if (min_frame_num == p0_FrameNum)
            {
                //如果最小的帧号是p0数据帧
                // LOG("图像帧号最小，为"+to_string(min_frame_num)+"，获取下一帧图像数据",LOGGER_ALARM);
                camera_queue0->Pop(&p0, true);      //获取图像帧
                p0_FrameNum = p0.nFrameID; //获取图像帧帧号
                timeStamp0_ns = p0.nTimestamp;      //获取图像帧的时间戳，单位1ns
            }
            else if (min_frame_num == p1_FrameNum)
            {
                //如果最小的帧号是p1
                // LOG("图像帧号最小，为"+to_string(min_frame_num)+"，获取下一帧图像数据",LOGGER_ALARM);
                camera_queue1->Pop(&p1, true);      //获取图像帧
                p1_FrameNum = p1.nFrameID; //获取图像帧帧号
                timeStamp1_ns = p1.nTimestamp;      //获取图像帧的时间戳，单位10ns
            }
            else
            {
                //如果最小的帧号是p2
                // LOG("图像帧号最小，为"+to_string(min_frame_num)+"，获取下一帧图像数据",LOGGER_ALARM);
                camera_queue2->Pop(&p2, true);      //获取图像帧
                p2_FrameNum = p2.nFrameID; //获取图像帧帧号
                timeStamp2_ns = p2.nTimestamp;      //获取图像帧的时间戳，单位10ns
            }
        }

        msg = "比对逻辑after--------相机0帧号：" + std::to_string(p0_FrameNum) + " 相机1帧号：" + std::to_string(p1_FrameNum) + +" 串口帧号：" + std::to_string(calibrate_frame_num);
        LOG_PLUS(msg, LOGGER_INFO);
    }
}
int validity::compare_frame_num(int a, int b, int c, int d)
{
    if (b < a) a = b;
    if (c < a) a = c;
    if (d < a) a = d;
    return a;
}

validity::validity(/* args */)
{
}

validity::~validity()
{
}
