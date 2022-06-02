#pragma once

#include "proto.h"
#include "uds.h"
#include "json.h"
#include "cthead.h"
#include "utils.h"


// TODO:后期printf_s都通过log模块输出到日志中
class ptreate : public CThread
{
public:
	/********************** UD *********************/
	// 原始数据
	cam_group_data cam_data;
	uart_group_data uart_data;

	// 数据跳变监测变量
	unsigned int pre_cam_frame = 0;
	unsigned int pre_uart_frame = 0;

	// 丢帧统计数
	unsigned int drop_cam_frame_count = 0;
	unsigned int drop_uart_frame_count = 0;
	
	// 预先开辟的预测数据队列
	needle_data A_FRONT;
	needle_data A_BACK;

	needle_data B_FRONT;
	needle_data B_BACK;

	needle_data C_FRONT;
	needle_data C_BACK;

	std::vector<std::vector<needle_data>> needle_set = { {A_FRONT, A_BACK}, {B_FRONT, B_BACK}, {C_FRONT, C_BACK} };

	// 预定义前后针格区域
	cv::Mat front_cropped_region;
	cv::Mat back_cropped_region;

	// 预定义针格数据
	needle_ans_data detected_res;

	// 缺陷统计
	int total_defect_count = 0;
	int total_continuous_defect_count = 0;
	bool continuous_flag = false;

	// 偏移表存在状态
	bool offset_table_exist_status = false;

	// 模板存在状态
	bool template_exist_status = false;

	// 状态标志位
	int gpm_cur_status = calibrate;

	// 检测行号数据
	int template_max_row = 0;
	int _row_count = -2;
	bool row_changed_flag = false;
	position _last_position;
	position _cur_position;
	
	// 偏移表制作起始标志位
	bool offset_table_start_flag = true;

	// 制作偏移表数据
	int _start_x = -1;
	int _shift_count = 1;
	int _stitch_index = 0;
	bool _valid_flag = false;

	// 记录数据用于Debug
	int real_edge_index = 0;
	int real_edge = 0;

	// offset table
	Document _offset_table;

	// template
	Document _clothes_template;

	// Decode yarn
	std::unordered_map<std::string, std::unordered_map<std::string ,std::unordered_set<int>>> yarn_positon;
	std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::vector<int>>>>> valid_region;

	// mem data
	mult_light_path SD;

	// virtual test
	int virtual_test_count = 0;

	// test log
	std::fstream detection_log_txt = util::create_txt_file("C:\\data\\DetectionLog0215.txt");
	std::fstream running_log_txt = util::create_txt_file("C:\\data\\RunningLog0215.txt");
	std::fstream debug_log_txt = util::create_txt_file("C:\\data\\DebugLog0215.txt");


	// 针织状态参数
	bool knit_status = false;
	/***********************************************/
	// 流程
	void load_offset_table(char* file_name);
	void load_template(char* file_name);
	void decode_yarn();

	void save_offset_table(const char* file_name);
	void auto_calibration(cv::Mat& src);
	g_data<calibration> img_calibration(cv::Mat& src);
	void make_offset_table(cv::Mat& src, uart_group_data uart);
	void split_image(cv::Mat& src, uart_group_data uart);
	bool focus_detection(cv::Mat& src);
	bool brightness_detection(cv::Mat& src);
	void cam_test(cv::Mat& src);
	bool data_validity_check();
	void row_count_record(uart_group_data& uart);
	bool look_up_needle_category(int row, int light_path, int side, int index);
	bool compare(int c_res, int row, int light_path, int side, int index);
	void detection();
	int needle_classification(int row, int light_paht, int side, int index);
	void position_test(cv::Mat& src, uart_group_data uart);

	// 工具
	sub_offset_data format_map_data(uart_group_data& uart, int start_x, int shift_count, int stitch_index);
	int get_offset_distance(int row, int path);
	void save_image(std::string file_name, cv::Mat& src);
	void dbg(int step);

	// 执行
	void init();
	void run();
	/***********************************************/

protected:
	virtual void Execute() override;
};
