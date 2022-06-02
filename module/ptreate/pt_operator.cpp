/**
 * @file pt_operator.cpp
 * @author lty (302702801@qq.com)
 * @brief
 * @version 0.1
 * @date 2022-04-27
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "pt_operator.h"

/************************************* 图像处理 *************************************/
cv::Mat ptreate::rotate_image(cv::Mat &src, float angle, bool w_gpu)
{
	cv::Mat dst;

	int width = src.cols;
	int height = src.rows;
	int center_x = static_cast<int>(width / 2);
	int center_y = static_cast<int>(height / 2);

	auto transform_element = getRotationMatrix2D(cv::Point(center_x, center_y), angle, 1);

	double cos = fabs(transform_element.at<double>(0, 0));
	double sin = fabs(transform_element.at<double>(0, 1));

	auto height_new = width * sin + height * cos;
	auto width_new = height * sin + width * cos;

	transform_element.at<double>(0, 2) += width_new / 2 - center_x;
	transform_element.at<double>(1, 2) += height_new / 2 - center_y;
	if (w_gpu)
	{
		cv::cuda::GpuMat gpu_in;
		cv::cuda::GpuMat gpu_out;

		gpu_in.upload(src);
		cv::cuda::warpAffine(gpu_in, gpu_out, transform_element, cv::Size(width_new, height_new));
		gpu_out.download(dst);
	}
	else
		warpAffine(src, dst, transform_element, cv::Size(width_new, height_new), cv::INTER_NEAREST);
	return dst;
}

cv::Point ptreate::get_rerotate_point(cv::Mat &src, cv::Mat &dst, int x, int y, float angle)
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

	auto transform_element = getRotationMatrix2D(cv::Point(src_center_x, src_center_y), angle, 1);

	double cos = transform_element.at<double>(0, 0);
	double sin = transform_element.at<double>(1, 0);
	int new_x = x_coordinate * cos + y_coordinate * sin + src_center_x;
	int new_y = y_coordinate * cos - x_coordinate * sin + src_center_y;

	cv::Point res(new_x, new_y);
	return res;
}

cv::Mat ptreate::get_masked_ROI(cv::Mat &src, cv::Mat &rotate_src, int x_s, int x_e, int y_s, int y_e, double angle)
{
	cv::Mat res;

	cv::Point p1 = get_rerotate_point(src, rotate_src, x_s, y_s, angle);
	cv::Point p2 = get_rerotate_point(src, rotate_src, x_e, y_s, angle);
	cv::Point p3 = get_rerotate_point(src, rotate_src, x_e, y_e, angle);
	cv::Point p4 = get_rerotate_point(src, rotate_src, x_s, y_e, angle);

	std::vector<cv::Point> anchor{p1, p2, p3, p4};
	auto min_rect = minAreaRect(anchor);
	auto max_rect = boundingRect(anchor);

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

	for (int i = max_rect.x; i < max_rect.x + max_rect.width; i++)
	{
		for (int j = max_rect.y; j < max_rect.y + max_rect.height; j++)
		{
			if (pointPolygonTest(anchor, cv::Point(i, j), false) >= 0)
				mask.at<uchar>(j - max_rect.y, i - max_rect.x) = 1;
		}
	}
	auto ROI = ptreate::get_ROI(src, max_rect.x, max_rect.y, max_rect.width, max_rect.height);
	bitwise_and(ROI, ROI, res, mask);
	return res;
}

cv::Mat ptreate::scale_image(cv::Mat &src, float scale)
{
	cv::Mat dst;
	resize(src, dst, cv::Size(0, 0), scale, scale, cv::INTER_LINEAR);
	return dst;
}

cv::Mat ptreate::get_ROI(cv::Mat &src, int x, int y, int width, int height)
{
	return src(cv::Rect(x, y, width, height));
}

cv::Mat ptreate::get_ROI_p(cv::Mat &src, int x_s, int x_e, int y_s, int y_e)
{
	return src(cv::Range(x_s, x_e), cv::Range(y_s, y_e));
}

