/**
 * @file pt_allocator.cpp
 * @author lty (302702801@qq.com)
 * @brief
 * @version 0.1
 * @date 2022-05-09
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "pt_allocator.h"
#include "pt_logic.h"
#include "../common/utils.h"
#include "../optjson/offsettable.h"
#include "../optjson/templatejson.h"
#include "../operation/operation.h"
#include "../logger/logger.h"

#define LOG_S(...)    Logger::get_instance().log_plus((const std::string)(FILENAME(__FILE__)), (const std::string)(__FUNCTION__), (int)(__LINE__), __VA_ARGS__)

PtAllocator::PtAllocator()
{
    LOG_S("开始构造预处理分配器", LOGGER_INFO);
    LOG_S("成功构造预处理分配器", LOGGER_INFO);
}

void PtAllocator::run()
{
    LOG_S("执行预处理分配器", LOGGER_INFO);
    init();
    // 创建三个预处理对象
    PtAlpActuator = std::make_shared<PtLogic>(0, shared_from_this());
    PtBlpActuator = std::make_shared<PtLogic>(1, shared_from_this());
    PtClpActuator = std::make_shared<PtLogic>(2, shared_from_this());

    // 启动线程执行预处理run函数
    std::thread A_ex(&PtLogic::run, PtAlpActuator);
    std::thread B_ex(&PtLogic::run, PtBlpActuator);
    std::thread C_ex(&PtLogic::run, PtClpActuator);

    while (true)
    {
        std::unique_lock<std::mutex> pt_a_lock(pt_a_mutex);
        pt_a_condition.wait(pt_a_lock, [&]()
                            { return lateral_y_pos_queue->Size() || shuttle_pos_queue->Size(); });
        // 处理横移队列数据
        if (lateral_y_pos_queue->Size())
        {
            lateral_y_pos_queue->Pop(&back_offset, false);
            std::cout << "获取到[横移坐标]"
                      << "时间戳[" << back_offset.timestamp << "]" << std::endl;
            set_offset((back_offset.y - BACK_GRID_ZERO_OFFSET) * BACK_GRID_CORRECTION_FACTOR);
        }

        // 处理沙嘴队列数据
        if (shuttle_pos_queue->Size())
        {
            shuttle_pos_queue->Pop(&yarn_pos, false);
            std::cout << "获取到[沙嘴坐标]"
                      << "时间戳[" << yarn_pos.timestamp << "]" << std::endl;
            set_shuttle_pos(yarn_pos);
        }
    }
    A_ex.join();
    B_ex.join();
    C_ex.join();
    LOG_S("结束预处理分配器", LOGGER_INFO);
}

void PtAllocator::init()
{
    software_decode_yarn();
}

void PtAllocator::software_decode_yarn()
{
    LOG_S("开始软件解析织物模板", LOGGER_INFO);

    // 逐行
    for (auto rows_itr = templatejson::d.MemberBegin(); rows_itr != templatejson::d.MemberEnd(); ++rows_itr)
    {
        // 逐光路
        for (auto path_itr = rows_itr->value.MemberBegin(); path_itr != rows_itr->value.MemberEnd(); ++path_itr)
        {
            auto pyarn = path_itr->value.FindMember("yarn_position");
            auto pFVR = path_itr->value.FindMember("front_valid");
            auto pBVR = path_itr->value.FindMember("back_valid");

            if (pFVR != path_itr->value.MemberEnd())
            {
                for (auto &v : pFVR->value.GetArray())
                {
                    std::vector<int> temp;
                    for (auto &c : v.GetArray())
                        temp.push_back(c.GetInt());
                    valid_region[rows_itr->name.GetString()][path_itr->name.GetString()]["front_valid"].push_back(temp);
                }
            }
            if (pBVR != path_itr->value.MemberEnd())
            {
                for (auto &v : pBVR->value.GetArray())
                {
                    std::vector<int> temp;
                    for (auto &c : v.GetArray())
                        temp.push_back(c.GetInt());
                    valid_region[rows_itr->name.GetString()][path_itr->name.GetString()]["back_valid"].push_back(temp);
                }
            }
            if (pyarn != path_itr->value.MemberEnd())
            {
                for (auto position_itr = pyarn->value.MemberBegin(); position_itr != pyarn->value.MemberEnd(); ++position_itr)
                {
                    yarn_positon[rows_itr->name.GetString()][path_itr->name.GetString()].insert(position_itr->value.GetInt());
                    for (int i = 1; i <= YARN_SHADE_COUNT; i++)
                    {
                        yarn_positon[rows_itr->name.GetString()][path_itr->name.GetString()].insert(position_itr->value.GetInt() + i);
                        yarn_positon[rows_itr->name.GetString()][path_itr->name.GetString()].insert(position_itr->value.GetInt() - i);
                    }
                }
            }
        }
        row++;
    }
    // Resize yarn position vector
    yarn_position_vector = std::vector<std::vector<int>>(row, std::vector<int>(8, NEEDLE_GRID_COUNT));
    LOG_S("成功软件解析织物模板，最大织物行数:" + std::to_string(row), LOGGER_INFO);
}


int PtAllocator::get_row()
{
    return row;
}

void PtAllocator::set_row(int c)
{
    row.store(c);
}

int PtAllocator::get_offset()
{
    return offset;
}

void PtAllocator::set_offset(int shift)
{
    offset.store(shift);
}

std::vector<int> PtAllocator::get_shuttle_pos(int row)
{
    // C++14下只有shared_timed_mutex具体效率区别与C++17的shared_mutex待测试
    std::shared_lock<std::shared_timed_mutex> r_lock(shuttle_rw_mutex);
    std::vector<int> res;
    for (int i = 0; i < SHUTTLE_POS_CNT; i++)
        if (yarn_position_vector[row][i] < NEEDLE_GRID_COUNT && yarn_position_vector[row][i] > 0)
            res.emplace_back(yarn_position_vector[row][i]);
    return res;
}

void PtAllocator::set_shuttle_pos(shuttle_pos pos_data)
{
    std::unique_lock<std::shared_timed_mutex> w_lock(shuttle_rw_mutex);

    // 记录当前状态，查找状态变化
    unsigned char sys0_state_change = static_cast<unsigned char>(pos_data.colNo) ^ sys0_shuttle_state;
    unsigned char sys1_state_change = static_cast<unsigned char>(pos_data.colNo >> 8) ^ sys1_shuttle_state;
    sys0_shuttle_state = static_cast<unsigned char>(pos_data.colNo);
    sys1_shuttle_state = static_cast<unsigned char>(pos_data.colNo >> 8);

    // 
    for (int i = 0; i < SHUTTLE_POS_CNT; i++)
    {
        // S0系统的i列沙嘴发生状态改变
        if (sys0_state_change & 1)
        {
            // 拿起
            if (sys0_shuttle_state >> i & 1)
            {
                // auto itr = offsettable::d["A"]["table"].FindMember(std::to_string(get_dpos(pos_data)).c_str());
                // // 未找到当前位置(说明在有效区域外[0, 629])
                // if (itr == offsettable::d["A"]["table"].MemberEnd())
                //     // do something
                //     std::cout << "A" << std::endl;
                // else
                //     int ap_first_ycor = offsettable::offsettable::d[light_path]["table"][std::to_string(get_dpos(u_data)).c_str()].GetArray()
                // // 对应位置的第一个坐标
                // int ap_first_ycor = 100;
                // int ap_first_index = 10;

                // int offset_cnt = (S0_AP_OFFSET - ap_first_ycor) / NEEDLE_GRID_WIDTH;

                // int real_index = ap_first_index + offset_cnt;
                

                std::cout << "S0 " << to_string(i) << "号位置状态改变:拿起" << std::endl;

            }
            // 放下
            else
                std::cout << "S0 " << to_string(i) << "号位置状态改变:放下" << std::endl;
        }
        // S1系统的i列沙嘴发生状态改变
        if (sys1_state_change & 1)
        {
            // 拿起
            if (sys1_shuttle_state >> i & 1)
                std::cout << "S1 " << to_string(i) << "号位置状态改变:拿起" << std::endl;
            // 放下
            else
                std::cout << "S1 " << to_string(i) << "号位置状态改变:放下" << std::endl;
        }
        sys0_state_change >>= 1;
        sys1_state_change >>= 1;
    }

    short x = pos_data.x;
    short y = pos_data.y;
}
