#pragma once

#include "proto.h"
#include "uds.h"
#include "json.h"
#include "cthead.h"
#include "utils.h"


// TODO:����printf_s��ͨ��logģ���������־��
class ptreate : public CThread
{
public:
	/********************** UD *********************/
	// ԭʼ����
	cam_group_data cam_data;
	uart_group_data uart_data;

	// �������������
	unsigned int pre_cam_frame = 0;
	unsigned int pre_uart_frame = 0;

	// ��֡ͳ����
	unsigned int drop_cam_frame_count = 0;
	unsigned int drop_uart_frame_count = 0;
	
	// Ԥ�ȿ��ٵ�Ԥ�����ݶ���
	needle_data A_FRONT;
	needle_data A_BACK;

	needle_data B_FRONT;
	needle_data B_BACK;

	needle_data C_FRONT;
	needle_data C_BACK;

	std::vector<std::vector<needle_data>> needle_set = { {A_FRONT, A_BACK}, {B_FRONT, B_BACK}, {C_FRONT, C_BACK} };

	// Ԥ����ǰ���������
	cv::Mat front_cropped_region;
	cv::Mat back_cropped_region;

	// Ԥ�����������
	needle_ans_data detected_res;

	// ȱ��ͳ��
	int total_defect_count = 0;
	int total_continuous_defect_count = 0;
	bool continuous_flag = false;

	// ƫ�Ʊ����״̬
	bool offset_table_exist_status = false;

	// ģ�����״̬
	bool template_exist_status = false;

	// ״̬��־λ
	int gpm_cur_status = calibrate;

	// ����к�����
	int template_max_row = 0;
	int _row_count = -2;
	bool row_changed_flag = false;
	position _last_position;
	position _cur_position;
	
	// ƫ�Ʊ�������ʼ��־λ
	bool offset_table_start_flag = true;

	// ����ƫ�Ʊ�����
	int _start_x = -1;
	int _shift_count = 1;
	int _stitch_index = 0;
	bool _valid_flag = false;

	// ��¼��������Debug
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


	// ��֯״̬����
	bool knit_status = false;
	/***********************************************/
	// ����
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

	// ����
	sub_offset_data format_map_data(uart_group_data& uart, int start_x, int shift_count, int stitch_index);
	int get_offset_distance(int row, int path);
	void save_image(std::string file_name, cv::Mat& src);
	void dbg(int step);

	// ִ��
	void init();
	void run();
	/***********************************************/

protected:
	virtual void Execute() override;
};
