#include "ptreate.h"
#include "gve.h"

/*************************************** 流程 **************************************/
void ptreate::load_offset_table(char* file_name)
{
	if (json::read(file_name, _offset_table))
	{
		offset_table_exist_status = true;
		printf_s("@@@Offset Table Check: Pass@@@\n");
	}
	else
	{
		printf_s("@@@Offset Table Check: Fail@@@\n");
		for (int i = 0; i < 3; i++)
		{
			light_path new_light_path;
			SD.add_light_path(new_light_path);
		}
	}
}

void ptreate::load_template(char* file_name)
{
	if (json::read(file_name, _clothes_template))
	{
		template_exist_status = true;
		printf_s("@@@Clothes Template Check: Pass@@@\n");
	}
	else
	{
		printf_s("@@@Clothes Template Check: Fail@@@\n");
	}
}

void ptreate::decode_yarn()
{
	// 逐行
	for (auto rows_itr = _clothes_template.MemberBegin(); rows_itr != _clothes_template.MemberEnd(); ++rows_itr) {
		// 逐光路
		for (auto path_itr = rows_itr->value.MemberBegin(); path_itr != rows_itr->value.MemberEnd(); ++path_itr) {
			//std::cout << rows_itr->name.GetString() << " " << path_itr->name.GetString() << std::endl;
			// 确认该光路是否有效
			auto pyarn = path_itr->value.FindMember("yarn_position");
			auto pFVR = path_itr->value.FindMember("front_valid");
			auto pBVR = path_itr->value.FindMember("back_valid");

			if (pFVR != path_itr->value.MemberEnd()) {
				for (auto& v: pFVR->value.GetArray()) {
					std::vector<int> temp;
					for (auto& c : v.GetArray())
						temp.push_back(c.GetInt());
					valid_region[rows_itr->name.GetString()][path_itr->name.GetString()]["front_valid"].push_back(temp);
				}
			}
			if (pBVR != path_itr->value.MemberEnd()) {
				for (auto& v : pBVR->value.GetArray()) {
					std::vector<int> temp;
					for (auto& c : v.GetArray())
						temp.push_back(c.GetInt());
					valid_region[rows_itr->name.GetString()][path_itr->name.GetString()]["back_valid"].push_back(temp);
				}
			}
			if (pyarn != path_itr->value.MemberEnd()) {
				for (auto position_itr = pyarn->value.MemberBegin(); position_itr != pyarn->value.MemberEnd(); ++position_itr) {
					yarn_positon[rows_itr->name.GetString()][path_itr->name.GetString()].insert(position_itr->value.GetInt());
					for (int i = 1; i <= YARN_SHADE_COUNT; i++) {
						yarn_positon[rows_itr->name.GetString()][path_itr->name.GetString()].insert(position_itr->value.GetInt() + i);
						yarn_positon[rows_itr->name.GetString()][path_itr->name.GetString()].insert(position_itr->value.GetInt() - i);
					}
				}
			}
		}
		template_max_row++;
	}
}

void ptreate::save_offset_table(const char* file_name)
{
	try {
		// 序列化数据结构
		json::parse(_offset_table, SD);

		// 将DOM树结构写入json文件中
		json::write(file_name, _offset_table);
	}
	catch (const std::exception& error) {
		printf_s("Save offset table error %s\n", error.what());
	}
}

void ptreate::auto_calibration(cv::Mat& src)
{
	if (!offset_table_exist_status) {
		// TODO:三光路的自动修正需要重新梳理算法和逻辑(待实现)
		int correct_count = 0;
		for (int i = 0; i < LIGHT_PATH_COUNT; i++) {
			// 单光路过滤
			if (std::find(VALID_LIGHT_PATH.begin(), VALID_LIGHT_PATH.end(), i) != VALID_LIGHT_PATH.end()) {
				auto g_cal = img_calibration(src);
				if (g_cal.state) {
					SD._mult_light_path[i]._calibration = g_cal.data;
					correct_count++;
				}
				else
					break;
			}
		}
		if (correct_count == VALID_LIGHT_PATH.size()) {
			gpm_cur_status = gentable;
			printf_s("@@@@Auto calibration OK@@@@\n");
		}
		else
		{
			printf_s("@@@@Auto calibration error@@@@\n");
		}
	}
	// 偏移表已存在，直接进去idle状态等待设备检测，目前先直接进入执行状态
	else
		gpm_cur_status = execute;
}

