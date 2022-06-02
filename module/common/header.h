#pragma once

#include <iostream>
#include <fstream>
#include <ostream>
#include <cctype>
#include <cstdio>

#include <thread>
#include <memory>
#include <future>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <vector>
#include <array>
#include <string>
#include <stack>
#include <queue>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <cmath>
#include <numeric>
#include <utility>
#include <random>
#include <chrono>
#include <algorithm>
#include <exception>

#if defined(_WIN32)
    #define U_OS_WINDOWS
    #include <Windows.h>
    #include <direct.h>
    #include <io.h>
    #include <process.h>
#else
#define U_OS_LINUX
    #include <dirent.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #include <pthread.h>
#endif
/***************************************** JSON ****************************************/
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
/***************************************** JSON ****************************************/
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/hourly_file_sink.h"
#include "spdlog/async.h"
#include <pthread.h>
/***************************************** UART ****************************************/
#include "serial/Serial.h"
/*************************************** PYTORCH ***************************************/
#include "torch/script.h"
#include "torch/torch.h"
/**************************************** OPENCV ***************************************/
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/cudawarping.hpp"
/**************************************** HIK CAM **************************************/
#include "MvCameraControl.h"
/*****************************************DH CAM*******************************************/
// #include "GxIAPI.h"
// #include "DxImageProc.h"

/**************************************** USR DEF **************************************/
using LL = long long;
using ULL = unsigned long long;
using PII = std::pair<int, int>;
using PIL = std::pair<int, LL>;
using PLL = std::pair<LL, LL>;
using VI = std::vector<int>;
/***************************************************************************************/

/****************************************** CFG ****************************************/
/*************************************** ERR CODE **************************************/
#define PT_OK 0x00000000

/********************* PTREATE *********************/
// 输入图像H
constexpr int IN_IMG_HEIGHT = 540;

// 输入图像W
constexpr int IN_IMG_WIDTH = 720;

// 起始特征到首个针格的偏移量
constexpr int Y_START_FIXED_OFFSET = 20;

// 机头步进的相对横移量(单位：像素)
constexpr int PER_STEP_RELATIVE_OFFSET = 55;

// 机头步进的估算针格数
constexpr int PER_STEP_CNT = 3;

// 光路有效区域边界
constexpr int VALID_REGION_COL_START = 170;
constexpr int VALID_REGION_COL_END = 550;

// LATERAL_Y_POS
// 后针排的零点(后针排与前针排对齐时候的MCU上报值)偏移(单位:列号)
constexpr int BACK_GRID_ZERO_OFFSET = 10;
/*************************************** ERR CODE **************************************/
#define PT_OK 0x00000000
// 后针排上报的y字段与真实图像的pix之间的比例系数(可能为float，暂定为int)
constexpr int BACK_GRID_CORRECTION_FACTOR = 3;

// SHUTTLE_POS
// S0系统相对A光路的偏移(单位:pix)
constexpr int SYS0_RELATIVE_A_OFFSET = 100;

// S1系统相对B光路的偏移(单位:pix)
constexpr int SYS1_RELATIVE_B_OFFSET = 100;
/********************* CAM *********************/

constexpr int CFG_CAM_CAM_MOUNT_COUNT = 2;

constexpr bool CFG_CAM_FRAME_RATE_CONTROL = false;

constexpr int CFG_CAM_ANTI_SHAKE_TIME = 20;

constexpr int CFG_CAM_GEV = 400;

constexpr float CFG_CAM_EXPOSURE_TIME = 150.0;

constexpr float CFG_CAM_GAIN = 2.0;

constexpr int CFG_CAM_CACHE_CAPACITY = 1000;
/********************* GPM *********************/
// 启动串口号
constexpr char *SERIAL_PORT_NAME = (char *)"/dev/ttyUSB0";

// 偏移表路径
constexpr char *OFFSET_TABLE_PATH = (char *)"../../../conf/table/offset_table.json";

// 模板路径
constexpr char *TEMPLATE_PATH = (char *)"../../../conf/template/JW2109066_XS_b.json";

