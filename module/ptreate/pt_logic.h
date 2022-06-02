/**
 * @file pt_logic.h
 * @author lty (302702801@qq.com)
 * @brief
 * @version 0.1
 * @date 2022-04-27
 *
 * @copyright Copyright (c) 2022
 *
 */

#pragma once

#include "../common/header.h"
#include "../operation/operation.h"
#include "pt_allocator.h"

class PtLogic
{
private:
	// 验证层上报预处理数据
	pt_data_pack gdata;

	// 指向PtAllocator的指针，构造的时候会传对象指针
	std::shared_ptr<PtAllocator> ptr_allocator;

	// 内部执行计数器
	long long inner_cnt = 0;

	// 当前光路的标志
	int light_path;

	// 制作偏移表的枚举状态
	enum inner_state
	{
		S_WAIT,
		S_FINDING,                                                                           
		S_DOING,
		S_SEND,
		S_IDLE
	};

	inner_state mk_table_state = S_WAIT;

	pt_calibration calibration_data;
	pt_offset_table offset_table_data;
	pt_needle_data front_needle_data;
	pt_needle_data back_needle_data;

	// 位置监测变量
	monitor_pos cur_pos;
	monitor_pos pre_pos;

	// 当前行号
	int row_count = 0;
	
	// 光路映射表
	std::map<int, std::string> LIGHT_PATH_MAPPING{{0, "A"}, {1, "B"}, {2, "C"}};

	// 换行标志
	bool row_changed_flag = false;

	// y轴坐标
	int y_coordinate = -1;
	
	// 针格索引
	int needle_index = 0;

	// 特征区域位置集合
	std::unordered_set<int> feature_position;

	// Split 修正错误记录
	int split_error_cnt = 0;

	/**
	 * @brief Get the direction object
	 *
	 * @param direction 串口发送的机头运行方向
	 * @return int
	 */
	inline int get_direction(unsigned char direction)
	{
		return direction ? 1 : -1;
	}

	/**
	 * @brief Get the direction pos object
	 *
	 * @param u_data 串口Machine Head XY数据
	 * @return int
	 */
	inline int get_dpos(machine_head_xy u_data)
	{
		return (static_cast<int>(u_data.direction) << 12) & static_cast<int>(u_data.y);
	}

public:
	PtLogic() = default;
	~PtLogic() = default;

	PtLogic(int l_p, std::shared_ptr<PtAllocator> ptr_a);
	/**
	 * @brief 线程入口函数
	 *
	 */
	void run();
	//

	/**
	 * @brief 线程内部初始化
	 *
	 */
	void init();

	// Function API
	/**
	 * @brief
	 *
	 * @param u_data 串口Machine Head XY数据
	 * @return true
	 * @return false
	 */
	bool check_row_valid(machine_head_xy u_data);
	
	/**
	 * @brief 检查当前线程处理的光路是否是有效的
	 * @details 通过读衣物模板.json来判断当前处理行的当前光路是否有效
	 * 
	 * @param row_cnt 
	 * @return true 有效
	 * @return false 无效
	 */
	bool check_light_path_valid(int row_cnt);

	/**
	 * @brief 检查当前列的图片是否在有效的截取区域中
	 * @details 通过读取偏移表.json来判断当前图像是否在工作区域中
	 *
	 * @param u_data 串口Machine Head XY数据
	 * @return true 有效
	 * @return false 无效
	 */
	bool check_col_valid(machine_head_xy u_data);

	/**
	 * @brief 拟合针格边缘
	 * @details 通过比较真实检测出的边缘位置与理论边缘位置比对来获取最终的边缘结果
	 * (防止针格断裂无法检测出边缘的情况、同时保证每个针格都按照真实坐标截取)
	 *
	 * @param r_edges
	 * @param direction
	 * @param x_start_cor
	 * @param split_cnt
	 * @return VI
	 */
	VI fit_edge(const VI &r_edges, unsigned char direction, int x_start_cor, int split_cnt);

	/**
	 * @brief 硬件方案获取后针排横移
	 *
	 * @return int 相对前针排偏移的像素距离
	 */
	int cal_back_offset_hardware();

	/**
	 * @brief 软件方案获取后针排横移
	 *
	 * @return int 相对前针排偏移的像素距离
	 */
	int cal_back_offset_software();

	/**
	 * @brief 获取当前列截取针格的数量
	 * @details 通过偏移表中对应串口位置来获取该位置需要截取的针格数量
	 *
	 * @return int
	 */
	int get_cur_split_cnt(machine_head_xy u_data);

	/**
	 * @brief 从数据中获取node节点的value(暂定接口，后期具体实现)
	 *
	 * @tparam T
	 * @param s_node
	 * @return T
	 */
	template <typename T>
	T get_node_value(std::string s_node)
	{
		T ans;
		return ans;
	}

	/**
	 * @brief 监控串口数据，提取有效的换行、行号、方向信息供应用层使用
	 *
	 * @param u_data 串口Machine Head XY数据
	 */
	void row_monitor(machine_head_xy u_data);

	// Custom image business logic
	/**
	 * @brief 自动校正执行函数
	 * @details 自动对输入的图片进行角度与中线校准
	 *
	 * @param src 相机采集的图像cv::Mat数据
	 */
	void auto_calibration(cv::Mat &src);

	/**
	 * @brief 制作偏移表执行函数
	 * @details 在机头运行过程中制作偏移表
	 *
	 * @param src 相机采集的图像cv::Mat数据
	 * @param u_data 串口Machine Head XY数据
	 */
	void make_offset_table(cv::Mat &src, machine_head_xy u_data);

	/**
	 * @brief 截取针格执行函数
	 * @details  统一方向为：-1：左，1：右(硬件不改，软件作一次修正)
	 *
	 * @param src 相机采集的图像cv::Mat数据
	 * @param u_data 串口Machine Head XY数据
	 */
	void split_image(cv::Mat &src, machine_head_xy u_data);

	/**
	 * @brief 自动监测执行函数
	 * @details 只在预先规定好的特征块位置出监测亮度和清晰度
	 *
	 * @param src 相机采集的图像cv::Mat数据
	 * @param u_data 串口Machine Head XY数据
	 */
	void auto_dect(cv::Mat &src, machine_head_xy u_data);
};