g_data<calibration> ptreate::img_calibration(cv::Mat& src)
{
	g_data<calibration> res{};

	res.state = false;

	// 伽马变换
	auto gamma = gamma_transform(src, 4, 1);

	// 直方图均衡化
	auto equal = equalize_hist(gamma);

	// 边缘检测
	auto edge = edge_detection(equal, 100, 200);

	// 霍夫线检测
	auto lines = hough_line_detection(edge, 50, 10, 20);

	if (lines.empty())
	{
		printf_s("Step1: This image couldn't find real midline. Reason: Edge img houghLinesP detect no line\n");
		return res;
	}

	// 斜率检测
	auto k_res = k_detection(lines, 30, 45);
	if (k_res.state)
	{
		res.data._angle = k_res.data;
		// 旋转边缘检测图像
		auto rotate_edge = rotate_image(edge, k_res.data, true);

		// 对旋转的图像做霍夫检测
		auto lines = hough_line_detection(rotate_edge, 50, 10, 20);
		if (lines.empty()) {
			printf_s("Step3: This image couldn't find real midline. Reason: Rotated edge img houghLinesP detect no line\n");
			return res;
		}

		// 直线叠加
		auto cluster_res = line_cluster(lines);
		if (cluster_res.state) {
			// 旋转原始图像
			auto rotate_src = rotate_image(src, k_res.data, true);

			//处理cluster区域获取ROI边界
			int width = rotate_src.cols;
			int height = rotate_src.rows;
			std::vector<int> y_coordinate;

			for (auto &sub_cluster : cluster_res.data) {
				y_coordinate.push_back(sub_cluster.first);
				y_coordinate.push_back(sub_cluster.second);
			}
			std::sort(y_coordinate.begin(), y_coordinate.end());
			int ROI_top = y_coordinate[1] - 10;
			int ROI_down = y_coordinate[2] + 10;

			// 获取ROI
			auto ROI = get_ROI(rotate_src, 0, ROI_top, width, ROI_down - ROI_top);

			// 二值化
			auto threshold = bin_threashold(ROI, 65, 255);

			// 霍夫线检测
			auto lines = hough_line_detection(threshold, 50, 10, 40);
			// 检测线距
			if (lines.empty()) {
				printf_s("Step5: This image couldn't find real midline. Reason: ROI region houghLinesP detect no line\n");
				return res;
			}
			std::vector<int> line_y_coordinate;
			for (auto &line : lines)
				line_y_coordinate.push_back(static_cast<int>(std::round((line[1] + line[3]) / 2)));

			std::sort(line_y_coordinate.begin(), line_y_coordinate.end());
			int real_ROI_top = 0;
			int real_ROI_down = 0;
			for (int i = 0; i < line_y_coordinate.size() - 1; i++) {
				if (55 > line_y_coordinate[i + 1] - line_y_coordinate[i] && line_y_coordinate[i + 1] - line_y_coordinate[i] > 30) {
					real_ROI_top = line_y_coordinate[i];
					real_ROI_down = line_y_coordinate[i + 1];
					break;
				}
			}
			if (real_ROI_down == real_ROI_top) {
				printf_s("Step6: This image couldn't find real midline. Reason: Line gap error\n");
				return res;
			}
			else {
				res.data._midline = (real_ROI_top + real_ROI_down) / 2 + y_coordinate[1] - 10;
				res.state = true;
				return res;
			}
		}
		else {
			printf_s("Step4: This image couldn't find real midline. Reason: Clustering area error\n");
			return res;
		}
	}
	else {
		printf_s("Step2: This image couldn't find real midline. Reason: No angle detected\n");
		return res;
	}
}

