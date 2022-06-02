/**
 * @file PtAllocator.h
 * @author lty (302702801@qq.com)
 * @brief
 * @version 0.1
 * @date 2022-05-09
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include "../common/header.h"

class PtLogic;
/**
 * @brief 用于监测横移、沙嘴位置、同时启动预处理线程
 *
 */
class PtAllocator : public std::enable_shared_from_this<PtAllocator>
{
private:
    // 衣物模板总行数
    std::atomic_int row{0};
    // 后针排偏移量(单位:->pix)
    std::atomic_int offset{0};

    lateral_y_pos back_offset;
    shuttle_pos yarn_pos;

    char system0_state;
    char system1_state;

public:
    PtAllocator();
    ~PtAllocator() = default;

    std::mutex pt_a_mutex;
    std::condition_variable pt_a_condition;

    std::shared_ptr<PtLogic> PtAlpActuator;
    std::shared_ptr<PtLogic> PtBlpActuator;
    std::shared_ptr<PtLogic> PtClpActuator;

    // Decode yarn
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_set<int>>> yarn_positon;
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::vector<int>>>>> valid_region;

    // template row
    int template_max_row = 0;

    /**
     * @brief
     *
     */
    void run();

    /**
     * @brief
     *
     */
    void init();

    /**
     * @brief  读取Json文件的Valid region 与 Yarn position存放在pubilc变量中供其他类来读取
     *
     */
    void software_decode_yarn();

    /**
     * @brief
     *
     * @param pos_data
     */
    void hardware_decode_yarn(shuttle_pos pos_data);

    /**
     * @brief Get the row object
     *
     * @return int
     */
    int get_row();

    /**
     * @brief Set the row object
     *
     * @param c
     */
    void set_row(int c);

    /**
     * @brief Get the offset object
     *
     * @return int
     */
    int get_offset();

    /**
     * @brief Set the offset object
     *
     * @param shift
     */
    void set_offset(int shift);
};
    // PtAllocator(std::shared_ptr<templatejson> ptr_ctj, std::shared_ptr<offsettable> ptr_otj);