cv::Mat ptreate::gamma_transform(cv::Mat &src, float fGamma, float fc)
{
	assert(src.elemSize() == 1);
	cv::Mat dst = cv::Mat::zeros(src.rows, src.cols, src.type());

	float fNormalFactor = 1.0f / 255.0f;

	std::vector<unsigned char> lookUp(256);
	for (int m = 0; m < lookUp.size(); m++)
	{
		float fOutput = pow(m * fNormalFactor, fGamma) * fc;
		fOutput = fOutput > 1.0f ? 1.0f : fOutput;
		lookUp[m] = static_cast<unsigned char>(fOutput * 255.0f);
	}

	for (int r = 0; r < src.rows; r++)
	{
		unsigned char *pInput = src.data + r * src.step[0];
		unsigned char *pOutput = dst.data + r * dst.step[0];
		for (int c = 0; c < src.cols; c++)
			pOutput[c] = lookUp[pInput[c]];
	}
	return dst;
}

cv::Mat ptreate::equalize_hist(cv::Mat &src)
{
	cv::Mat dst;
	equalizeHist(src, dst);
	return dst;
}

cv::Mat ptreate::bin_threashold(cv::Mat &src, int thresh, int maxval)
{
	cv::Mat dst;
	threshold(src, dst, thresh, maxval, cv::THRESH_BINARY);
	return dst;
}

cv::Mat ptreate::edge_detection(cv::Mat &src, int min_threshold, int max_threshold)
{
	cv::Mat dst;
	Canny(src, dst, min_threshold, max_threshold, 3, true);
	return dst;
}

std::vector<cv::Vec4i> ptreate::hough_line_detection(cv::Mat &src, int threshold, int minLineLength, int maxLineGap)
{
	std::vector<cv::Vec4i> lines;
	HoughLinesP(src, lines, 1, CV_PI / 180, threshold, minLineLength, maxLineGap);
	return lines;
}

/****************************************************************************************/

/*************************************** 自定义算法 **************************************/
//比较轮廓面积(用来进行轮廓排序)
bool Contour_Area(std::vector<cv::Point> contour1, std::vector<cv::Point> contour2)
{
	return contourArea(contour1) > contourArea(contour2);
}

//比较轮廓周长(用来进行轮廓排序)
bool Contour_arcLength(std::vector<cv::Point> contour1, std::vector<cv::Point> contour2)
{
	return arcLength(contour1, true) > arcLength(contour2, true);
}

//比较直线x坐标
bool hline_x(cv::Vec4f line1, cv::Vec4f line2)
{
	return line1[0] < line2[0];
}

//比较矩形中心x坐标
bool Rect_x(cv::RotatedRect box1, cv::RotatedRect box2)
{
	return box1.center.x < box2.center.x;
}

void drawLine(cv::Mat &img, std::vector<cv::Vec2f> lines, double rows, double cols, cv::Scalar scalar, int n)
{
	cv::Point pt1, pt2;
	for (size_t i = 0; i < lines.size(); i++)
	{
		float rho = lines[i][0];   //直线距离坐标原点的距离
		float theta = lines[i][1]; //直线与坐标原点的垂线与x轴的夹角
		double a = cos(theta);	   //夹角的余弦
		double b = sin(theta);	   //夹角的正弦
		double x0 = a * rho;
		double y0 = b * rho;				  //直线与过坐标原点的垂线的交点
		double length = std::max(rows, cols); //图像高宽的最大值

		//计算直线上的一点
		pt1.x = cvRound(x0 - length * (-b));
		pt1.y = cvRound(y0 - length * (a));

		//计算直线上的另一点
		pt2.x = cvRound(x0 + length * (-b));
		pt2.y = cvRound(y0 + length * (a));

		//两点绘制一条直线
		line(img, pt1, pt2, scalar, n);
	}
}