void ptreate::make_offset_table(cv::Mat& src, uart_group_data uart)
{
	if (_row_count == 4)
	{
		save_offset_table("../../../conf/table/offset_table.json");
		gpm_cur_status = execute;
		printf_s("@@@@Complete offset table@@@@\n");
	}

	if (2 <= _row_count && _row_count < 4)
	{
		printf_s("@@@@Making offset table@@@@Position:%d@@@@\n", uart.col);
		for (int i = 0; i < LIGHT_PATH_COUNT; i++)
		{
			// 单光路过滤
			if (std::find(VALID_LIGHT_PATH.begin(), VALID_LIGHT_PATH.end(), i) != VALID_LIGHT_PATH.end())
			{
				bool continue_flag = false;
				std::vector<int> x_coordinate;
				std::vector<int> x_k_means;
				std::vector<int> true_edges;

				std::string direction(uart.direction ? "R" : "L");
				std::string file_name("");
				//file_name = "c:/data/src/" + direction + std::to_string(uart.col) + ".bmp";
				//save_image(file_name, src);

				// 旋转图像
				auto rotate_src = rotate_image(src, SD._mult_light_path[i]._calibration._angle, true);

				file_name = "C:/data/rotate/" + direction + std::to_string(uart.col) + ".bmp";
				save_image(file_name, rotate_src);
				// 获取ROI
				auto ROI_window = get_ROI_p(rotate_src, SD._mult_light_path[i]._calibration._midline - WINDOW_X0_OFFSET,
					SD._mult_light_path[i]._calibration._midline - WINDOW_X1_OFFSET,
					WINDOW_Y0,
					WINDOW_Y1);

				// 二值化
				auto bin_window = bin_threashold(ROI_window, 175, 255);

				// 边缘检测
				auto edge_window = edge_detection(bin_window, 110, 200);

				// 霍夫线检测
				auto lines = hough_line_detection(edge_window, 10, 10, 10);

				cv::cvtColor(ROI_window, ROI_window, cv::COLOR_GRAY2BGR);
				if (!lines.empty())
				{
					int parallel_lines_count = 0;
					for (const auto& line : lines)
					{
						if (std::abs(line[0] - line[2]) > 8)
						{
							parallel_lines_count++;
						}
						else
						{
							//cv::line(ROI_window, cv::Point(line[0], line[1]), cv::Point(line[2], line[3]), cv::Scalar(0, 0, 255));
							x_coordinate.push_back(static_cast<int>(std::round((line[0] + line[2]) / 2)));
						}
					}
					if (parallel_lines_count > 1)
					{
						continue_flag = true;
					}
					std::sort(x_coordinate.begin(), x_coordinate.end());
					x_k_means = k_means(x_coordinate);
				}

				std::cout << "聚类结果:";
				for (auto& c : x_k_means)
				{
					cv::line(ROI_window, cv::Point(c, 0), cv::Point(c, 40), cv::Scalar(0, 255, 0), 2);
					std::cout << " " << c;
				}
				std::cout << std::endl;

				true_edges = center_filter(x_k_means);
				std::cout << "过滤结果:";
				for (auto& c : true_edges)
				{
					cv::line(ROI_window, cv::Point(c, 0), cv::Point(c, 40), cv::Scalar(0, 0, 255), 2);
					std::cout << " " << c;
				}
				std::cout << std::endl;
				// 有效区域
				if (2 <= x_k_means.size() && x_k_means.size() <= 9 && 2 < true_edges.size() && true_edges.size() <= 5 && !continue_flag) {
					file_name = "C:/data/ROI/correct" + direction + std::to_string(uart.col) + ".bmp";
					save_image(file_name, ROI_window);
					_valid_flag = true;
					// 找到的起始位置
					if (_start_x < 0) {
						// 目前不处理掉头图片
						if (uart.direction == R) {
							real_edge_index = true_edges.size() - 1;
							_start_x = true_edges[true_edges.size() - 1] + WINDOW_Y0;
						}
						else if (uart.direction == L) {
							real_edge_index = 0;
							_start_x = true_edges[0] + WINDOW_Y0;
						}
					}
					else {
						real_edge = search(true_edges, _start_x - WINDOW_Y0);
						real_edge_index = std::distance(true_edges.begin(), std::find(true_edges.begin(), true_edges.end(), real_edge));

						// 计算异常的位置，可能是针格缺陷，后期可以记录用作检修信息
						if (std::abs(_start_x - WINDOW_Y0 - real_edge) > 16) {
							printf_s("Maybe error, position = D:%d R:%d C:%d Offset:%d\n", uart.direction, uart.row, uart.col, std::abs(_start_x - WINDOW_Y0 - real_edge));
							// 此时不修正start_x使用理论值
						}
						else
							_start_x = real_edge + WINDOW_Y0;
					}
					//printf_s("边缘总数:%d, 当前索引:%d\n", true_edges.size(), real_edge_index);
					if (uart.direction == R) {
						if ((_start_x - _shift_count * NEEDLE_GRID_WIDTH + RELATIVE_OFFSET) > (WINDOW_Y1 - 40))
							_shift_count = 2;
						else if ((WINDOW_Y0 + 40) > (_start_x - _shift_count * NEEDLE_GRID_WIDTH + RELATIVE_OFFSET) || real_edge_index - _shift_count < 0)
							_shift_count = 1;

						// 结尾处理
						if (real_edge_index == 0) {
							printf_s("处理右行，结尾有效区域if路径\n");
							SD._mult_light_path[i].add_sub_offset_data(format_map_data(uart, -1, -1, -1));
							_start_x += RELATIVE_OFFSET;
							continue;
						}
					}
					else if (uart.direction == L) {
						if ((_start_x + _shift_count * NEEDLE_GRID_WIDTH - RELATIVE_OFFSET) > (WINDOW_Y1 - 40) || real_edge_index + _shift_count >= true_edges.size())
							_shift_count = 1;
						else if ((WINDOW_Y0 + 40) > (_start_x + _shift_count * NEEDLE_GRID_WIDTH - RELATIVE_OFFSET))
							_shift_count = 2;

						// 结尾处理
						if (real_edge_index == true_edges.size() - 1) {
							printf_s("处理左行，结尾有效区域if路径\n");
							SD._mult_light_path[i].add_sub_offset_data(format_map_data(uart, -1, -1, -1));
							_start_x -= RELATIVE_OFFSET;
							continue;
						}
					}
					// 写偏移表数据
					SD._mult_light_path[i].add_sub_offset_data(format_map_data(uart, _start_x, _shift_count, _stitch_index));
					// 修改下一次起始位置
					_start_x += uart.direction == R ? -(_shift_count * NEEDLE_GRID_WIDTH - RELATIVE_OFFSET) : (_shift_count * NEEDLE_GRID_WIDTH - RELATIVE_OFFSET);
					// 更新下一次索引
					_stitch_index += uart.direction == R ? _shift_count : -_shift_count;
				}
				else {
					file_name = "C:/data/ROI/error" + direction + std::to_string(uart.col) + ".bmp";
					save_image(file_name, ROI_window);
					if (_valid_flag) {
						if (uart.direction == R) {
							if (_stitch_index < NEEDLE_GRID_COUNT - 1) {
								printf_s("处理右行，结尾无效区域if路径\n");
								_shift_count = NEEDLE_GRID_COUNT - _stitch_index;
								printf_s("索引:%d,截取个数:%d\n", _stitch_index, _shift_count);
								SD._mult_light_path[i].add_sub_offset_data(format_map_data(uart, _start_x, _shift_count, _stitch_index));
							}
							else {
								printf_s("处理右行，结尾无效区域else路径\n");
								SD._mult_light_path[i].add_sub_offset_data(format_map_data(uart, -1, -1, -1));
							}
							_stitch_index = NEEDLE_GRID_COUNT - 1;
						}
						else if (uart.direction == L) {
							if (_stitch_index > 0) {
								printf_s("处理左行，结尾无效区域if路径\n");
								_shift_count = _stitch_index + 1;
								printf_s("索引:%d,截取个数:%d\n", _stitch_index, _shift_count);
								SD._mult_light_path[i].add_sub_offset_data(format_map_data(uart, _start_x, _shift_count, _stitch_index));
							}
							else {
								printf_s("处理左行，结尾无效区域else路径\n");
								SD._mult_light_path[i].add_sub_offset_data(format_map_data(uart, -1, -1, -1));
							}
							_stitch_index = 0;
						}
						_valid_flag = false;
					}
					else {
						SD._mult_light_path[i].add_sub_offset_data(format_map_data(uart, -1, -1, -1));
					}
					// 赋值-1用于逻辑处理起点
					_start_x = -1;
				}
			}
		}
	}
}