// NN模型路径
constexpr char *MODEL_PATH = (char *)"../../../conf/model/single_channel_test.pt";

// 有效光路数量
constexpr int LIGHT_PATH_COUNT = 3;

// 有效光路开关
constexpr std::array<int, 1> VALID_LIGHT_PATH{2};

// 光路映射表
const std::vector<std::string> LIGHT_PATH_MAPPING{"A", "B", "C"};

// 有效区域映射表
const std::vector<std::string> VALID_REGION_MAPPING{"front_valid", "back_valid"};

const std::vector<std::string> NEEDLE_POS{"front_needle", "back_needle"};

// 单光路的Window
constexpr int WINDOW_XS_OFFSET = 80;
constexpr int WINDOW_XE_OFFSET = 120;
constexpr int WINDOW_Y0 = 175;
constexpr int WINDOW_Y1 = 315;

// 针格数量
constexpr int NEEDLE_GRID_COUNT = 628;

// 针排映射
constexpr int FRONT_NEEDLE = 0;
constexpr int BACK_NEEDLE = 1;

// 针格属性
constexpr int NEEDLE_GRID_WIDTH = 32;
constexpr int NEEDLE_GRID_HIGH = 96;

// 偏移属性
constexpr int RELATIVE_OFFSET = 55;

// 针格数量修正
constexpr int NEEDLE_INDEX_FIX = 1;
constexpr int ZERO_INDEX_FIX = 1;

// 免检索引始末
constexpr int NEEDLE_FREE_START = 35;
constexpr int NEEDLE_FREE_END = 590;

// 沙嘴遮挡屏蔽数(单边)
constexpr int YARN_SHADE_COUNT = 10;

// 后针排固定偏移
constexpr int BACK_NEEDLE_FIX_OFFSET = NEEDLE_GRID_WIDTH / 4;

// TODO:废纱行数(方案待定)(模板从机头运动开始，牵拉梳信号在废纱起始行开始，此时行号重置，后续比较都需要做废纱行修正)
constexpr int WASTE_YARN_GAP = 9;

// 定检阈值
constexpr int FOCUS_THRESHOLD = 800;
constexpr int BRIGHTNESS_UPPER_THRESHOLD = 180;
constexpr int BRIGHTNESS_LOWER_THRESHOLD = 100;

// 缺陷检测缺省值
constexpr int MAX_DEFECT_COUNT = 4;
constexpr int MAX_CONTINUOUS_DEFECT_COUNT = 2;

/********************* UART ********************/
// head & tail
constexpr unsigned char FRAME_HEAD_0 = 0x7E;
constexpr unsigned char FRAME_HEAD_1 = 0xA5;

constexpr unsigned char FRAME_TAIL_0 = 0x7E;
constexpr unsigned char FRAME_TAIL_1 = 0x0D;
// 网元ID
constexpr unsigned char MID_VIE = 0x10;
constexpr unsigned char MID_CAM = 0x11;
constexpr unsigned char MID_MCU = 0x12;
constexpr unsigned char MID_OAM = 0x13;
constexpr unsigned char MID_GPM = 0x14;

constexpr unsigned char MID_EMS = 0x30;

constexpr unsigned char MID_CMS = 0x50;

constexpr unsigned char MID_GMT = 0x70;

constexpr unsigned char UD_LATERAL_Y_POS = 0x06; // 横移机构位置信息
constexpr unsigned char UD_SHUTTLE_POS = 0x07;   // 梭子位置信息

/// MCU-OAM
// UD STATUS CMD
constexpr unsigned char M2O_UD_STATUS = 0x04;
// UD STATUS TYPE
constexpr unsigned char M2O_UD_STATUS_NEW_PIECE = 0x00;
constexpr unsigned char M2O_UD_STATUS_MCU_ALIGN_NK = 0x01;
constexpr unsigned char M2O_UD_STATUS_MCU_ALIGN_ING = 0x02;
constexpr unsigned char M2O_UD_STATUS_MCU_ALIGN_OK = 0x03;