// Auto calibration
float ptreate::cal_rotate_angle(cv::Mat &src)
{
	float angle = 0.0;
	cv::Mat threImg, blurImg, edgeImg, dst;
	std::vector<cv::Vec4i> hierachy;
	std::vector<std::vector<cv::Point>> contours, conts_dst;

	//预处理
	dst = src.clone();
	threshold(dst, threImg, 128, 255, cv::THRESH_BINARY);
	blur(threImg, blurImg, cv::Size(3, 3), cv::Point(-1, -1), cv::BORDER_DEFAULT);
	double highThre = 250;
	double lowThre = highThre / 3;
	Canny(blurImg, edgeImg, lowThre, highThre, 3, true);

	//轮廓提取
	findContours(edgeImg, contours, hierachy, cv::RETR_TREE, cv::CHAIN_APPROX_NONE, cv::Point(0, 0));
	for (size_t i = 0; i < contours.size(); i++)
	{
		double cArea = contourArea(contours[i]);
		if (cArea > 50)
		{
			conts_dst.push_back(contours[i]);
		}
	}

	drawContours(dst, conts_dst, -1, cv::Scalar(0, 0, 255), 1, cv::LINE_4); //调试

	// std::vector<cv::Rect> boundRect(contours.cv::size());  //定义外接矩形集合
	std::vector<cv::RotatedRect> dst_box;
	std::vector<cv::RotatedRect> box(contours.size()); //定义最小外接矩形集合
	cv::Point2f Rect[4];
	float rectLength = 0.0; //矩形长
	float rectWidth = 0.0;	//矩形宽
	for (size_t i = 0; i < conts_dst.size(); i++)
	{
		// boundRect[i] = boundingRect(cv::Mat(conts_dst[i]));
		// circle(dst,cv::Point(box[i].center.x, box[i].center.y), 3, cv::Scalar(0, 255, 0), -1, 8);  //绘制最小外接矩形的中心点
		// rectangle(dst, cv::Point(boundRect[i].x, boundRect[i].y), cv::Point(boundRect[i].x + boundRect[i].width, boundRect[i].y + boundRect[i].height), cv::Scalar(0, 255, 0), 2, 8);

		box[i] = minAreaRect(conts_dst[i]); //计算每个轮廓最小外接矩形
		box[i].points(Rect);				//把最小外接矩形四个端点复制给rect数组

		//根据长宽比，提取棱的外接矩形
		int ratio = 20;
		rectLength = (float)(sqrt(pow((Rect[0].x - Rect[1].x), 2)) + sqrt(pow((Rect[0].y - Rect[1].y), 2)));
		rectWidth = (float)(sqrt(pow((Rect[0].x - Rect[3].x), 2)) + sqrt(pow((Rect[0].y - Rect[3].y), 2)));
		if (rectLength < rectWidth)
		{
			std::swap(rectLength, rectWidth);
		}

		if (rectLength / rectWidth > ratio)
		{
			dst_box.push_back(box[i]); //利用RotatedRect计算角度
			for (int j = 0; j < 4; j++)
			{
				line(dst, Rect[j], Rect[(j + 1) % 4], cv::Scalar(0, 255, 0), 1, 8); //绘制矩形每条边
			}
			angle += abs(dst_box[dst_box.size() - 1].angle);
		}
	}
	angle = angle / dst_box.size(); //求平均
	return angle;
}