void ptreate::split_image(cv::Mat& src, uart_group_data uart)
{
	do {
		// 编织未开始直接跳过
		if (!knit_status)
			break;
		
		for (int i = 0; i < LIGHT_PATH_COUNT; i++) {
			// 当前光路无效直接跳过
			if (std::find(VALID_LIGHT_PATH.begin(), VALID_LIGHT_PATH.end(), i) == VALID_LIGHT_PATH.end() ||
				_clothes_template[("row" + std::to_string(_row_count + WASTE_YARN_GAP)).c_str()][LIGHT_PATH_MAPPING[i].c_str()].FindMember("direction") ==
				_clothes_template[("row" + std::to_string(_row_count + WASTE_YARN_GAP)).c_str()][LIGHT_PATH_MAPPING[i].c_str()].MemberEnd())
				continue;

			std::string D = std::to_string(uart.direction);
			std::string C = std::to_string(uart.col);
			
			// 当前位置不存在直接跳过
			if (_offset_table[LIGHT_PATH_MAPPING[i].c_str()]["table"].FindMember((D + C).c_str()) == _offset_table[LIGHT_PATH_MAPPING[i].c_str()]["table"].MemberEnd())
				break;

			int y_coordinate = _offset_table[LIGHT_PATH_MAPPING[i].c_str()]["calibration"]["midline"].GetInt();
			int x_coordinate = _offset_table[LIGHT_PATH_MAPPING[i].c_str()]["table"][(D + C).c_str()]["x_coordinate"].GetInt();

			// 当前位置存在但不是有效针格区域跳过
			if (x_coordinate == -1)
				break;

			util::create_dir("c:/data/original/group1/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/front");
			util::create_dir("c:/data/original/group1/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/back");
			//util::create_dir("c:/data/original/group1/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/dect");

			/******************************************采集数据*******************************************************/
			util::create_dir("c:/data/original/src/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/dect");

			util::create_dir("c:/data/original/group2/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/front");
			util::create_dir("c:/data/original/group2/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/back");

			util::create_dir("c:/data/original/group3/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/front");
			util::create_dir("c:/data/original/group3/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/back");

			util::create_dir("c:/data/original/group4/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/front");
			util::create_dir("c:/data/original/group4/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/back");
			/******************************************采集数据*******************************************************/

			int shift_count = _offset_table[LIGHT_PATH_MAPPING[i].c_str()]["table"][(D + C).c_str()]["shift_count"].GetInt();
			int stitch_index = _offset_table[LIGHT_PATH_MAPPING[i].c_str()]["table"][(D + C).c_str()]["stitch_index"].GetInt();

			/******************************************采集数据*******************************************************/
			save_image("c:/data/original/src/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/dect/R" + std::to_string(_row_count + WASTE_YARN_GAP) + "I" + std::to_string(stitch_index + ZERO_INDEX_FIX + NEEDLE_INDEX_FIX) + "COL" + std::to_string(uart.col) + ".bmp", src);
			/******************************************采集数据*******************************************************/

			// 旋转图像
			auto rotate_src = rotate_image(src, _offset_table[LIGHT_PATH_MAPPING[i].c_str()]["calibration"]["angle"].GetDouble(), true);

			// 获取ROI进行位置修正
			cv::Mat detection_window_img = get_ROI_p(rotate_src, y_coordinate - WINDOW_X0_OFFSET,
				y_coordinate - WINDOW_X1_OFFSET,
				x_coordinate - 12,
				x_coordinate + 12);

			// 二值化
			auto bin_window = bin_threashold(detection_window_img, 100, 255);

			// 边缘检测
			auto edge_window = edge_detection(bin_window, 110, 200);

			// 霍夫线检测
			auto lines = hough_line_detection(edge_window, 10, 10, 10);

			//cv::Mat test_img;
			//cv::cvtColor(rotate_src, test_img, cv::COLOR_GRAY2BGR);

			//cv::line(test_img, cv::Point(x_coordinate - 12, y_coordinate - WINDOW_X0_OFFSET), cv::Point(x_coordinate - 12, y_coordinate - WINDOW_X1_OFFSET), cv::Scalar(255, 0, 0), 1);
			//cv::line(test_img, cv::Point(x_coordinate + 12, y_coordinate - WINDOW_X0_OFFSET), cv::Point(x_coordinate + 12, y_coordinate - WINDOW_X1_OFFSET), cv::Scalar(255, 0, 0), 1);
			//cv::line(test_img, cv::Point(x_coordinate - 12, y_coordinate - WINDOW_X0_OFFSET), cv::Point(x_coordinate + 12, y_coordinate - WINDOW_X0_OFFSET), cv::Scalar(255, 0, 0), 1);
			//cv::line(test_img, cv::Point(x_coordinate - 12, y_coordinate - WINDOW_X1_OFFSET), cv::Point(x_coordinate + 12, y_coordinate - WINDOW_X1_OFFSET), cv::Scalar(255, 0, 0), 1);

			if (!lines.empty()) {
				float x_offset = 0;
				int x_counts = 0;
				for (auto& line : lines) {
					//cv::line(test_img, cv::Point(x_coordinate - 12 + line[0], y_coordinate - WINDOW_X0_OFFSET + line[1]), cv::Point(x_coordinate - 12 + line[2], y_coordinate - WINDOW_X0_OFFSET + line[3]), cv::Scalar(0, 0, 255));
					if (std::abs(line[0] - line[2]) > 5)
						continue;
					x_offset += (line[0] + line[2]) / 2;
					x_counts++;
				}
				if (x_counts > 0) {
					x_offset /= x_counts;
					x_offset -= 12;
					x_coordinate += static_cast<int>(std::round(x_offset));

					//cv::line(test_img, cv::Point(x_coordinate, y_coordinate - WINDOW_X0_OFFSET), cv::Point(x_coordinate, y_coordinate - WINDOW_X1_OFFSET), cv::Scalar(0, 255, 0), 2);
				}
			}
		/*	else
			{
				cv::line(test_img, cv::Point(x_coordinate, y_coordinate - WINDOW_X0_OFFSET), cv::Point(x_coordinate, y_coordinate - WINDOW_X1_OFFSET), cv::Scalar(0, 255, 0), 2);
			}

			save_image("C:/data/split/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/dect/R" + std::to_string(_row_count + WASTE_YARN_GAP) + "I" + std::to_string(stitch_index + ZERO_INDEX_FIX + NEEDLE_INDEX_FIX) + "COL" + std::to_string(uart.col) +  ".bmp", test_img);
*/
			cv::Mat filp_back_group1;
			cv::Mat filp_back_group2;

			cv::Mat front_cropped_region_group2;
			cv::Mat front_cropped_region_group3;
			cv::Mat front_cropped_region_group4;

			cv::Mat back_cropped_region_group2;
			cv::Mat back_cropped_region_group3;
			cv::Mat back_cropped_region_group4;

			int x_s_g1, x_e_g1, y_s_g1, y_e_g1, x_s_g2, x_e_g2, y_s_g2, y_e_g2;
			double angle;

			// 截取固定针格数量
			for (int j = 0; j < shift_count; j++) {
				if (uart.direction == R) {
					front_cropped_region = get_ROI_p(rotate_src,
													 y_coordinate - NEEDLE_GRID_HIGH - 10,
													 y_coordinate - 10,
													 x_coordinate - NEEDLE_GRID_WIDTH * (j + 1),
													 x_coordinate - NEEDLE_GRID_WIDTH * j);

					int offset_distance = get_offset_distance(_row_count, i);

					x_s_g1 = y_coordinate - NEEDLE_GRID_HIGH - 10;
					x_e_g1 = y_coordinate - 10;
					y_s_g1 = x_coordinate - NEEDLE_GRID_WIDTH * (j + 1);
					y_e_g1 = x_coordinate - NEEDLE_GRID_WIDTH * j;

					x_s_g2 = y_coordinate - NEEDLE_GRID_HIGH - 10;
					x_e_g2 = y_coordinate - 10;
					y_s_g2 = x_coordinate - NEEDLE_GRID_WIDTH * (j + 1) - 5;
					y_e_g2 = x_coordinate - NEEDLE_GRID_WIDTH * j + 5;
					angle = _offset_table[LIGHT_PATH_MAPPING[i].c_str()]["calibration"]["angle"].GetDouble();

					front_cropped_region_group2 = get_ROI_p(rotate_src, x_s_g2, x_e_g2, y_s_g2, y_e_g2);

					front_cropped_region_group3 = get_masked_ROI(src, rotate_src, y_s_g1, y_e_g1, x_s_g1, x_e_g1, angle);

					front_cropped_region_group4 = get_masked_ROI(src, rotate_src, y_s_g2, y_e_g2, x_s_g2, x_e_g2, angle);

					// TODO: 坐标修正为7
					back_cropped_region = get_ROI_p(rotate_src,
													y_coordinate + 7,
													y_coordinate + NEEDLE_GRID_HIGH + 7,
													x_coordinate - NEEDLE_GRID_WIDTH * (j + 1) + offset_distance,
													x_coordinate - NEEDLE_GRID_WIDTH * j + offset_distance);

					x_s_g1 = y_coordinate + 7;
					x_e_g1 = y_coordinate + NEEDLE_GRID_HIGH + 7;
					y_s_g1 = x_coordinate - NEEDLE_GRID_WIDTH * (j + 1) + offset_distance;
					y_e_g1 = x_coordinate - NEEDLE_GRID_WIDTH * j + offset_distance;

					x_s_g2 = y_coordinate + 7;
					x_e_g2 = y_coordinate + NEEDLE_GRID_HIGH + 7;
					y_s_g2 = x_coordinate - NEEDLE_GRID_WIDTH * (j + 1) + offset_distance - 5;
					y_e_g2 = x_coordinate - NEEDLE_GRID_WIDTH * j + offset_distance + 5;

					back_cropped_region_group2 = get_ROI_p(rotate_src, x_s_g2, x_e_g2, y_s_g2, y_e_g2);

					back_cropped_region_group3 = get_masked_ROI(src, rotate_src, y_s_g1, y_e_g1, x_s_g1, x_e_g1, angle);

					back_cropped_region_group4 = get_masked_ROI(src, rotate_src, y_s_g2, y_e_g2, x_s_g2, x_e_g2, angle);

				}
				else if (uart.direction == L) {
					front_cropped_region = get_ROI_p(rotate_src,
													 y_coordinate - NEEDLE_GRID_HIGH - 10,
													 y_coordinate - 10,
													 x_coordinate + NEEDLE_GRID_WIDTH * j,
													 x_coordinate + NEEDLE_GRID_WIDTH * (j + 1));

					x_s_g1 = y_coordinate - NEEDLE_GRID_HIGH - 10;
					x_e_g1 = y_coordinate - 10;
					y_s_g1 = x_coordinate + NEEDLE_GRID_WIDTH * j;
					y_e_g1 = x_coordinate + NEEDLE_GRID_WIDTH * (j + 1);

					x_s_g2 = y_coordinate - NEEDLE_GRID_HIGH - 10;
					x_e_g2 = y_coordinate - 10;
					y_s_g2 = x_coordinate + NEEDLE_GRID_WIDTH * j - 5;
					y_e_g2 = x_coordinate + NEEDLE_GRID_WIDTH * (j + 1) + 5;
					angle = _offset_table[LIGHT_PATH_MAPPING[i].c_str()]["calibration"]["angle"].GetDouble();

					front_cropped_region_group2 = get_ROI_p(rotate_src, x_s_g2, x_e_g2, y_s_g2, y_e_g2);

					front_cropped_region_group3 = get_masked_ROI(src, rotate_src, y_s_g1, y_e_g1, x_s_g1, x_e_g1, angle);

					front_cropped_region_group4 = get_masked_ROI(src, rotate_src, y_s_g2, y_e_g2, x_s_g2, x_e_g2, angle);

					int offset_distance = get_offset_distance(_row_count, i);

					back_cropped_region = get_ROI_p(rotate_src,
													y_coordinate + 7,
													y_coordinate + NEEDLE_GRID_HIGH + 7,
													x_coordinate + NEEDLE_GRID_WIDTH * j + offset_distance,
													x_coordinate + NEEDLE_GRID_WIDTH * (j + 1) + offset_distance);

					x_s_g1 = y_coordinate + 7;
					x_e_g1 = y_coordinate + NEEDLE_GRID_HIGH + 7;
					y_s_g1 = x_coordinate + NEEDLE_GRID_WIDTH * j + offset_distance;
					y_e_g1 = x_coordinate + NEEDLE_GRID_WIDTH * (j + 1) + offset_distance;

					x_s_g2 = y_coordinate + 7;
					x_e_g2 = y_coordinate + NEEDLE_GRID_HIGH + 7;
					y_s_g2 = x_coordinate + NEEDLE_GRID_WIDTH * j + offset_distance - 5;
					y_e_g2 = x_coordinate + NEEDLE_GRID_WIDTH * (j + 1) + offset_distance + 5;

					back_cropped_region_group2 = get_ROI_p(rotate_src, x_s_g2, x_e_g2, y_s_g2, y_e_g2);

					back_cropped_region_group3 = get_masked_ROI(src, rotate_src, y_s_g1, y_e_g1, x_s_g1, x_e_g1, angle);

					back_cropped_region_group4 = get_masked_ROI(src, rotate_src, y_s_g2, y_e_g2, x_s_g2, x_e_g2, angle);

				}

				// 后针排需要旋转180后送入NN
				cv::flip(back_cropped_region, filp_back_group1, -1);

				cv::flip(back_cropped_region_group2, filp_back_group2, -1);

				// 修正为真实坐标保存
				save_image("C:/data/original/group1/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/front/" + std::to_string(ZERO_INDEX_FIX + NEEDLE_INDEX_FIX + stitch_index + (uart.direction == R ? j : -j)) + ".bmp", front_cropped_region.clone());
				save_image("C:/data/original/group1/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/back/" + std::to_string(ZERO_INDEX_FIX + NEEDLE_INDEX_FIX + stitch_index + (uart.direction == R ? j : -j)) + ".bmp", filp_back_group1.clone());
				
				save_image("C:/data/original/group2/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/front/" + std::to_string(ZERO_INDEX_FIX + NEEDLE_INDEX_FIX + stitch_index + (uart.direction == R ? j : -j)) + ".bmp", front_cropped_region_group2.clone());
				save_image("C:/data/original/group2/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/back/" + std::to_string(ZERO_INDEX_FIX + NEEDLE_INDEX_FIX + stitch_index + (uart.direction == R ? j : -j)) + ".bmp", filp_back_group2.clone());
				
				save_image("C:/data/original/group3/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/front/" + std::to_string(ZERO_INDEX_FIX + NEEDLE_INDEX_FIX + stitch_index + (uart.direction == R ? j : -j)) + ".bmp", front_cropped_region_group3.clone());
				save_image("C:/data/original/group3/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/back/" + std::to_string(ZERO_INDEX_FIX + NEEDLE_INDEX_FIX + stitch_index + (uart.direction == R ? j : -j)) + ".bmp", back_cropped_region_group3.clone());
				
				save_image("C:/data/original/group4/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/front/" + std::to_string(ZERO_INDEX_FIX + NEEDLE_INDEX_FIX + stitch_index + (uart.direction == R ? j : -j)) + ".bmp", front_cropped_region_group4.clone());
				save_image("C:/data/original/group4/" + std::to_string(_row_count + WASTE_YARN_GAP) + "/back/" + std::to_string(ZERO_INDEX_FIX + NEEDLE_INDEX_FIX + stitch_index + (uart.direction == R ? j : -j)) + ".bmp", back_cropped_region_group4.clone());


				// 实际检测逻辑
				// batch predict的逻辑
				for (int k = 0; k < 2; k++) {
					if (k == 0)
						needle_set[i][k].src[stitch_index + (uart.direction == R ? j : -j)] = front_cropped_region;
					else
						needle_set[i][k].src[stitch_index + (uart.direction == R ? j : -j)] = back_cropped_region;
					// 更新结尾索引
					needle_set[i][k].end = stitch_index + (uart.direction == R ? j : -j);

					// 更新起始索引
					if (row_changed_flag)
						needle_set[i][k].start = stitch_index;
				}
				row_changed_flag = false;
			}
		}
	} while (false);
}