// UD XY CMD by lq
constexpr unsigned char M2O_UD_XY = 0x05; // CMD坐标信息汇报指令

// UD Fault CMD created by wzy
constexpr unsigned char M2O_UD_Fault = 0x08;
constexpr unsigned char M2O_DU_Fault = 0x08;
// UD Fault Type created by wzy
constexpr unsigned char M2O_DU_Fault_Handle = 0x00;            //手柄故障
constexpr unsigned char M2O_DU_Fault_Traverse = 0x01;          //横移机构故障
constexpr unsigned char M2O_DU_Fault_Drawcomb_L = 0x02;        //牵拉梳上下传感器故障 最小值
constexpr unsigned char M2O_DU_Fault_Drawcomb_H = 0x03;        //牵拉梳上下传感器故障 最大值
constexpr unsigned char M2O_DU_Fault_Door_L = 0x04;            //机台左右门故障 最小值
constexpr unsigned char M2O_DU_Fault_Door_H = 0x05;            //机台左右门故障 最大值
constexpr unsigned char M2O_DU_Fault_Location_L = 0x06;        //前/后位置传感器故障 最小值
constexpr unsigned char M2O_DU_Fault_Location_H = 0x07;        //前/后位置传感器故障 最大值
constexpr unsigned char M2O_DU_Fault_Connection_L = 0x10;      //光源1~8接线故障 最小值
constexpr unsigned char M2O_DU_Fault_Connection_H = 0x17;      //光源1~8接线故障 最大值
constexpr unsigned char M2O_DU_Fault_Overcurrent_L = 0x20;     //光源1~8过流错误 最小值
constexpr unsigned char M2O_DU_Fault_Overcurrent_H = 0x27;     //光源1~8过流错误 最大值
constexpr unsigned char M2O_DU_Fault_Overtemperature_L = 0x30; //光源1~8过温错误 最小值
constexpr unsigned char M2O_DU_Fault_Overtemperature_H = 0x37; //光源1~8过温错误 最大值
constexpr unsigned char M2O_DU_Fault_Shuttle_L = 0x50;         //梭子0-7故障 最小值
constexpr unsigned char M2O_DU_Fault_Shuttle_H = 0x57;         //梭子0-7故障 最大值
constexpr unsigned char M2O_DU_Fault_Motor = 0x57;             //马达故障
constexpr unsigned char M2O_DU_Fault_Camera_L = 0x70;          //相机1-4故障 最小值
constexpr unsigned char M2O_DU_Fault_Camera_H = 0x73;          //相机1-4故障 最大值

// UD ALM CMD created by jzy
constexpr unsigned char M2O_UD_ALM_REP = 0x09;
// UD ALM TYPE created by jzy
constexpr unsigned char M2O_UD_ALM_REP_POWER_VOL = 0x00;      //主板+12V电源电压范围告警
constexpr unsigned char M2O_UD_ALM_REP_CAM_VOL = 0x01;        //相机电源电压范围告警
constexpr unsigned char M2O_UD_ALM_REP_LIGHT_VOL = 0x02;      //光源输入电源电压范围告警
constexpr unsigned char M2O_UD_ALM_REP_CURRENT_FLAT_L = 0x10; //光源1~8电流平坦度失衡告警 下限
constexpr unsigned char M2O_UD_ALM_REP_CURRENT_FLAT_H = 0x17; //光源1~8电流平坦度失衡告警 上限
constexpr unsigned char M2O_UD_ALM_REP_VOL_FLAT_L = 0x20;     //光源1~8电压平坦度失衡告警  下限
constexpr unsigned char M2O_UD_ALM_REP_VOL_FLAT_H = 0x27;     //光源1~8电压平坦度失衡告警  上限
constexpr unsigned char M2O_UD_ALM_REP_WIRE_BREAK = 0x30;     //针织线断线告警

// UD SET1 CMD created by wzy
constexpr unsigned char M2O_DU_SET1_New = 0x0A;
// UD SET2 CMD created by wzy
constexpr unsigned char M2O_DU_SET2_New = 0x0C;