int ptreate::cal_split_midline(cv::Mat &src)
{
	cv::Mat threImg, blurImg, edgeImg, dst, morthImg;
	std::vector<cv::Vec4i> hierachy;
	std::vector<std::vector<cv::Point>> contours, conts_dst, conts_out;

	//预处理
	dst = src.clone();
	threshold(dst, threImg, 150, 255, cv::THRESH_BINARY);
	cv::Mat kernel = getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3), cv::Point(-1, -1));
	morphologyEx(threImg, morthImg, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1));
	morphologyEx(morthImg, morthImg, cv::MORPH_OPEN, kernel, cv::Point(-1, -1));

	//定义提取midline感兴趣区
	cv::Mat mask = cv::Mat::zeros(src.size(), src.type());
	int width = src.cols;
	int height = 150;
	int center_c = src.cols / 2;
	int center_r = src.rows / 2;
	// cv::Rect cv::Rect((int)(center_r - w/2), (int)(center_c - len /2), (int)(center_r + w / 2), (int)(center_c + len / 2));
	cv::Rect Rect((int)(center_c - width / 2), (int)(center_r - height / 2), width, height);

	// cv::Mat roiImg;
	// cv::Mat roiImg(morthImg.rows, morthImg.cols, CV_8UC3);//声明一个Mat实例，长宽高和src一样
	// src(cv::Rect).copyTo(image);//copyTo()方法是深拷贝方法

	// morthImg(cv::Rect).copyTo(roiImg);
	// roiImg = morthImg(cv::Rect);

	//做个背景图片--黑色背景
	cv::Mat _roiImg;
	mask(Rect) = cv::Scalar(255, 255, 255);
	cv::Mat roiImg = cv::Mat::zeros(src.size(), src.type());
	roiImg = cv::Scalar(0, 0, 0);
	morthImg.copyTo(roiImg, mask); // copyto的图片融合功能
	_roiImg = roiImg.clone();	   //备用

	double highThre = 50;
	double lowThre = highThre / 3;
	Canny(roiImg, edgeImg, lowThre, highThre, 3, true);
	findContours(edgeImg, contours, hierachy, cv::RETR_TREE, cv::CHAIN_APPROX_NONE, cv::Point(0, 0));

	for (size_t i = 0; i < contours.size(); i++)
	{
		double clength = arcLength(contours[i], true);
		if (clength > 10)
		{
			conts_dst.push_back(contours[i]);
		}
	}

	sort(conts_dst.begin(), conts_dst.end(), Contour_arcLength); //轮廓从大到小排序

	conts_out.push_back(conts_dst[0]);
	conts_out.push_back(conts_dst[1]);

	drawContours(roiImg, conts_out, -1, cv::Scalar(0, 0, 255), 1, cv::LINE_4); //调试

	//获取目标roi
	cv::Moments moment0, moment1; //矩
	moment0 = moments(conts_out[0], false);
	moment1 = moments(conts_out[1], false);
	cv::Point pt0;
	if (moment0.m00 != 0) //除数不能为0
	{
		pt0.x = cvRound(moment0.m10 / moment0.m00); //计算重心横坐标
		pt0.y = cvRound(moment0.m01 / moment0.m00); //计算重心纵坐标
	}

	cv::Point pt1;
	if (moment1.m00 != 0) //除数不能为0
	{
		pt1.x = cvRound(moment1.m10 / moment1.m00); //计算重心横坐标
		pt1.y = cvRound(moment1.m01 / moment1.m00); //计算重心纵坐标
	}

	////求roi各极限
	cv::Point extTop, extBot;
	if (pt1.y > pt0.y)
	{
		extTop = *min_element(conts_dst[1].begin(), conts_dst[1].end(),
							  [](const cv::Point &lhs, const cv::Point &rhs)
							  { return lhs.y < rhs.y; });

		extBot = *max_element(conts_dst[0].begin(), conts_dst[0].end(),
							  [](const cv::Point &lhs, const cv::Point &rhs)
							  { return lhs.y < rhs.y; });
	}
	else
	{
		extTop = *min_element(conts_dst[0].begin(), conts_dst[0].end(),
							  [](const cv::Point &lhs, const cv::Point &rhs)
							  { return lhs.y < rhs.y; });

		extBot = *max_element(conts_dst[1].begin(), conts_dst[1].end(),
							  [](const cv::Point &lhs, const cv::Point &rhs)
							  { return lhs.y < rhs.y; });
	}

	cv::Point extLeft0 = *min_element(conts_dst[0].begin(), conts_dst[0].end(),
									  [](const cv::Point &lhs, const cv::Point &rhs)
									  { return lhs.x < rhs.x; });

	cv::Point extRight0 = *max_element(conts_dst[0].begin(), conts_dst[0].end(),
									   [](const cv::Point &lhs, const cv::Point &rhs)
									   { return lhs.x < rhs.x; });

	cv::Point extLeft1 = *min_element(conts_dst[1].begin(), conts_dst[1].end(),
									  [](const cv::Point &lhs, const cv::Point &rhs)
									  { return lhs.x < rhs.x; });

	cv::Point extRight1 = *max_element(conts_dst[1].begin(), conts_dst[1].end(),
									   [](const cv::Point &lhs, const cv::Point &rhs)
									   { return lhs.x < rhs.x; });

	int roiLeft = extLeft0.x < extLeft1.x ? extLeft0.x : extLeft1.x;
	int roiRight = extRight0.x > extRight1.x ? extRight0.x : extRight1.x;
	int roiTop = extBot.y;
	int roiBot = extTop.y;

	cv::Rect roiRect(roiLeft, roiTop, (roiRight - roiLeft), (roiBot - roiTop));

	cv::Mat roiMask = cv::Mat::zeros(src.size(), src.type());
	cv::Mat dstImg;
	roiMask(roiRect) = cv::Scalar(255, 255, 255);
	dstImg = cv::Mat::zeros(src.size(), src.type());
	dstImg = cv::Scalar(0, 0, 0);
	_roiImg.copyTo(dstImg, roiMask); // copyto的图片融合功能

	highThre = 50;
	lowThre = highThre / 3;
	Canny(dstImg, edgeImg, lowThre, highThre, 3, true);
	findContours(edgeImg, contours, hierachy, cv::RETR_TREE, cv::CHAIN_APPROX_NONE, cv::Point(0, 0));

	sort(contours.begin(), contours.end(), Contour_arcLength);				 //轮廓从大到小排序
	drawContours(dstImg, contours, 0, cv::Scalar(0, 0, 255), 1, cv::LINE_4); //调试

	cv::Moments moment_mid = moments(contours[0], false);
	cv::Point midPoint;
	if (moment_mid.m00 != 0) //除数不能为0
	{
		midPoint.x = cvRound(moment_mid.m10 / moment_mid.m00); //计算重心横坐标
		midPoint.y = cvRound(moment_mid.m01 / moment_mid.m00); //计算重心纵坐标
	}

	line(src, cv::Point(0, midPoint.y), cv::Point(src.cols - 1, midPoint.y), cv::Scalar(0, 255, 0), 1, cv::LINE_AA);

	return midPoint.y;
}