bool ptreate::focus_detection(cv::Mat& src)
{
	cv::Mat dst;
	cv::Scalar mean;
	cv::Scalar stddev;

	cv::Laplacian(src, dst, CV_8U);
	cv::meanStdDev(dst, mean, stddev);
	float res = stddev.val[0];
	return res > FOCUS_THRESHOLD ? true : false;
}

bool ptreate::brightness_detection(cv::Mat& src)
{
	double minv = 0.0, maxv = 0.0;
	double* minp = &minv;
	double* maxp = &maxv;
	cv::minMaxIdx(src, minp, maxp);
	return ((minv > BRIGHTNESS_LOWER_THRESHOLD) && (maxv > BRIGHTNESS_UPPER_THRESHOLD)) ? true : false;
}

void ptreate::cam_test(cv::Mat& src)
{
	focus_detection(src) ? printf_s("Focus test pass\n") : printf_s("Focus test error\n");
	brightness_detection(src) ? printf_s("Brightness test pass\n") : printf_s("Brightness test error\n");
}

bool ptreate::data_validity_check()
{
	// TODO: 优化图像丢帧的处理措施
	if (uart_data.flag == 0xBB) {
		// TODO:BB预警处理机制待定
		running_log_txt << "Uart data in BB status" << std::endl;
		return false;
	}
	// TODO: 跳变导致的串口图像不匹配处理方案：缺失位置做丢弃处理,(目前只考虑送入图片帧号只会向后突变的情况)
	// 监测图像帧是否会跳变
	if (cam_data.frame_num != ++pre_cam_frame) {
		cam_queue.push_f(cam_data);
		drop_cam_frame_count++;
		running_log_txt << "Cam frame mutation" << std::endl;
		return false;
	}

	// 监测串口帧是否会跳变
	if (uart_data.frame_num != ++pre_uart_frame) {
		uart_queue.push_f(uart_data);
		drop_uart_frame_count++;
		running_log_txt << "Uart frame mutation" << std::endl;
		return false;
	}
	return true;
}

