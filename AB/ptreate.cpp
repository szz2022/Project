#include "ptreate.h"

/************************************* 图像处理 *************************************/
cv::Mat ptreate::rotate_image(cv::Mat& src, float angle, bool w_gpu)
{
	cv::Mat dst;

	int width = src.cols;
	int height = src.rows;
	int center_x = static_cast<int>(width / 2);
	int center_y = static_cast<int>(height / 2);

	auto transform_element = cv::getRotationMatrix2D(cv::Point(center_x, center_y), angle, 1);

	double cos = fabs(transform_element.at<double>(0, 0));
	double sin = fabs(transform_element.at<double>(0, 1));

	auto height_new = width * sin + height * cos;
	auto width_new = height * sin + width * cos;
	
	transform_element.at<double>(0, 2) += width_new / 2 - center_x;
	transform_element.at<double>(1, 2) += height_new / 2 - center_y;
	if (w_gpu) {
		cv::cuda::GpuMat gpu_in;
		cv::cuda::GpuMat gpu_out;

		gpu_in.upload(src);
		cv::cuda::warpAffine(gpu_in, gpu_out, transform_element, cv::Size(width_new, height_new));
		gpu_out.download(dst);
	}
	else
		cv::warpAffine(src, dst, transform_element, cv::Size(width_new, height_new), cv::INTER_NEAREST);
	return dst;
}

cv::Point ptreate::get_rerotate_point(cv::Mat& src, cv::Mat& dst, int x, int y, float angle)
{
	int src_width = src.cols;
	int src_height = src.rows;
	float src_center_x = src_width / 2;
	float src_center_y = src_height / 2;

	int dst_width = dst.cols;
	int dst_height = dst.rows;
	float dst_center_x = dst_width / 2;
	float dst_center_y = dst_height / 2;

	float gap_x = dst_center_x - src_center_x;
	float gap_y = dst_center_y - src_center_y;

	float x_coordinate = x - gap_x - src_center_x;
	float y_coordinate = y - gap_y - src_center_y;

	auto transform_element = cv::getRotationMatrix2D(cv::Point(src_center_x, src_center_y), angle, 1);

	double cos = transform_element.at<double>(0, 0);
	double sin = transform_element.at<double>(1, 0);
	int new_x = x_coordinate * cos + y_coordinate * sin + src_center_x;
	int new_y = y_coordinate * cos - x_coordinate * sin + src_center_y;

	cv::Point res(new_x, new_y);
	return res;
}

cv::Mat ptreate::get_masked_ROI(cv::Mat& src, cv::Mat& rotate_src, int x_s, int x_e, int y_s, int y_e, double angle)
{
	cv::Mat res;

	cv::Point p1 = get_rerotate_point(src, rotate_src, x_s, y_s, angle);
	cv::Point p2 = get_rerotate_point(src, rotate_src, x_e, y_s, angle);
	cv::Point p3 = get_rerotate_point(src, rotate_src, x_e, y_e, angle);
	cv::Point p4 = get_rerotate_point(src, rotate_src, x_s, y_e, angle);

	vector<cv::Point> anchor{ p1, p2, p3, p4 };
	auto min_rect = cv::minAreaRect(anchor);
	auto max_rect = cv::boundingRect(anchor);

	cv::Mat mask(max_rect.height, max_rect.width, CV_8UC1, cv::Scalar(0));

	if ((p1.x > src.cols || p1.x < 0) || (p1.y > src.rows || p1.y < 0))
	{
		return mask;
	}
	if ((p2.x > src.cols || p2.x < 0) || (p2.y > src.rows || p2.y < 0))
	{
		return mask;
	}
	if ((p3.x > src.cols || p3.x < 0) || (p3.y > src.rows || p3.y < 0))
	{
		return mask;
	}
	if ((p4.x > src.cols || p4.x < 0) || (p4.y > src.rows || p4.y < 0))
	{
		return mask;
	}

	for (int i = max_rect.x; i < max_rect.x + max_rect.width; i++) {
		for (int j = max_rect.y; j < max_rect.y + max_rect.height; j++) {
			if (cv::pointPolygonTest(anchor, cv::Point(i, j), false) >= 0)
				mask.at<uchar>(j - max_rect.y, i - max_rect.x) = 1;
		}
	}
	auto ROI = ptreate::get_ROI(src, max_rect.x, max_rect.y, max_rect.width, max_rect.height);
	cv::bitwise_and(ROI, ROI, res, mask);
	return res;
}

