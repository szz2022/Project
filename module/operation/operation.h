#ifndef OPERATION_H
#define OPERATION_H

// #include <sys/io.h>
#include "../msgqueue/mqtest.h"
#include "../common/header.h"
#include "../common/utils.h"
#include "../optjson/templatejson.h"
#include "../logger/logger.h"
#include "ThreadPool.h"
#include "../json/json.h"

//声明状态机的各个状态
enum StateLevel
{
    IDLE = 0,
    AUTO_CALIBRATE = 1,
    GENTABLE = 2,
    VISDECT = 3,
    AUTO_DETECT = 4,
    ERR_MODE = 5
};

//声明状态机的各个状态
enum EventNumber
{
    E0 = 0,
    E1 = 1,   //首次启动软件
    E2 = 2,   //串口上报UD_status的mcu已对齐
    E3 = 3,   //串口上报UD_status的开启新织片
    E4 = 4,   //自校准处理完成
    E5 = 5,   //偏移表制作完成
    E6 = 6,   //衣物质检完成
    E7 = 7,   //自动检测完成
    E8 = 8,   //其他模块异常
    E9 = 9,   //自校准异常
    E10 = 10, //偏移表制作异常
    E11 = 11, //衣物质检异常
    E12 = 12, //自动检测异常
    E13 = 13, //异常消除
    E14 = 14  // MCU上报机台异常
};

//定义一个全局状态
extern StateLevel level;

//定义一个全局计数变量
extern int sums;


// 定义三个全局自动检测光路成功标志
extern bool auto_dect_a;
extern bool auto_dect_b;
extern bool auto_dect_c;
//记录自动检测出错光路
extern std::string auto_dect_err_name; 


// 定义一个全局自动检测异常计数器
extern int auto_dect_err_cnt;

// 预处理执行器->业务逻辑 队列
extern ConcurrentQueue<pt_calibration> *calibrate_queue;
extern ConcurrentQueue<pt_offset_table> *offset_table_queue;
extern ConcurrentQueue<pt_auto_detect> *auto_detect_queue;

// 预处理执行器->神经网络 队列
extern ConcurrentQueue<pt_needle_data> *needle_queue;

// 神经网络->业务逻辑 队列
extern ConcurrentQueue<nn_needle_ans_data> *needle_ans_queue;

// 相机->校验层 队列
extern ConcurrentQueue<cam_img_data> *camera_queue0;
extern ConcurrentQueue<cam_img_data> *camera_queue1;
extern ConcurrentQueue<cam_img_data> *camera_queue2;
extern ConcurrentQueue<cam_img_data> *camera_queue3;

// 串口->校验层 队列
extern ConcurrentQueue<machine_head_xy> *machine_head_xy_queue;

// 串口->预处理分配器 队列
extern ConcurrentQueue<lateral_y_pos> *lateral_y_pos_queue;
extern ConcurrentQueue<shuttle_pos> *shuttle_pos_queue;

// 校验层->预处理执行器 队列
extern ConcurrentQueue<pt_data_pack> *ptreate_queue0;
extern ConcurrentQueue<pt_data_pack> *ptreate_queue1;
extern ConcurrentQueue<pt_data_pack> *ptreate_queue2;

// 预处理队列vector
extern std::vector<ConcurrentQueue<pt_data_pack> *> pt_queue_set;

//事件表队列，业务逻辑根据这个来判断
extern ConcurrentQueue<EventNumber> *event_number_queue;

//当状态为ERR_MODE时需要处理各模块的异常情况
extern ConcurrentQueue<module_except_code> *except_code_queue;

//上锁
void set_level(StateLevel prev_level, StateLevel next_level);

void handle();

#endif