// Make offset table
int ptreate::search_start_feature(cv::Mat &src, bool r_direction)
{
	cv::Mat threImg, blurImg, edgeImg, dst, morthImg;
	std::vector<cv::Vec4i> hierachy;
	std::vector<std::vector<cv::Point>> contours, conts_dst, conts_out;

	//预处理
	dst = src.clone();
	threshold(dst, threImg, 150, 255, cv::THRESH_BINARY);
	cv::Mat kernel = getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3), cv::Point(-1, -1));
	morphologyEx(threImg, morthImg, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1));
	morphologyEx(morthImg, morthImg, cv::MORPH_OPEN, kernel, cv::Point(-1, -1));

	//定义提取midline感兴趣区
	cv::Mat mask = cv::Mat::zeros(src.size(), src.type());
	int width = src.cols;
	int height = 150;
	int center_c = src.cols / 2;
	int center_r = src.rows / 2;
	// cv::Rect cv::Rect((int)(center_r - w/2), (int)(center_c - len /2), (int)(center_r + w / 2), (int)(center_c + len / 2));
	cv::Rect Rect((int)(center_c - width / 2), (int)(center_r - height / 2), width, height);

	// cv::Mat roiImg;
	// cv::Mat roiImg(morthImg.rows, morthImg.cols, CV_8UC3);//声明一个Mat实例，长宽高和src一样
	// src(cv::Rect).copyTo(image);//copyTo()方法是深拷贝方法

	// morthImg(cv::Rect).copyTo(roiImg);
	// roiImg = morthImg(cv::Rect);

	//做个背景图片--黑色背景
	cv::Mat _roiImg;
	mask(Rect) = cv::Scalar(255, 255, 255);
	cv::Mat roiImg = cv::Mat::zeros(src.size(), src.type());
	roiImg = cv::Scalar(0, 0, 0);
	morthImg.copyTo(roiImg, mask); // copyto的图片融合功能
	_roiImg = roiImg.clone();	   //备用

	double highThre = 50;
	double lowThre = highThre / 3;
	Canny(roiImg, edgeImg, lowThre, highThre, 3, true);
	findContours(edgeImg, contours, hierachy, cv::RETR_TREE, cv::CHAIN_APPROX_NONE, cv::Point(0, 0));

	for (size_t i = 0; i < contours.size(); i++)
	{
		double clength = arcLength(contours[i], true);
		if (clength > 10)
		{
			conts_dst.push_back(contours[i]);
		}
	}

	sort(conts_dst.begin(), conts_dst.end(), Contour_arcLength); //轮廓从大到小排序

	conts_out.push_back(conts_dst[0]);
	conts_out.push_back(conts_dst[1]);

	drawContours(roiImg, conts_out, -1, cv::Scalar(0, 0, 255), 1, cv::LINE_4); //调试

	//获取目标roi
	cv::Moments moment0, moment1; //矩
	moment0 = moments(conts_out[0], false);
	moment1 = moments(conts_out[1], false);
	cv::Point pt0;
	if (moment0.m00 != 0) //除数不能为0
	{
		pt0.x = cvRound(moment0.m10 / moment0.m00); //计算重心横坐标
		pt0.y = cvRound(moment0.m01 / moment0.m00); //计算重心纵坐标
	}

	cv::Point pt1;
	if (moment1.m00 != 0) //除数不能为0
	{
		pt1.x = cvRound(moment1.m10 / moment1.m00); //计算重心横坐标
		pt1.y = cvRound(moment1.m01 / moment1.m00); //计算重心纵坐标
	}

	////求roi各极限
	cv::Point extRight0, extRight1;
	if (pt1.y > pt0.y)
	{

		extRight1 = *max_element(conts_dst[1].begin(), conts_dst[1].end(),
								 [](const cv::Point &lhs, const cv::Point &rhs)
								 { return lhs.x < rhs.x; });
	}
	else
	{

		extRight0 = *max_element(conts_dst[0].begin(), conts_dst[0].end(),
								 [](const cv::Point &lhs, const cv::Point &rhs)
								 { return lhs.x < rhs.x; });
	}

	int roiRight = extRight0.x > extRight1.x ? extRight0.x : extRight1.x;

	line(src, cv::Point(roiRight, 0), cv::Point(roiRight, src.rows - 1), cv::Scalar(0, 255, 0), 1, cv::LINE_AA);

	return roiRight;
}

