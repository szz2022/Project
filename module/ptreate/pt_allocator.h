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

    // S0沙嘴状态
    unsigned char sys0_shuttle_state{0};

    // S1沙嘴状态
    unsigned char sys1_shuttle_state{0};

    // 预创建后针排数据
    lateral_y_pos back_offset;
    
    // 预创建沙嘴位置数据
    shuttle_pos yarn_pos;

    // 沙嘴位置vector
    std::vector<std::vector<int>> yarn_position_vector;

    // Decode yarn
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_set<int>>> yarn_positon;
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::vector<int>>>>> valid_region;

    // 沙嘴读写锁
    std::shared_timed_mutex shuttle_rw_mutex;


    /**
	 * @brief Get the direction pos object
	 *
	 * @param shuttle_data 串口shuttle_pos数据
	 * @return int
	 */
	inline int get_dpos(shuttle_pos shuttle_data)
	{
		return (static_cast<int>(shuttle_data.direction) << 12) & static_cast<int>(shuttle_data.y);
	}

public:
    PtAllocator();
    ~PtAllocator() = default;


    std::mutex pt_a_mutex;
    std::condition_variable pt_a_condition;

    std::shared_ptr<PtLogic> PtAlpActuator;
    std::shared_ptr<PtLogic> PtBlpActuator;
    std::shared_ptr<PtLogic> PtClpActuator;


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
     * @brief 获取当前衣物的最大行号
     *
     * @return int
     */
    int get_row();

    /**
     * @brief 设置当前衣物的最大行号
     *
     * @param c
     */
    void set_row(int c);

    /**
     * @brief 获取当前后针排横移距离(PIX)
     *
     * @return int
     */
    int get_offset();

    /**
     * @brief 设置当前后针排横移距离(PIX)
     *
     * @param shift
     */
    void set_offset(int shift);

    /**
     * @brief Get the shuttle pos object
     * 
     * @param row 当前监测的行号
     * @return std::vector<int> 返回沙嘴所在行的针格索引位置
     */
    std::vector<int> get_shuttle_pos(int row);

    void set_shuttle_pos(shuttle_pos pos_data);


};
