#pragma once

#include "../common/header.h"
class templatejson
{
private:
    // template_filepath为template.json配置文件所在路径
    static std::string template_filepath;

public:
    static rapidjson::Document d;
    //读取并解析template.json配置文件并初始化Document d,存入内存
    static void init();

    //获取指定结点的值 使用示例：
    // const Value &a = templatejson::get_member("row1","A","direction");
    // cout << a.GetString();
    static const rapidjson::Value &get_member(std::string name1);
    static const rapidjson::Value &get_member(std::string name1, std::string name2);
    static const rapidjson::Value &get_member(std::string name1, std::string name2, std::string name3);
    static const rapidjson::Value &get_member(std::string name1, std::string name2, std::string name3, std::string name4);
    static const rapidjson::Value &get_member(std::string name1, std::string name2, std::string name3, std::string name4, std::string name5);
    static const rapidjson::SizeType get_member_count();
};
