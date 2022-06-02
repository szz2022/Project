/**
 * @file pt_operator.h
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

namespace ptreate
{
	// Image Processing
	/**
	 * @brief
	 *
	 * @param src
	 * @param angle
	 * @param w_gpu
	 * @return cv::Mat
	 */
	cv::Mat rotate_image(cv::Mat &src, float angle, bool w_gpu);

	/**
	 * @brief Get the rerotate point object
	 *
	 * @param src
	 * @param dst
	 * @param x
	 * @param y
	 * @param angle
	 * @return cv::Point
	 */
	cv::Point get_rerotate_point(cv::Mat &src, cv::Mat &dst, int x, int y, float angle);

	/**
	 * @brief Get the masked ROI object
	 *
	 * @param src
	 * @param rotate_src
	 * @param x_s
	 * @param x_e
	 * @param y_s
	 * @param y_e
	 * @param angle
	 * @return cv::Mat
	 */
	cv::Mat get_masked_ROI(cv::Mat &src, cv::Mat &rotate_src, int x_s, int x_e, int y_s, int y_e, double angle);

	/**
	 * @brief
	 *
	 * @param src
	 * @param scale
	 * @return cv::Mat
	 */
	cv::Mat scale_image(cv::Mat &src, float scale);

	/**
	 * @brief
	 *
	 * @param src
	 * @param x
	 * @param y
	 * @param width
	 * @param height
	 * @return cv::Mat
	 */
	cv::Mat get_ROI(cv::Mat &src, int x, int y, int width, int height);

	/**
	 * @brief Get the ROI p object
	 *
	 * @param src
	 * @param x_s
	 * @param x_e
	 * @param y_s
	 * @param y_e
	 * @return cv::Mat
	 */
	cv::Mat get_ROI_p(cv::Mat &src, int x_s, int x_e, int y_s, int y_e);

	/**
	 * @brief
	 *
	 * @param src
	 * @param fGamma
	 * @param fc
	 * @return cv::Mat
	 */
	cv::Mat gamma_transform(cv::Mat &src, float fGamma, float fc);

	/**
	 * @brief
	 *
	 * @param src
	 * @return cv::Mat
	 */
	cv::Mat equalize_hist(cv::Mat &src);

	/**
	 * @brief
	 *
	 * @param src
	 * @param thresh
	 * @param maxval
	 * @return cv::Mat
	 */
	cv::Mat bin_threashold(cv::Mat &src, int thresh, int maxval);

	/**
	 * @brief
	 *
	 * @param src
	 * @param min_threshold
	 * @param max_threshold
	 * @return cv::Mat
	 */
	cv::Mat edge_detection(cv::Mat &src, int min_threshold, int max_threshold);

	/**
	 * @brief
	 *
	 * @param src
	 * @param threshold
	 * @param minLineLength
	 * @param maxLineGap
	 * @return vector<cv::Vec4i>
	 */
	std::vector<cv::Vec4i> hough_line_detection(cv::Mat &src, int threshold, int minLineLength, int maxLineGap);

	// Custom Algorithm
	/**
	 * @brief 依据图像针格特征方向计算出针排相对水平的逆时针旋转角度
	 *
	 * @param src 原始图片
	 * @return double 旋转角度
	 */
	float cal_rotate_angle(cv::Mat &src);

	/**
	 * @brief 依据图像特征计算出前后针排中间的分界中线坐标
	 *
	 * @param src 原始图片
	 * @return int 分界区域坐标
	 */
	int cal_split_midline(cv::Mat &src);

	/**
	 * @brief 寻找左右特征区域的起始位置
	 *
	 * @param src 特征区域图片(具体区域待定:全图 or 某参数表中设置好的小区域)
	 * @param r_direction 机头运行方向(不同方向特征对称，需要做一次判断)
	 * @return int 返回特征分界线的坐标
	 */
	int search_start_feature(cv::Mat &src, bool r_direction);

	/**
	 * @brief 获取输入图像中的针格边缘坐标
	 *
	 * @param src 特诊区域图片(依据上次最后一个针格坐标而选取的区域，选取逻辑为[x_end - w / 2, x_end + cnt * w + w / 2])
	 * @return VI 返回一组图像中找到的边缘坐标
	 */
	VI search_region_edge(cv::Mat &src);

	/**
	 * @brief 修正默认针格边缘为真实图像边缘
	 *
	 * @param src 特征区域图片(目前区域为[x_default - w / 2, x_default + w / 2])
	 * @param default_edge 默认针格边缘坐标
	 * @return int 修正后的针格坐标
	 */
	PII modify_edge(cv::Mat &src, int default_edge);

	/**
	 * @brief 监测当前图片的清晰度是否在阈值范围(阈值设置在参数配置模块中)内
	 *
	 * @param src 到达特征区域的图像
	 * @return true 正常
	 * @return false 模糊
	 */
	bool focus_detection(cv::Mat &src);

	/**
	 * @brief 监测当前图片的亮度值是否在阈值范围(阈值设置在参数配置模块中)内
	 *
	 * @param src 到达特征区域的图像
	 * @return int 0:正常，-1:过暗，1:过亮
	 */
	int brightness_detection(cv::Mat &src);

	/**
	 * @brief (Abandoned)
	 *
	 * @param data
	 * @return VI
	 */
	VI k_means(VI data);

	/**
	 * @brief
	 *
	 * @param group
	 * @param target
	 * @return int
	 */
	int search(VI group, int target);

	/**
	 * @brief (Abandoned)
	 *
	 * @param centers
	 * @return VI
	 */
	VI center_filter(VI centers);

	/**
	 * @brief (Abandoned)
	 *
	 * @param lines
	 * @param lower
	 * @param upper
	 * @return g_data<float>
	 */
	g_data<float> k_detection(std::vector<cv::Vec4i> &lines, int lower, int upper);

	/**
	 * @brief (Abandoned)
	 *
	 * @param lines
	 * @return g_data<vector<PII>>
	 */
	g_data<std::vector<PII>> line_cluster(std::vector<cv::Vec4i> &lines);

	/**
	 * @brief (Abandoned)
	 * 
	 * @param position 
	 * @param region_set 
	 * @return true 
	 * @return false 
	 */
	bool valid_region_check(int position, std::vector<VI> region_set);
};