void ptreate::row_count_record(uart_group_data& uart)
{
	do {
		// 只在生成和执行阶段更新行信息
		if (gpm_cur_status != gentable && gpm_cur_status != execute)
			break;
		_cur_position.direction = uart.direction;
		_cur_position.row = uart.row;
		_cur_position.col = uart.col;

		// 当前方向与前一次方向不同时进行行号检测
		if (_cur_position.row != _last_position.row) {
			// 行增加且在执行阶段将各个光路的图片送入NN线程预测
			if (gpm_cur_status == execute) {
				if (_row_count + WASTE_YARN_GAP >= template_max_row)
					knit_status = false;
				
				if (knit_status) {
					// 光路循环
					for (int i = 0; i < LIGHT_PATH_COUNT; i++) {
						// 当前光路无效直接跳过
						if (std::find(VALID_LIGHT_PATH.begin(), VALID_LIGHT_PATH.end(), i) == VALID_LIGHT_PATH.end() ||
							_clothes_template[("row" + std::to_string(_row_count + WASTE_YARN_GAP)).c_str()][LIGHT_PATH_MAPPING[i].c_str()].FindMember("direction") ==
							_clothes_template[("row" + std::to_string(_row_count + WASTE_YARN_GAP)).c_str()][LIGHT_PATH_MAPPING[i].c_str()].MemberEnd())
							continue;

						// 前后针排循环
						for (int j = 0; j < 2; j++) {
							needle_set[i][j].row = _row_count;
							needle_set[i][j].light_path = i;
							needle_set[i][j].needle_position = j;
							if (needle_set[i][j].start > needle_set[i][j].end)
								std::swap(needle_set[i][j].start, needle_set[i][j].end);
							needle_queue.push_b(needle_set[i][j]);
						}
					}
				}
			}

			// 制作偏移表在左行时复位
			if (gpm_cur_status == gentable && offset_table_start_flag) {
				if (_cur_position.direction == L) {
					_row_count = 0;
					offset_table_start_flag = false;
				}
			}

			_row_count++;
			_last_position.direction = _cur_position.direction;
			_last_position.row = _cur_position.row;
			row_changed_flag = true;
			printf_s("检测到换行！当前行:%d, 修正行号:%d, 内部行号:%d\n", _cur_position.row, _row_count, _row_count + WASTE_YARN_GAP);
		}
	} while (false);
}