VI ptreate::search_region_edge(cv::Mat &src)
{
	//假设上次结束x_end
	int x_end = 555;
	int w = 30;
	int cnt = 6;

	cv::Mat mask, _src, _roiImg;

	mask = cv::Mat::zeros(src.size(), src.type());

	_src = cv::Mat::zeros(src.size(), src.type());

	//_roiImg = cv::Mat::zeros(src.cv::size(), src.type());

	//[x_end - w / 2, x_end + cnt * w + w / 2])
	/*cv::Rect cv::Rect(450, 480, src.cols- 450, src.rows-480);*/
	cv::Point pTopleft(x_end - w / 2, 480);
	cv::Point pBotRight(x_end + cnt * w + w / 2, 880);
	cv::Rect roiRect(pTopleft.x, pTopleft.y, pBotRight.x - pTopleft.x, pBotRight.y - pTopleft.y);
	mask(roiRect) = cv::Scalar(255, 255, 255);
	src.copyTo(_roiImg, mask); // copyto的图片融合功能

	cv::Mat threImg, blurImg, edgeImg, dst, morthImg;
	std::vector<cv::Vec4i> hierachy;
	std::vector<std::vector<cv::Point>> contours, conts_dst, conts_out;

	//预处理
	// dst = src.clone();
	threshold(_roiImg, threImg, 150, 255, cv::THRESH_BINARY);
	cv::Mat kernel = getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3), cv::Point(-1, -1));
	morphologyEx(threImg, morthImg, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1));
	morphologyEx(morthImg, morthImg, cv::MORPH_OPEN, kernel, cv::Point(-1, -1));

	double highThre = 50;
	double lowThre = highThre / 3;
	cv::Mat _morthImg;
	cvtColor(morthImg, _morthImg, cv::COLOR_BGR2GRAY);
	Canny(_morthImg, edgeImg, lowThre, highThre, 3, true);

	//轮廓提取
	findContours(edgeImg, contours, hierachy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE, cv::Point(0, 0));
	for (size_t i = 0; i < contours.size(); i++)
	{
		double clength = arcLength(contours[i], true);
		if (clength > 20)
		{
			conts_dst.push_back(contours[i]);
		}
	}

	std::vector<cv::RotatedRect> dst_box;
	std::vector<cv::RotatedRect> box(conts_dst.size()); //定义最小外接矩形集合
	cv::Point2f Rect[4];
	float rectLength = 0.0; //矩形长
	float rectWidth = 0.0;	//矩形宽
	for (size_t i = 0; i < conts_dst.size(); i++)
	{
		box[i] = minAreaRect(conts_dst[i]); //计算每个轮廓最小外接矩形
		box[i].points(Rect);				//把最小外接矩形四个端点复制给rect数组

		//根据长宽比，提取棱的外接矩形
		int ratio = 15;
		rectLength = (float)(sqrt(pow((Rect[0].x - Rect[1].x), 2)) + sqrt(pow((Rect[0].y - Rect[1].y), 2)));
		rectWidth = (float)(sqrt(pow((Rect[0].x - Rect[3].x), 2)) + sqrt(pow((Rect[0].y - Rect[3].y), 2)));
		if (rectLength < rectWidth)
		{
			std::swap(rectLength, rectWidth);
		}

		if (rectLength / rectWidth > ratio)
		{
			dst_box.push_back(box[i]); //利用RotatedRect计算角度
									   // for (int j = 0; j < 4; j++)
									   //{
									   //	line(dst, cv::Rect[j], cv::Rect[(j + 1) % 4], cv::Scalar(0, 255, 0), 1, 8);  //绘制矩形每条边
									   // }
		}
	}

	sort(dst_box.begin(), dst_box.end(), Rect_x); //按x排序
	std::vector<int> xs;
	for (int i = 1; i < dst_box.size(); ++i)
	{ //忽略第一个x_end

		xs.push_back(dst_box[i].center.x);
	}

