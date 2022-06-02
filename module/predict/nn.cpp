/**
 * @file nn.cpp
 * @author lty (302702801@qq.com)
 * @brief 
 * @version 0.1
 * @date 2022-05-05
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "nn.h"
#include "../ptreate/pt_operator.h"
#include "../operation/operation.h"

void mobilenet::run()
{
    // 加载神经网络
    init_torch_model();

    // while循环读取消息队列送入神经网络预测
    while (true)
    {
        needle_queue->Pop(&n_data);
        batch_predict(n_data);
    }
}

torch::Tensor mobilenet::process(cv::Mat& image, int height, int width)
{
    std::vector<int64_t> dims = { 1, height, width, 1 };

    torch::Tensor image_T = torch::from_blob(image.data, dims, torch::kByte).to(at::kCUDA);

    image_T = image_T.permute({ 0, 3 , 1 , 2 });

    image_T = image_T.toType(torch::kFloat);

    image_T = image_T.div_(255);
    image_T = image_T.sub_(0.485).div_(0.229);

    return image_T;
}

void mobilenet::init_torch_model()
{
    if (torch::cuda::is_available())
    {
        printf("@@@Cuda Support Check: Pass@@@\n");
        model = torch::jit::load(MODEL_PATH);
        model.to(at::kCUDA);
        model.eval();
        warm_up();
        printf("@@@Load NN Model Check: Pass@@@\n");
    }
    else
    {
        printf("@@@Cuda Support Check: Fail@@@\n");
    }
}

void mobilenet::warm_up()
{
    cv::Mat image = cv::Mat::ones(cv::Size(32, 96), CV_8UC1);

    for (int i = 0; i < 5; i++)
    {
        auto r_img = ptreate::rotate_image(image, 180, true);
        single_predict(r_img);
    }
}

int mobilenet::single_predict(cv::Mat& image)
{
    if (image.empty())
    {
        printf("Can not load image\n");
    }
    torch::Tensor img = process(image, NEEDLE_GRID_HIGH, NEEDLE_GRID_WIDTH);
    torch::Tensor result = model.forward({ img }).toTensor();

    return torch::argmax(result).item<int>();
}

void mobilenet::batch_predict(pt_needle_data n_data)
{
    for (int i = 0; i < n_data.needle_img.size(); i++)
    {
        torch::Tensor img = process(n_data.needle_img[i], NEEDLE_GRID_HIGH, NEEDLE_GRID_WIDTH);
        tensor_vec[i] = img;
    }
    torch::TensorList tensor_list{ tensor_vec };
    torch::Tensor batch_of_tensors = torch::cat(tensor_list);
    torch::Tensor result = model.forward({ batch_of_tensors }).toTensor();

    // 将结果送入结果队列中
    nn_needle_ans_data res;
    res.row = n_data.row;
    res.light_path = n_data.light_path;
    res.needle_position = n_data.needle_position;
    res.start = n_data.start;
    res.end = n_data.end;

    auto predict_res = torch::argmax(result, 1);
    for (int i = 0; i < predict_res.size(0); i++)
    {
       res.category[i] = (predict_res[i].item().toInt());
       res.cvimage[i] = n_data.needle_img[i];
    }

    needle_ans_queue->Push(res);
}