cv::Mat ptreate::scale_image(cv::Mat& src, float scale)
{
	cv::Mat dst;
	cv::resize(src, dst, cv::Size(0, 0), scale, scale, cv::INTER_LINEAR);
	return dst;
}

cv::Mat ptreate::get_ROI(cv::Mat& src, int x, int y, int width, int height)
{
	return src(cv::Rect(x, y, width, height));
}

cv::Mat ptreate::get_ROI_p(cv::Mat& src, int x_s, int x_e, int y_s, int y_e)
{
	return src(cv::Range(x_s, x_e), cv::Range (y_s, y_e));
}

cv::Mat ptreate::gamma_transform(cv::Mat& src, float fGamma, float fc)
{
	assert(src.elemSize() == 1);
	cv::Mat dst = cv::Mat::zeros(src.rows, src.cols, src.type());

	float fNormalFactor = 1.0f / 255.0f;

	vector<unsigned char> lookUp(256);
	for (int m = 0; m < lookUp.size(); m++) {
		float fOutput = pow(m * fNormalFactor, fGamma) * fc;
		fOutput = fOutput > 1.0f ? 1.0f : fOutput;
		lookUp[m] = static_cast<unsigned char>(fOutput * 255.0f);
	}

	for (int r = 0; r < src.rows; r++) {
		unsigned char* pInput = src.data + r * src.step[0];
		unsigned char* pOutput = dst.data + r * dst.step[0];
		for (int c = 0; c < src.cols; c++)
			pOutput[c] = lookUp[pInput[c]];
	}
	return dst;
}

cv::Mat ptreate::equalize_hist(cv::Mat& src)
{
	cv::Mat dst;
	cv::equalizeHist(src, dst);
	return dst;
}

cv::Mat ptreate::bin_threashold(cv::Mat& src, int thresh, int maxval)
{
	cv::Mat dst;
	cv::threshold(src, dst, thresh, maxval, cv::THRESH_BINARY);
	return dst;
}

cv::Mat ptreate::edge_detection(cv::Mat& src, int min_threshold, int max_threshold)
{
	cv::Mat dst;
	cv::Canny(src, dst, min_threshold, max_threshold, 3, true);
	return dst;
}

vector<cv::Vec4i> ptreate::hough_line_detection(cv::Mat& src, int threshold, int minLineLength, int maxLineGap)
{
	vector<cv::Vec4i> lines;
	cv::HoughLinesP(src, lines, 1, CV_PI / 180, threshold, minLineLength, maxLineGap);
	return lines;
}
/***********************************************************************************/

/*************************************** 算法 **************************************/
VI ptreate::k_means(VI data)
{
	vector<pair<float, vector<float>>> cluster;
	VI res;
	for (auto& var : data) {
		bool changed = false;
		for (auto& c : cluster) {
			if (c.first - 2 < var && var < c.first + 7) {
				c.second.push_back(var);
				c.first = accumulate(c.second.begin(), c.second.end(), 0) / c.second.size();
				changed = true;
				break;
			}
		}
		if (!changed) {
			pair<float, vector<float>> sub_data;
			sub_data.first = var;
			sub_data.second.push_back(var);
			cluster.push_back(sub_data);
		}
	}

	for (auto& c : cluster)
		res.push_back(static_cast<int>(round(c.first)));
	return res;
}

int ptreate::search(VI group, int target)
{
	if (target <= group[0])
		return group[0];
	if (target >= group[group.size() - 1])
		return group[group.size() - 1];

	int left = 0;
	int right = group.size() - 1;
	while (left < right) {
		int mid = left + ((right - left + 1) >> 1);
		if (group[mid] > target)
			right = mid - 1;
		else
			left = mid;
	}
	return abs(group[left] - target) < abs(group[left + 1] - target) ? group[left] : group[left + 1];
}

