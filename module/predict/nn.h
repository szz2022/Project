/**
 * @file nn.h
 * @author lty (302702801@qq.com)
 * @brief 
 * @version 0.1
 * @date 2022-05-05
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "../common/header.h"

class mobilenet
{
public:
    /**
     * @brief 加载神经网络模型
     * 
     */
    void init_torch_model();

    /**
     * @brief 预热GPU
     * 
     */
    void warm_up();
    
    /**
     * @brief 线程执行入口函数
     * 
     */
    void run();

    /**
     * @brief 单张图片预测API
     * 
     * @param image 输入CV::Mat图像数据
     * @return int 返回结果
     */
    int single_predict(cv::Mat& image);

    /**
     * @brief bathc预测API直接将结果写入消息队列
     * 
     * @param n_data 
     */
    void batch_predict(pt_needle_data n_data);

private:
    pt_needle_data n_data;
    torch::jit::script::Module model;
    std::vector<torch::Tensor> tensor_vec;

    /**
     * @brief 处理cv::Mat图像归一化转化为Tensor
     * 
     * @param image 输入CV::Mat图像数据
     * @param height 输入的高度
     * @param width 输入的宽度
     * @return torch::Tensor 返回tensor图像
     */
    torch::Tensor process(cv::Mat& image, int height, int width);
};