bool ptreate::look_up_needle_category(int row, int light_path, int side, int index)
{
	if (side == 0)
		return _clothes_template[("row" + std::to_string(row)).c_str()][LIGHT_PATH_MAPPING[light_path].c_str()]["front_needle"][index + NEEDLE_INDEX_FIX].GetInt();
	else
		return _clothes_template[("row" + std::to_string(row)).c_str()][LIGHT_PATH_MAPPING[light_path].c_str()]["back_needle"][index + NEEDLE_INDEX_FIX].GetInt();
}

bool ptreate::compare(int c_res, int row, int light_path, int side, int index)
{
	bool t_res = look_up_needle_category(row + WASTE_YARN_GAP, light_path, side, index);
	if (side == 0)
		detection_log_txt << "@前针排检测@" << "行:" << row + WASTE_YARN_GAP << " 索引:" << index + 1 << " 模板:" << t_res << " 检测结果:" << c_res << std::endl;
	else
		detection_log_txt << "@后针排检测@" << "行:" << row + WASTE_YARN_GAP << " 索引:" << index + 1 << " 模板:" << t_res << " 检测结果:" << c_res << std::endl;
	return t_res == c_res ? true : false;
}

void ptreate::detection()
{
	while (!needle_ans_queue.Empty()) {
		// TODO: 所有索引需要统一
		// TODO: 后期加入后针排只检测有效编织区域的图像，
		needle_ans_queue.pop(detected_res);
		//printf_s("当前行:%d, 检测结果起始：%d， 结束：%d\n",detected_res.row + WASTE_YARN_GAP, detected_res.start, detected_res.end);

		for (int i = std::max(detected_res.start, NEEDLE_FREE_START); i <= std::min(detected_res.end, NEEDLE_FREE_END); i++) {
			// 纱线位置是真实索引需要+2修正
			if (yarn_positon["row" + std::to_string(detected_res.row + WASTE_YARN_GAP)][LIGHT_PATH_MAPPING[detected_res.light_path]].find(i + ZERO_INDEX_FIX + NEEDLE_INDEX_FIX) == yarn_positon["row" + std::to_string(detected_res.row + WASTE_YARN_GAP)][LIGHT_PATH_MAPPING[detected_res.light_path]].end()
				&& valid_region_check(i + NEEDLE_INDEX_FIX, valid_region["row" + std::to_string(detected_res.row + WASTE_YARN_GAP)][LIGHT_PATH_MAPPING[detected_res.light_path]][VALID_REGION_MAPPING[detected_res.needle_position]])
				&& !compare(detected_res.ans[i], detected_res.row, detected_res.light_path, detected_res.needle_position, i)){
				if (detected_res.needle_position == 0)
					save_image("C:/data/res/front/" + std::to_string(detected_res.ans[i]) + "/" + std::to_string(detected_res.row + WASTE_YARN_GAP) + "-" + std::to_string(i + NEEDLE_INDEX_FIX + ZERO_INDEX_FIX) + ".bmp", detected_res.src[i]);
				else
					save_image("C:/data/res/back/" + std::to_string(detected_res.ans[i]) + "/" + std::to_string(detected_res.row + WASTE_YARN_GAP) + "-" + std::to_string(i + NEEDLE_INDEX_FIX + ZERO_INDEX_FIX) + ".bmp", detected_res.src[i]);

				// TODO:上报EMS错误，内部错误统计
				//printf_s("Row:%d Col:%d detected difference.\n", detected_res.row, i);
				if (!continuous_flag)
					total_continuous_defect_count == 0;

				total_defect_count++;
				total_continuous_defect_count++;
				continuous_flag = true;
			}
			else
				continuous_flag = false;

			// 缺陷超阈值停机
			if (total_defect_count > MAX_DEFECT_COUNT || total_continuous_defect_count > MAX_CONTINUOUS_DEFECT_COUNT)
			{
				// TODO:上报EMS错误，下报MCU停机，是否直接切换状态到idel还是等待EMS下发停止指令进行停机
				//printf_s("Defect has exceeds default value\n");
			}
		}
	}
}