// 算法的鲁棒性待验证，目前只能作为辅助手段，因为实际针格存在破损或畸形，无法按照固定距离来处理
VI ptreate::center_filter(VI centers)
{
	VI true_center;
	deque<pair<int, int>> queue;
	VI shadow_queue;

	pair<int, int> index(0, 1);
	queue.push_back(index);
	shadow_queue.push_back(index.first * 10 + index.second);
	while (!queue.empty()) {
		auto &ind = queue.front();
		queue.pop_front();
		int cur_p = ind.first;
		int next_p = ind.second;

		if (next_p >= centers.size())
			continue;

		if (25 <= centers[next_p] - centers[cur_p] && centers[next_p] - centers[cur_p] <= 37) {
			true_center.push_back(centers[cur_p]);
			if (next_p == centers.size() - 1)
				true_center.push_back(centers[next_p]);
		}
		else {
			pair<int, int> used(cur_p, next_p + 1);
			if (find(shadow_queue.begin(), shadow_queue.end(), cur_p * 10 + next_p + 1) != shadow_queue.end()) {
				queue.push_back(used);
				shadow_queue.push_back(cur_p * 10 + next_p + 1);
			}
		}
		pair<int, int> used(next_p, next_p + 1);
		if (find(shadow_queue.begin(), shadow_queue.end(), next_p * 10 + next_p + 1) == shadow_queue.end()) {
			queue.push_back(used);
			shadow_queue.push_back(next_p * 10 + next_p + 1);
		}
	}
	return true_center;
}

g_data<float> ptreate::k_detection(vector<cv::Vec4i>& lines, int lower, int upper)
{
	g_data<float> res{};
	res.state = false;
	res.data = 0.0;
	vector<float> degrees;

	for (auto &line : lines) {
		if (line[2] - line[0] == 0)
			continue;
		float k = static_cast<float>((line[3] - line[1])) / static_cast<float>((line[2] - line[0]));
		float degree = (atan(k)) * 180.0 / CV_PI;

		if (lower < degree && degree < upper)
			degrees.push_back(degree);
	}
	if (degrees.empty())
		return res;
	float sum = accumulate(begin(degrees), end(degrees), 0.0);
	float degree_mean = sum / degrees.size() + 90;
	res.state = true;
	res.data = degree_mean;
	return res;
}

g_data<vector<PII>> ptreate::line_cluster(vector<cv::Vec4i>& lines)
{
	g_data<vector<PII>> res;
	res.state = false;

	for (auto &line : lines) {
		if (abs(line[1] - line[3]) <= 3 && abs(line[0] - line[2]) >= 5)
			continue;

		if (line[1] > line[3])
			swap(line[1], line[3]);

		if (res.data.empty()) {
			pair<int, int> sub_data(line[1], line[3]);
			res.data.push_back(sub_data);
			continue;
		}
		bool change_flag = true;
		for (auto &cluster : res.data) {
			if ((cluster.first <= line[1] && line[1] <= cluster.second) || (cluster.first <= line[3] && line[3] <= cluster.second) ||
				(line[1] <= cluster.first && cluster.first <= line[3]) || (line[1] <= cluster.second && cluster.second <= line[3])) {
				cluster.first = min({ cluster.first, line[1], line[3] });
				cluster.second = max({ cluster.second, line[1], line[3] });
				change_flag = false;
			}
		}
		if (change_flag) {
			pair<int, int> sub_data(line[1], line[3]);
			res.data.push_back(sub_data);
		}
	}
	if (res.data.size() != 2)
		return res;
	else {
		res.state = true;
		return res;
	}
}

bool ptreate::valid_region_check(int position, vector<VI> region_set)
{
	for (auto& region : region_set) {
		if (position >= region[0] && position <= region[1])
			return true;
	}
	return false;
}
/***********************************************************************************/