#pragma region 直线提取(备用)
	//直线提取
	// cvtColor(edgeImg, dst, COLOR_GRAY2BGR);
	// std::vector<cv::Vec4f> hlines;
	// std::vector<cv::Vec4f> hlines_dst;
	// HoughLinesP(edgeImg, hlines, 2, CV_PI, 50, 40, 10);

	// for (size_t i = 0; i < hlines.cv::size(); i++)
	//{
	//	cv::Vec4f hl = hlines[i];
	//	cv::Point p1(hl[0], hl[1]);
	//	cv::Point p2(hl[2], hl[3]);
	//	double distance = sqrtf(powf((p1.x - p2.x), 2) + powf((p1.y - p2.y), 2));
	//	if (distance > 50) {
	//		hlines_dst.push_back(hl);
	//		line(dst, cv::Point(hl[0], hl[1]), cv::Point(hl[2], hl[3]), cv::Scalar(255, 0, 0), 1, LINE_AA);//调试
	//	}

	//}
	// sort(hlines_dst.begin(), hlines_dst.end(), hline_x);//直线按x从小到大排序
#pragma endregion

	return xs;
}

// Split image
PII ptreate::modify_edge(cv::Mat &src, int default_edge)
{
	PII res;
	return res;
}

// Auto detect
bool ptreate::focus_detection(cv::Mat &src)
{
	cv::Mat dst;
	cv::Scalar mean;
	cv::Scalar stddev;

	Laplacian(src, dst, CV_8U);
	meanStdDev(dst, mean, stddev);
	float res = stddev.val[0];
	return res > FOCUS_THRESHOLD ? true : false;
}

int ptreate::brightness_detection(cv::Mat &src)
{
	// 通过均值 or 中位数 or 直方图曲线拟合相似度来判断是否过亮或过暗
	// TODO: 算法更新
	double minv = 0.0, maxv = 0.0;
	double *minp = &minv;
	double *maxp = &maxv;
	minMaxIdx(src, minp, maxp);

	return ((minv > BRIGHTNESS_LOWER_THRESHOLD) && (maxv > BRIGHTNESS_UPPER_THRESHOLD)) ? true : false;
}

VI ptreate::k_means(VI data)
{
	std::vector<std::pair<float, std::vector<float>>> cluster;
	VI res;
	for (auto &var : data)
	{
		bool changed = false;
		for (auto &c : cluster)
		{
			if (c.first - 2 < var && var < c.first + 7)
			{
				c.second.push_back(var);
				c.first = accumulate(c.second.begin(), c.second.end(), 0) / c.second.size();
				changed = true;
				break;
			}
		}
		if (!changed)
		{
			std::pair<float, std::vector<float>> sub_data;
			sub_data.first = var;
			sub_data.second.push_back(var);
			cluster.push_back(sub_data);
		}
	}

	for (auto &c : cluster)
		res.push_back(static_cast<int>(round(c.first)));
	return res;
}

