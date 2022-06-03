/**
 * @file pt_logic.cpp
 * @author lty (302702801@qq.com)
 * @brief
 * @version 0.1
 * @date 2022-04-27
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "pt_logic.h"
#include "pt_operator.h"
#include "../optjson/templatejson.h"
#include "../optjson/offsettable.h"
#include "../logger/logger.h"

#define LOG_S(...) Logger::get_instance().log_plus((const std::string)(FILENAME(__FILE__)), (const std::string)(__FUNCTION__), (int)(__LINE__), __VA_ARGS__)

PtLogic::PtLogic(int l_p, std::shared_ptr<PtAllocator> ptr_a)
{
	LOG_S("开始构造" + LIGHT_PATH_MAPPING[l_p] + "光路预处理", LOGGER_INFO);
	light_path = l_p;
	ptr_allocator = ptr_a;
	LOG_S("成功构造" + LIGHT_PATH_MAPPING[l_p] + "光路预处理", LOGGER_INFO);
}

void PtLogic::run()
{
	LOG_S("执行" + LIGHT_PATH_MAPPING[light_path] + "光路预处理", LOGGER_INFO);
	// 初始化
	init();
	// loop
	while (true)
	{
		// 队列内部实现了阻塞可以直接调用pop来获取数据
		// 具体线程与队列的映射
		pt_queue_set[light_path]->Pop(&gdata);
		LOG_S(LIGHT_PATH_MAPPING[light_path] + "光路获取到图像帧宽:" + std::to_string(gdata.c_data.cols) +
				  " 高:" + std::to_string(gdata.c_data.rows) +
				  " and 串口帧:" + std::to_string(gdata.u_data.frame_num - 1) +
				  "串口行号:" + std::to_string(gdata.u_data.x) +
				  "串口列号:" + std::to_string(gdata.u_data.y) +
				  "串口方向：" + std::to_string(gdata.u_data.direction) +
				  "当前状态:" + STATE_MAPPING[(int)level],
			  LOGGER_INFO);

		row_monitor(gdata.u_data);

		switch (level)
		{
		case IDLE:
			// 预留状态，具体工作待填充
			test_save_with_position(gdata.c_data, gdata.u_data);
			break;

		case AUTO_CALIBRATE:
			auto_calibration(gdata.c_data);
			break;

		case GENTABLE:
			make_offset_table(gdata.c_data, gdata.u_data);
			break;

		case VISDECT:
			split_image(gdata.c_data, gdata.u_data);
			break;

		case AUTO_DETECT:
			auto_dect(gdata.c_data, gdata.u_data);
			break;

		case ERR_MODE:
			// 预留状态，具体工作待填充
			break;

		default:
			break;
		}
	}
	LOG_S("结束" + LIGHT_PATH_MAPPING[light_path] + "光路预处理", LOGGER_INFO);
}

void PtLogic::init()
{
	// 具体操作待定
}

bool PtLogic::check_row_valid(machine_head_xy u_data)
{
	return false;
}

bool PtLogic::check_light_path_valid(int row_cnt)
{
	// 返回衣物模板中该光路key中alue是否有效
	auto itr = templatejson::d[("row" + std::to_string(row_cnt)).c_str()][LIGHT_PATH_MAPPING[light_path].c_str()].FindMember("direction");
	if (itr == templatejson::d[("row" + std::to_string(row_cnt)).c_str()][LIGHT_PATH_MAPPING[light_path].c_str()].MemberEnd())
		return false;
	else
		return true;
}

bool PtLogic::check_col_valid(machine_head_xy u_data)
{
	// 返回偏移表中是否存在该列号所对应的key值
	auto itr = offsettable::d[light_path]["table"].FindMember(std::to_string(get_dpos(u_data)).c_str());
	if (itr == offsettable::d[light_path]["table"].MemberEnd())
		return false;
	else
		return true;
}

int PtLogic::get_cur_split_cnt(machine_head_xy u_data)
{
	// 返回偏移表中列号对应array的长度
	return offsettable::d[light_path]["table"][std::to_string(get_dpos(u_data)).c_str()].GetArray().Size();
}

int PtLogic::cal_back_offset_hardware()
{
	return ptr_allocator->get_offset();
}

int PtLogic::cal_back_offset_software()
{
	std::string back_offset = get_node_value<std::string>("offset");
	float res = 0;

	// 目前偏移均依据织针的实际位置偏移来修正:"N"的位置不需要修正
	if (back_offset[0] == 'U')
		res += 0.25;
	else if (back_offset[0] == '#')
		res += 0.5;

	// TODO: 目前后针排移动方向颠倒，待实测(待截取区域是否会超出实际图像边界？)
	if (back_offset.size() == 4)
		res += back_offset[2] == 'R' ? (back_offset[3] - '0') : -(back_offset[3] - '0');
	res *= NEEDLE_GRID_WIDTH;

	return static_cast<int>(std::round(res));
}

VI PtLogic::fit_edge(const VI &r_edges, unsigned char direction, int x_start_cor, int split_cnt)
{
	VI res;
	while (split_cnt--)
	{
		int virtual_edge = x_start_cor + get_direction(direction) * NEEDLE_GRID_WIDTH;
		int nearest_real_edge = ptreate::search(r_edges, virtual_edge);
		if (abs(nearest_real_edge - virtual_edge) > NEEDLE_GRID_WIDTH / 2)
		{
			res.emplace_back(virtual_edge);
			x_start_cor = virtual_edge;
		}
		else
		{
			res.emplace_back(nearest_real_edge);
			x_start_cor = nearest_real_edge;
		}
	}
	// 将下次起始的虚拟位置也加入结果的最后一位中
	res.emplace_back(x_start_cor + get_direction(direction) * NEEDLE_GRID_WIDTH);
	return res;
}

void PtLogic::row_monitor(machine_head_xy u_data)
{
	do
	{
		// 内部计数器自增
		inner_cnt++;

		cur_pos.direction = u_data.direction;
		cur_pos.row = u_data.x;
		cur_pos.col = u_data.y;

		// 当前行号与之前行号不同时进行(目前认为硬件发送的行号是处理过防抖的)
		if (cur_pos.row != pre_pos.row)
		{
			row_count++;

			pre_pos.direction = cur_pos.direction;
			pre_pos.row = cur_pos.row;

			// 只有这两个状态下需要换行的标志位
			if (level == GENTABLE || level == VISDECT)
				row_changed_flag = true;
		}
		pre_pos.col = cur_pos.col;

		LOG_S(LIGHT_PATH_MAPPING[light_path] + "开始处理" + std::to_string(row_count) + "行" + std::to_string(u_data.y) + "列数据" + +"光路预处理", LOGGER_INFO);

	} while (false);
}

void PtLogic::auto_calibration(cv::Mat &src)
{
	calibration_data.angle = ptreate::cal_rotate_angle(src);
	calibration_data.mid_line = ptreate::cal_split_midline(src);
	calibration_data.light_path = light_path + 'A';
	// 向业务逻辑线程发送最终结果type->pt_calibration
	calibrate_queue->Push(calibration_data);
	LOG_S(LIGHT_PATH_MAPPING[light_path] + "光路执行自校准,Angle=" + std::to_string(calibration_data.angle) +
			  ",MidLine=" + std::to_string(calibration_data.mid_line),
		  LOGGER_DEBUG);
}

void PtLogic::make_offset_table(cv::Mat &src, machine_head_xy u_data)
{

	switch (mk_table_state)
	{
	case S_WAIT:
	{
		if ((row_changed_flag && cur_pos.direction == R) ||
			(row_changed_flag && needle_index == NEEDLE_GRID_COUNT && cur_pos.direction == L))
		{
			row_changed_flag = false;
			mk_table_state = S_FINDING;
		}
		break;
	}

	case S_FINDING:
	{
		// 选取前针排有效光路区域作为ROI
		cv::Mat feature_ROI = ptreate::get_ROI_p(src,
												 VALID_REGION_COL_START,
												 VALID_REGION_COL_END,
												 calibration_data.mid_line,
												 IN_IMG_HEIGHT);

		// 返回真实图像中特征分界线的y坐标
		y_coordinate = ptreate::search_start_feature(feature_ROI, u_data.direction) + VALID_REGION_COL_START;
		if (y_coordinate != -1)
		{
			// 区分左右(左右两边的偏移量应该是一样的)
			y_coordinate += get_direction(u_data.direction) * Y_START_FIXED_OFFSET;

			// 将特征区域位置写入位置集合中方便自动监测判断
			feature_position.emplace(get_dpos(u_data));
			mk_table_state = S_DOING;
		}
		break;
	}

	case S_DOING:
	{
		// 每次进入S_DOING都经过了一次移动所以y_coordinate需要减去相对偏移
		y_coordinate -= get_direction(u_data.direction) * PER_STEP_RELATIVE_OFFSET;

		// 计算预选框位置
		// **由于每次相机步进长度并不是针格的整数倍，但实际截取针格需要截取整数个针格，所以需要做成动态选取针格数量，从而防止截出有效区域

		// max_cnt取下整
		int max_cnt = abs(y_coordinate - u_data.direction ? VALID_REGION_COL_END : VALID_REGION_COL_START) / NEEDLE_GRID_WIDTH;
		// min_cnt取上整
		int min_cnt = ceil(std::max(NEEDLE_GRID_WIDTH,
									PER_STEP_RELATIVE_OFFSET + (u_data.direction ? (VALID_REGION_COL_START - y_coordinate) : (y_coordinate - VALID_REGION_COL_END))) /
						   (float)NEEDLE_GRID_WIDTH);

		// 随机获取当前图片截取的针格数量
		int real_cnt = rand() % (max_cnt - min_cnt + 1) + min_cnt;

		int window_ys = y_coordinate - get_direction(u_data.direction) * NEEDLE_GRID_WIDTH / 2;
		int window_ye = y_coordinate + get_direction(u_data.direction) * (real_cnt * NEEDLE_GRID_WIDTH + NEEDLE_GRID_WIDTH / 2);

		// 依据新的区间提取ROI
		cv::Mat ROI = ptreate::get_ROI_p(src,
										 calibration_data.mid_line + WINDOW_XS_OFFSET,
										 calibration_data.mid_line + WINDOW_XE_OFFSET,
										 std::min(window_ys, window_ye),
										 std::max(window_ys, window_ye));

		// 获取ROI区域边缘y坐标vector(VI的size可能是[0, real_cnt + 1],
		//                         正常情况下是real_cnt + 1个边缘,如果内部存在针格缺陷没有找到或者异常都会导致最终结果少于real_cnt + 1)
		VI real_edges = ptreate::search_region_edge(ROI);

		// 拟合真实y坐标与理论y坐标
		VI fixed_edges = fit_edge(real_edges, u_data.direction, y_coordinate, real_cnt);

		// 依据真实数量截取前后针格
		for (int i = 0; i < fixed_edges.size() - 1; i++)
		{
			offset_table_data.offset_table[get_dpos(u_data)].emplace_back(std::make_pair(needle_index, fixed_edges[i]));
			u_data.direction ? needle_index++ : needle_index--;

			// 当索引超出实际范围后说明已经找全了所有针格
			if (needle_index > NEEDLE_GRID_COUNT)
			{
				needle_index = NEEDLE_GRID_COUNT;
				mk_table_state = S_FINDING;
				break;
			}
			else if (needle_index < 0)
			{
				needle_index = 0;
				mk_table_state = S_SEND;
				break;
			}
		}

		// 更新y_coordinate为下一次起始坐标
		y_coordinate = fixed_edges[fixed_edges.size() - 1];
		break;
	}

	case S_SEND:
	{
		// 向业务逻辑线程发送最终结果type->offset_table_data
		offset_table_data.light_path = light_path + 'A';
		offset_table_queue->Push(offset_table_data);
		mk_table_state = S_IDLE;
		break;
	}

	case S_IDLE:
		break;

	default:
		break;
	}
}

void PtLogic::split_image(cv::Mat &src, machine_head_xy u_data)
{
	if (row_changed_flag)
	{
		row_changed_flag = false;
		// 向神经网络线程发送最终结果type->pt_needle_data
		needle_queue->Push(front_needle_data);
		needle_queue->Push(back_needle_data);
		LOG_S("成功截取" + std::to_string(row_count - 1) + "行前后针格并送入神经网络消息队列", LOGGER_INFO);
	}

	if (check_light_path_valid(row_count) && check_col_valid(u_data))
	{
		for (int i = 0; i < get_cur_split_cnt(u_data); i++)
		{
			int y_coordinate = get_node_value<int>("y_coordinate");
			int index = get_node_value<int>("stitch_index");
			// 旋转图片到水平方向
			auto rotate_src = ptreate::rotate_image(src, calibration_data.angle, true);

			// 修正边缘
			auto res = ptreate::modify_edge(rotate_src, y_coordinate);
			if (res.first != PT_OK)
			{
				split_error_cnt++;
				if (split_error_cnt > 8)
				{
					// TODO: 告警(偏移过多）
					LOG_S("截取针格连续偏移次数过多，需要修正", LOGGER_ALARM);
				}
				LOG_S("截取针格边缘异常,Cnt:" + std::to_string(split_error_cnt), LOGGER_DEBUG);
			}
			else
				split_error_cnt = 0;

			// 注意区分左行与右行
			// 截取前针格
			auto front_cropped_region = ptreate::get_ROI_p(rotate_src,
														   calibration_data.mid_line + 10,
														   calibration_data.mid_line + NEEDLE_GRID_HIGH + 10,
														   y_coordinate + (u_data.direction ? 0 : -1) * NEEDLE_GRID_WIDTH,
														   y_coordinate + (u_data.direction ? 1 : 0) * NEEDLE_GRID_WIDTH);

			// 截取后针格
			// 两种计算方式(软件存在偏差，硬件方法逻辑需要重新梳理)
			int back_offset = cal_back_offset_software();
			// int back_offset = cal_back_offset_hardware();

			auto back_cropped_region = ptreate::get_ROI_p(rotate_src,
														  calibration_data.mid_line - NEEDLE_GRID_HIGH - 10,
														  calibration_data.mid_line - 10,
														  y_coordinate + (u_data.direction ? 0 : -1) * NEEDLE_GRID_WIDTH + back_offset,
														  y_coordinate + (u_data.direction ? 1 : 0) * NEEDLE_GRID_WIDTH + back_offset);

			// 180度反转
			cv::flip(back_cropped_region, back_cropped_region, -1);

			// 将截取的针格按索引覆盖写入array数据中
			front_needle_data.needle_img[index] = front_cropped_region;
			back_needle_data.needle_img[index] = back_cropped_region;
		}
	}
}

void PtLogic::auto_dect(cv::Mat &src, machine_head_xy u_data)
{
	// 只在特征区域处作自动检测
	if (feature_position.count(get_dpos(u_data)))
	{
		auto res = ptreate::brightness_detection(src) && ptreate::focus_detection(src);
		// 向业务逻辑线程发送最终结果type->bool
		auto_detect_queue->Push(pt_auto_detect(light_path + 'A', res));
	}
}

void PtLogic::test_save_with_position(cv::Mat &src, machine_head_xy u_data)
{
	auto start = util::get_cur_time();
	std::string path_str = "../data/" + std::to_string(light_path) + "/" + std::to_string(u_data.y);
	if (!util::exists(path_str))
	{
		util::mkdirs(path_str);
	}
	cv::imwrite(path_str + "/" + std::to_string(u_data.x) + ".bmp", src);
	auto end = util::get_cur_time();
	LOG_S("成功写入图片:行号:" + std::to_string(u_data.x) + " 列号:" + std::to_string(u_data.y) + " 耗时:" + std::to_string((chrono::duration_cast<chrono::microseconds>(end - start).count()) / 1000000.0), LOGGER_INFO);
}