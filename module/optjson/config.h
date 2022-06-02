#pragma once

#include "../common/header.h"
class config
{
private:
    // config_filepath为config.json配置文件所在路径
    static std::string config_filepath;

public:
    static rapidjson::Document d;
    //读取并解析config.json配置文件并初始化Document d,存入内存
    static void init();

    //获取指定结点的值 示例：
    // const Value &a = config::get_member("alarm_log_filepath");
    // cout << a.GetString();
    static rapidjson::Value &get_member(std::string name);

    //改变config.json配置文件并重新赋值成员变量 示例
    // Value a("/home/zs/seu/log/run.log");
    // config::set_member("alarm_log_filepath", a);
    static void set_member(std::string name, rapidjson::Value &value);
};