// UD ALM CHECK_SET1 created by jzy
constexpr unsigned char M2O_DU_CHECK_SET1 = 0x0B;
// UD ALM CHECK_SET2 created by jzy
constexpr unsigned char M2O_DU_CHECK_SET2 = 0x0D;

// UD ALM CMD
constexpr unsigned char M2O_UD_ALM = 0x04;
constexpr unsigned char M2O_DU_ALM = 0x04;
// UD ALM TYPE
constexpr unsigned char M2O_DU_ALM_PWR = 0x00;
constexpr unsigned char M2O_DU_ALM_PAD_WIRE = 0x01;
constexpr unsigned char M2O_DU_ALM_ENCODER_WIRE = 0x02;
constexpr unsigned char M2O_DU_ALM_COMB_WIRE = 0x03;
constexpr unsigned char M2O_DU_ALM_CAM_WIRE = 0x04;
constexpr unsigned char M2O_DU_ALM_LIGHT_WIRE = 0x05;
constexpr unsigned char M2O_DU_ALM_CFE_MOTO = 0x06;

// UD SET1 CMD
constexpr unsigned char M2O_DU_SET1 = 0x11;

// UD OP1 CMD
constexpr unsigned char M2O_DU_OP1 = 0x12;

// UD OP2 CMD
constexpr unsigned char M2O_DU_OP2 = 0x13;

// UD SET3 CMD
constexpr unsigned char M2O_DU_SET3 = 0x14;

// UD OP4 CMD by zgs
constexpr unsigned char M2O_UD_OP4 = 0x0E;
// UD OP4 TYPE by zgs
constexpr unsigned char M2O_UD_OP4_NOT_REC_DA = 0x01;
constexpr unsigned char M2O_UD_OP4_CRC_FAIL = 0x02;
constexpr unsigned char M2O_UD_OP4_CHECK_FAIL_CNT = 0x03;
constexpr unsigned char M2O_UD_OP4_NOT_REC_HA = 0x04;
constexpr unsigned char M2O_UD_OP4_RESEND_D = 0x05;
constexpr unsigned char M2O_UD_OP4_PHOTO_CNT = 0x06;
constexpr unsigned char M2O_UD_OP4_XY_CNT = 0x07;

// UD OP6 CMD by zgs
constexpr unsigned char M2O_UD_OP6 = 0x0F;

// UD OP8 CMD by zgs
constexpr unsigned char M2O_UD_OP8 = 0x10;

// OP10 OP11 OP12 CMD by zs
constexpr unsigned char M2O_UD_OP10 = 0x0F;

// frame state
constexpr unsigned char H_FRAME = 0x00;
constexpr unsigned char D_FRAME = 0x01;
constexpr unsigned char HA_FRAME = 0x02;
constexpr unsigned char DA_FRAME = 0x03;

// direction
constexpr unsigned char R = 0x01;
constexpr unsigned char L = 0x00;
constexpr unsigned char TL = 0xA2;
constexpr unsigned char TR = 0xA3;

/***************************************************************************************/
/******************************************* DS ****************************************/
struct cam_img_data
{
    cv::Mat imgMat;
    uint64_t nFrameID;
    uint64_t nTimestamp;
};

// MACHINE_HEAD_XY机头位置信息
struct machine_head_xy
{
    unsigned char flag;       // 0xAA 正常 0xBB 预警
    unsigned int frame_num;   // 帧号
    unsigned char direction;  // 方向(1 or 0)
    unsigned short x;         // 行号
    short y;                  // 机头列坐标
    unsigned short timestamp; // 时间戳
    machine_head_xy() {}
    machine_head_xy(unsigned char flag, unsigned int frame_num, unsigned char direction, short x, short y, short timestamp)
    {
        this->flag = flag;
        this->frame_num = frame_num;
        this->direction = direction;
        this->x = x;
        this->y = x;
        this->timestamp = timestamp;
    }
};