int ptreate::needle_classification(int row, int light_path, int side, int index)
{
	if (yarn_positon["row" + std::to_string(row)][LIGHT_PATH_MAPPING[light_path]].find(index) == yarn_positon["row" + std::to_string(row)][LIGHT_PATH_MAPPING[light_path]].end() &&
		index > NEEDLE_FREE_START && index < NEEDLE_FREE_END)
		return look_up_needle_category(row, light_path, side, index) ? 1 : 0;
	else
		return -1;
}

void ptreate::position_test(cv::Mat& src, uart_group_data uart)
{
	cv::Mat src_copy = src.clone();
	util::create_dir("C:/data/position/" + std::to_string(uart.direction) + std::to_string(uart.col));
	save_image("C:/data/position/" + std::to_string(uart.direction) + std::to_string(uart.col) + "/" + std::to_string(uart.row) + ".bmp", src_copy);
}
/***********************************************************************************/

/*************************************** 工具 **************************************/
sub_offset_data ptreate::format_map_data(uart_group_data& uart, int start_x, int shift_count, int stitch_index)
{
	sub_offset_data res{ uart.direction, uart.col };

	res._offset_data._x_coordinate = start_x;
	res._offset_data._shift_count = shift_count;
	res._offset_data._stitch_index = stitch_index;
	return res;
}

int ptreate::get_offset_distance(int row, int path)
{
	std::string back_offset = _clothes_template[("row" + std::to_string(row + WASTE_YARN_GAP)).c_str()][LIGHT_PATH_MAPPING[path].c_str()]["offset"].GetString();
	float res = 0;
	// 目前偏移均依据织针的实际位置偏移来修正:"N"的位置不需要修正
	if (back_offset[0] == 'U')
		res += 0.25;
	else if (back_offset[0] == '#')
		res += 0.5;
	// TODO: 目前后针排移动方向颠倒，待实测(待截取区域是否会超出实际图像边界？)
	if (back_offset.size() == 4)
		res += back_offset[2] == 'R' ? -(back_offset[3] - '0') : (back_offset[3] - '0');
	res *= NEEDLE_GRID_WIDTH;

	return static_cast<int>(std::round(res));
}

void ptreate::save_image(std::string file_name, cv::Mat& src)
{
	cv::imwrite(file_name, src);
}

void ptreate::dbg(int step)
{
	printf_s("Cam queue:%d, Uart queue:%d\n", cam_queue.Size(), uart_queue.Size());
	printf_s("Cur cam frame:%d, Cur uart frame:%d\n", cam_data.frame_num, uart_data.frame_num);
	printf_s("Get One UartInfo, Flag[%d], Direction[%d], Row[%d], Col[%d], Frame Num[%d], Cur row:%d\n", uart_data.flag, uart_data.direction, uart_data.row, uart_data.col, uart_data.frame_num, _row_count + WASTE_YARN_GAP);
	debug_log_txt << "Get One Uart Info" << "Flag:" << uart_data.flag << " Direction:" << uart_data.direction << " Row:" << uart_data.row << " Col:" << uart_data.col << " Frame Num:" << uart_data.frame_num << " Cur row:" << _row_count + WASTE_YARN_GAP << std::endl;
	//if (!(uart_data.frame_num % step)) {
	//	//printf_s("Get One UartInfo, Flag[%d], Direction[%d], Row[%d], Col[%d], Frame Num[%d], Cur row:%d\n", uart_data.flag, uart_data.direction, uart_data.row, uart_data.col, uart_data.frame_num, _row_count + WASTE_YARN_GAP);
	//	printf_s("Cam queue:%d, Uart queue:%d\n", cam_queue.Size(), uart_queue.Size());
	//	printf_s("Cur cam frame:%d, Cur uart frame:%d\n", cam_data.frame_num, uart_data.frame_num);
	//	//printf_s("Cur status %d\n", gpm_cur_status);
	//	//printf_s("Cur row:%d, Cur cam frame:%d, Cur uart frame:%d, Cam drop count:%d, Uart drop count:%d\n", _row_count, cam_data.frame_num, uart_data.frame_num, drop_cam_frame_count, drop_uart_frame_count);
	//}
}
/***********************************************************************************/

/*************************************** 执行 **************************************/
void ptreate::init()
{
	// 加载偏移表
	load_offset_table(OFFSET_TABLE_PATH);

	// 加载模板
	load_template(TEMPLATE_PATH);

	// 解析沙嘴
	decode_yarn();
}

void ptreate::run()
{
	while (!cam_queue.Empty() && !uart_queue.Empty() && !IsTerminated()) {
		// 获取图像数据
		cam_queue.pop(cam_data);
		// 获取串口数据
		uart_queue.pop(uart_data);

		// Debug打印
		dbg(10);

		// 数据有效性检测
		if (!data_validity_check())
			continue;

		// 更新行号
		row_count_record(uart_data);

		switch (gpm_cur_status) {
			// 上电状态初始化为wait
			// 等待状态
			case wait:
			// 空闲状态
			case idle:
				position_test(cam_data.img, uart_data);
				break;
			// 锁定状态
			case slock:
				Sleep(1);
				break;

			// 校准状态
			case calibrate:
				auto_calibration(cam_data.img);
				break;
			// 制作偏移表状态
			case gentable:
				make_offset_table(cam_data.img, uart_data);
				break;

			// 执行状态(预留)
			case execute:
			// 切图状态
			case split:
				split_image(cam_data.img, uart_data);
			// 检测状态
			case detect:
				detection();
				break;
			// 测试状态
			case test:
				cam_test(cam_data.img);
				break;
			default:
				break;
		}
	}
}
/***********************************************************************************/

/*************************************** 线程 **************************************/
void ptreate::Execute()
{
	// 预处理初始化
	init();
	while (!IsTerminated()) {
		Wait();
		if (IsTerminated())
			return;
		ResetEvent();
		run();
		SetEvent();
	}
}