int ptreate::search(VI group, int target)
{
	int n = group.size() - 1;
	if (target <= group[0])
		return group[0];
	if (target >= group[n])
		return group[n];

	int left = 0;
	int right = n;
	while (left < right)
	{
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
	std::deque<std::pair<int, int>> queue;
	VI shadow_queue;

	std::pair<int, int> index(0, 1);
	queue.push_back(index);
	shadow_queue.push_back(index.first * 10 + index.second);
	while (!queue.empty())
	{
		auto &ind = queue.front();
		queue.pop_front();
		int cur_p = ind.first;
		int next_p = ind.second;

		if (next_p >= centers.size())
			continue;

		if (25 <= centers[next_p] - centers[cur_p] && centers[next_p] - centers[cur_p] <= 37)
		{
			true_center.push_back(centers[cur_p]);
			if (next_p == centers.size() - 1)
				true_center.push_back(centers[next_p]);
		}
		else
		{
			std::pair<int, int> used(cur_p, next_p + 1);
			if (find(shadow_queue.begin(), shadow_queue.end(), cur_p * 10 + next_p + 1) != shadow_queue.end())
			{
				queue.push_back(used);
				shadow_queue.push_back(cur_p * 10 + next_p + 1);
			}
		}
		std::pair<int, int> used(next_p, next_p + 1);
		if (find(shadow_queue.begin(), shadow_queue.end(), next_p * 10 + next_p + 1) == shadow_queue.end())
		{
			queue.push_back(used);
			shadow_queue.push_back(next_p * 10 + next_p + 1);
		}
	}
	return true_center;
}

g_data<float> ptreate::k_detection(std::vector<cv::Vec4i> &lines, int lower, int upper)
{
	g_data<float> res{};
	res.state = false;
	res.data = 0.0;
	std::vector<float> degrees;

	for (auto &line : lines)
	{
		if (line[2] - line[0] == 0)
			continue;
		float k = static_cast<float>((line[3] - line[1])) / static_cast<float>((line[2] - line[0]));
		float degree = (atan(k)) * 180.0 / CV_PI;

		if (lower < degree && degree < upper)
			degrees.push_back(degree);
	}
	if (degrees.empty())
		return res;
	float sum = std::accumulate(std::begin(degrees), std::end(degrees), 0.0);
	float degree_mean = sum / degrees.size() + 90;
	res.state = true;
	res.data = degree_mean;
	return res;
}

g_data<std::vector<PII>> ptreate::line_cluster(std::vector<cv::Vec4i> &lines)
{
	g_data<std::vector<PII>> res;
	res.state = false;

	for (auto &line : lines)
	{
		if (abs(line[1] - line[3]) <= 3 && abs(line[0] - line[2]) >= 5)
			continue;

		if (line[1] > line[3])
			std::swap(line[1], line[3]);

		if (res.data.empty())
		{
			std::pair<int, int> sub_data(line[1], line[3]);
			res.data.push_back(sub_data);
			continue;
		}
		bool change_flag = true;
		for (auto &cluster : res.data)
		{
			if ((cluster.first <= line[1] && line[1] <= cluster.second) || (cluster.first <= line[3] && line[3] <= cluster.second) ||
				(line[1] <= cluster.first && cluster.first <= line[3]) || (line[1] <= cluster.second && cluster.second <= line[3]))
			{
				cluster.first = std::min({cluster.first, line[1], line[3]});
				cluster.second = std::max({cluster.second, line[1], line[3]});
				change_flag = false;
			}
		}
		if (change_flag)
		{
			std::pair<int, int> sub_data(line[1], line[3]);
			res.data.push_back(sub_data);
		}
	}
	if (res.data.size() != 2)
		return res;
	else
	{
		res.state = true;
		return res;
	}
}

bool ptreate::valid_region_check(int position, std::vector<VI> region_set)
{
	for (auto &region : region_set)
	{
		if (position >= region[0] && position <= region[1])
			return true;
	}
	return false;
}
/***********************************************************************************/