// LATERAL_Y_POS横移结构位置信息
struct lateral_y_pos
{
    unsigned char cmd;
    unsigned char flag;       // 0xAA 正常 0xBB 预警
    unsigned char direction;  // 横移机构运动方向(1 or 0)
    short y;                  // 横移列坐标
    unsigned short timestamp; // 时间戳
    lateral_y_pos() {}
    lateral_y_pos(unsigned char cmd, unsigned char flag, unsigned char direction, short y, unsigned short timestamp)
    {
        this->cmd = cmd;
        this->flag = flag;
        this->direction = direction;
        this->y = y;
        this->timestamp = timestamp;
    }
};

// SHUTTLE_POS 梭子位置信息
struct shuttle_pos
{
    unsigned char cmd;
    unsigned short colNo;     // 参照文档进行解析
    unsigned char direction;  // 梭子运动方向(0 or 1)
    short pos;                // 梭子列坐标
    unsigned short timestamp; // 时间戳
    shuttle_pos() {}
    shuttle_pos(unsigned char cmd, unsigned short colNo, unsigned char direction, unsigned short pos, unsigned short timestamp)
    {
        this->cmd = cmd;
        this->colNo = colNo;
        this->direction = direction;
        this->pos = pos;
        this->timestamp = timestamp;
    }
};

// 预处理INPUT
struct pt_data_pack
{
    cv::Mat c_data;
    machine_head_xy u_data;
    pt_data_pack() {}
    pt_data_pack(cv::Mat c_data, machine_head_xy u_data)
    {
        this->c_data = c_data.clone();
        this->u_data = u_data;
    }
};

// 自校准数据结构
struct pt_calibration
{
    char light_path; //光路
    float angle;     //角度
    int mid_line;    //中线

    pt_calibration() {}
    pt_calibration(char light_path, float angle, int mid_line)
    {
        this->light_path = light_path;
        this->angle = angle;
        this->mid_line = mid_line;
    }
};

// 偏移表数据结构
struct pt_offset_table
{
    char light_path;
    std::unordered_map<int, std::vector<PII>> offset_table;
};

// 原始针格数据
struct pt_needle_data
{
    int row;
    int light_path;
    int needle_position;
    int start;
    int end;
    std::array<cv::Mat, NEEDLE_GRID_COUNT> needle_img;
};

// 自动监测数据
struct pt_auto_detect
{
    char light_path;
    bool pass;
    pt_auto_detect(){};
    pt_auto_detect(char light_path, bool res)
    {
        this->light_path = light_path;
        this->pass = res;
    }
};

struct nn_needle_ans_data
{
    int row;                                         // 行号 (织物行号与模板rowx对应)
    int light_path;                                  // 光路 (0 or 1 or 2分别对应A、B、C光路)
    int needle_position;                             // 前、后针排 (0:前,1:后)
    int start;                                       // 检测起始索引 (0 <= start < end <= NEEDLE_GRID_COUNT)
    int end;                                         // 检测结束索引 (0 <= start < end <= NEEDLE_GRID_COUNT)
    std::array<int, NEEDLE_GRID_COUNT> category;     // 神经网络预测结果
    std::array<float, NEEDLE_GRID_COUNT> confidence; // 神经网络预测置信度
    std::array<cv::Mat, NEEDLE_GRID_COUNT> cvimage;  // 针格图片
};

// 行号监测数据
struct monitor_pos
{
    unsigned char direction;
    short row;
    short col;
};

// 自定义模板状态数据
template <typename T>
struct g_data
{
    bool state;
    T data;
};

enum serial_port_status
{
    HEAD,
    SN,
    FT,

    DST,
    SRC,
    LEN,
    DATA,

    CRC16,
    TAIL,
};

enum module_except_code
{
    CAM_DEVICE_NOT_FOUND,
    CAM_DEVICE_INIT_FAILURE,
    CAM_RETRY_FAILURE,
    CAM_IMAGE_CALL_FAILURE,
    CAM_LOST_FRAME,
    AUTO_DECT_ERROR,
    EXCEPTION_WARNING,
    NORMAL_CODE,
    VISDECT_EXCEPTION,
};
/***************************************************************************************/
