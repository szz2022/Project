#pragma once

#include "../common/header.h"

namespace ptreate
{
	// Image Processing
	cv::Mat rotate_image(cv::Mat& src, float angle, bool w_gpu);
	cv::Point get_rerotate_point(cv::Mat& src, cv::Mat& dst, int x, int y, float angle);
	cv::Mat get_masked_ROI(cv::Mat& src, cv::Mat& rotate_src, int x_s, int x_e, int y_s, int y_e, double angle);
	cv::Mat scale_image(cv::Mat& src, float scale);
	cv::Mat get_ROI(cv::Mat& src, int x, int y, int width, int height);
	cv::Mat get_ROI_p(cv::Mat& src, int x_s, int x_e, int y_s, int y_e);
	cv::Mat gamma_transform(cv::Mat& src, float fGamma, float fc);
	cv::Mat equalize_hist(cv::Mat& src);
	cv::Mat bin_threashold(cv::Mat& src, int thresh, int maxval);
	cv::Mat edge_detection(cv::Mat& src, int min_threshold, int max_threshold);
	vector<cv::Vec4i> hough_line_detection(cv::Mat& src, int threshold, int minLineLength, int maxLineGap);

	// Custom Algorithm
	VI k_means(VI data);
	int search(VI group, int target);
	VI center_filter(VI centers);
	g_data<float> k_detection(vector<cv::Vec4i>& lines, int lower, int upper);
	g_data<vector<PII>> line_cluster(vector<cv::Vec4i>& lines);
	bool valid_region_check(int position, vector<VI> region_set);
};
