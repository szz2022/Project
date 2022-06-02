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
            hardware_decode_yarn(yarn_pos);
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
        template_max_row++;
    }
    LOG_S("成功软件解析织物模板，最大织物行数:" + std::to_string(template_max_row), LOGGER_INFO);
}

void PtAllocator::hardware_decode_yarn(shuttle_pos pos_data)
{
    // pos_data.pos;
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
