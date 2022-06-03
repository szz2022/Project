#include "validity.h"
// extern ConcurrentQueue<uart_group_data> *machine_head_xy_queue; // 存放machine_head_xy的队列
// extern ConcurrentQueue<MV_FRAME_OUT_INFO_EX> camera_queue0;
// extern ConcurrentQueue<MV_FRAME_OUT_INFO_EX> camera_queue1;
// extern ConcurrentQueue<MV_FRAME_OUT_INFO_EX> camera_queue2;
// extern ConcurrentQueue<MV_FRAME_OUT_INFO_EX> camera_queue3;
bool validity::check_validity()
{
    machine_head_xy calibrate;
    cam_img_data p0;
    cam_img_data p1;
    cam_img_data p2;
    machine_head_xy_queue->Pop(&calibrate, true); //获取串口帧
    camera_queue0->Pop(&p0, true);                 //获取图像帧
    camera_queue1->Pop(&p1, true);                 //获取图像帧
    camera_queue2->Pop(&p2, true);                 //获取图像帧

    uint64_t tiStampCalibrate = calibrate.timestamp;//获取串口帧的时间戳，单位100us
    uint64_t tiStampCalibrate_ns = calibrate.timestamp*100000;//统一串口帧的时间戳，单位1ns

    uint64_t timeStamp0_ns = p0.nTimestamp; //获取图像帧的时间戳，单位1ns
    uint64_t timeStamp1_ns = p1.nTimestamp; //获取图像帧的时间戳，单位1ns
    uint64_t timeStamp2_ns = p2.nTimestamp; //获取图像帧的时间戳，单位1ns


    uint64_t p0_timeStamp_diff = p0.nTimestamp - calibrate.timestamp;//获取相机帧首帧和串口首帧时间戳差值
    uint64_t p1_timeStamp_diff = p1.nTimestamp - calibrate.timestamp;//获取相机帧首帧和串口首帧时间戳差值
    uint64_t p2_timeStamp_diff = p2.nTimestamp - calibrate.timestamp;//获取相机帧首帧和串口首帧时间戳差值
    
    uint64_t calibrate_frame_num = calibrate.frame_num - 1; //获取串口帧帧号
    uint64_t p0_FrameNum = p0.nFrameID;                //获取图像帧帧号
    uint64_t p1_FrameNum = p1.nFrameID;                //获取图像帧帧号
    uint64_t p2_FrameNum = p2.nFrameID;                //获取图像帧帧号

    uint64_t threshold_value=500000;    //设置时间戳阈值为5ms，即500000ns
    uint64_t calibrate_over_flow_offset = 0;//用于存储串口帧时间戳偏移量

    while (1)
    {
        
        if (calibrate_frame_num == p0_FrameNum && p0_FrameNum == p1_FrameNum && p1_FrameNum == p2_FrameNum){
            //如果四个帧好相等,判断时间戳的差值是否小于阈值
            uint64_t real_diff0 = tiStampCalibrate_ns - timeStamp0_ns + p0_timeStamp_diff;
            uint64_t real_diff1 = tiStampCalibrate_ns - timeStamp1_ns + p1_timeStamp_diff;
            uint64_t real_diff2 = tiStampCalibrate_ns - timeStamp2_ns + p2_timeStamp_diff;

            ptreate_queue0->Push(pt_data_pack(p0.imgMat, calibrate));
            ptreate_queue1->Push(pt_data_pack(p1.imgMat, calibrate));
            ptreate_queue2->Push(pt_data_pack(p2.imgMat, calibrate));

            if (real_diff0 < threshold_value || 0 - real_diff0 < threshold_value){
                //如果时间戳的插值，小于阈值，那么将串口帧数据与每帧图像数据组合起来发给后续线程
                //发给预处理线程
            }else{
                //如果时间戳的插值，大于阈值，那么写入debug日志
                LOG_PLUS("1号相机和串口帧号一致,等于"+to_string(calibrate_frame_num)+"，但时间戳差值超出阈值",LOGGER_INFO);
            }

            if (real_diff1 < threshold_value || 0 - real_diff1 < threshold_value){
                //如果时间戳的插值，小于阈值，那么将串口帧数据与每帧图像数据组合起来发给后续线程
                //发给预处理线程
            }else{
                //如果时间戳的插值，大于阈值，那么写入debug日志
                LOG_PLUS("2号相机和串口帧号一致,等于"+to_string(calibrate_frame_num)+"，但时间戳差值超出阈值",LOGGER_INFO);
            }

            if (real_diff2 < threshold_value || 0 - real_diff2 < threshold_value){
                //如果时间戳的插值，小于阈值，那么将串口帧数据与每帧图像数据组合起来发给后续线程
                //发给预处理线程
            }else{
                //如果时间戳的插值，大于阈值，那么写入debug日志
                LOG_PLUS("3号相机和串口帧号一致,等于"+to_string(calibrate_frame_num)+"，但时间戳差值超出阈值",LOGGER_INFO);
            }


            {
                int size_of_queue[] = { machine_head_xy_queue->Size(), camera_queue0->Size(), camera_queue1->Size(), camera_queue2->Size() };
                int max_size = size_of_queue[0];
                int min_size = size_of_queue[0];
                LOG_PLUS("串口队列长度:"+ to_string(size_of_queue[0]) +
                        " 相机1队列长度:"+ to_string(size_of_queue[1]) +
                        " 相机2队列长度:"+ to_string(size_of_queue[2]) +
                        " 相机3队列长度:"+ to_string(size_of_queue[3]), LOGGER_INFO);

                string res = " , 串口队列长度: " + to_string(size_of_queue[0]) + " ;相机队列长度: ";
                const int max_diff = 3; 
                for (int i = 1; i < sizeof(size_of_queue)/sizeof(int); i++) {
                    res += to_string(size_of_queue[i])+",";
                    if (size_of_queue[i] > max_size) max_size = size_of_queue[i];
                    else if (size_of_queue[i] < min_size) min_size = size_of_queue[i];
                }
                res.pop_back();
                if (max_size - min_size >  max_diff) {
                        LOG_PLUS("串口队列与相机队列长度差值超过 "+ to_string(max_diff) + res, LOGGER_ERROR);
                        //TO DO:
                        except_code_queue->Push(CAM_RETRY_FAILURE);
                        event_number_queue->Push(E8);//触发其他模块异常事件
                        break;
                }
            }
            

            machine_head_xy_queue->Pop(&calibrate, true); //获取串口帧
            camera_queue0->Pop(&p0, true);                 //获取图像帧
            camera_queue1->Pop(&p1, true);                 //获取图像帧
            camera_queue2->Pop(&p2, true);                 //获取图像帧
            calibrate_frame_num = calibrate.frame_num - 1; //获取串口帧帧号
            p0_FrameNum = p0.nFrameID;                //获取图像帧帧号
            p1_FrameNum = p1.nFrameID;                //获取图像帧帧号
            p2_FrameNum = p2.nFrameID;                //获取图像帧帧号
            
            tiStampCalibrate_ns = calibrate.timestamp*100000;//统一串口帧的时间戳，单位1ns
            if (tiStampCalibrate_ns == 0) 
            {
                calibrate_over_flow_offset += 65536*100000;
            }
            tiStampCalibrate_ns += calibrate_over_flow_offset;

            timeStamp0_ns = p0.nTimestamp; //获取图像帧的时间戳，单位1ns
            timeStamp1_ns = p1.nTimestamp; //获取图像帧的时间戳，单位1ns
            timeStamp2_ns = p2.nTimestamp; //获取图像帧的时间戳，单位1ns
        }
        else
        {
            //如果四个帧号不一致，调用函数，判断最小的帧是哪个。
            int min_frame_num = compare_frame_num(calibrate_frame_num, p0_FrameNum, p1_FrameNum, p2_FrameNum);
            if (min_frame_num == calibrate_frame_num)
            {
                LOG_PLUS("串口帧号最小，为"+to_string(min_frame_num)+"，获取下一帧串口数据",LOGGER_ALARM);
                machine_head_xy_queue->Pop(&calibrate, true); //获取下串口帧
                calibrate_frame_num = calibrate.frame_num - 1;    //获取新的串口帧帧号
                tiStampCalibrate_ns = calibrate.timestamp*100000;//统一串口帧的时间戳，单位1ns
                if (tiStampCalibrate_ns == 0) 
                {
                    calibrate_over_flow_offset += 65536*100000;
                }
                tiStampCalibrate_ns += calibrate_over_flow_offset;
            }
            else if (min_frame_num == p0_FrameNum)
            {
                //如果最小的帧号是p0数据帧
                LOG_PLUS("图像帧号最小，为"+to_string(min_frame_num)+"，获取下一帧图像数据",LOGGER_ALARM);
                camera_queue0->Pop(&p0, true); //获取图像帧
                p0_FrameNum = p0.nFrameID;//获取图像帧帧号
                timeStamp0_ns = p0.nTimestamp; //获取图像帧的时间戳，单位1ns
            }
            else if (min_frame_num == p1_FrameNum)
            {
                //如果最小的帧号是p1
                LOG_PLUS("图像帧号最小，为"+to_string(min_frame_num)+"，获取下一帧图像数据",LOGGER_ALARM);
                camera_queue1->Pop(&p1, true); //获取图像帧
                p1_FrameNum = p1.nFrameID;//获取图像帧帧号
                timeStamp1_ns = p1.nTimestamp; //获取图像帧的时间戳，单位1ns
            }
            else
            {
                //如果最小的帧号是p2
                LOG_PLUS("图像帧号最小，为"+to_string(min_frame_num)+"，获取下一帧图像数据",LOGGER_ALARM);
                camera_queue2->Pop(&p2, true); //获取图像帧
                p2_FrameNum = p2.nFrameID;//获取图像帧帧号
                timeStamp2_ns = p2.nTimestamp; //获取图像帧的时间戳，单位1ns
            }
        }
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