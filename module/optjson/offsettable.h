#pragma once

#include "../common/header.h"
class offsettable
{
private:
    // offsettable.json 路径
    static std::string offsettable_filepath;

public:
    static rapidjson::Document d;
    //读取并解析offsettable.json配置文件并初始化Document d,存入内存
    static void init();

    //获取指定值 示例：
    // const Value &a = offsettable::get_member("A", "calibration", "midline");
    // cout << a.GetDouble();
    static const rapidjson::Value &get_member(std::string name1);
    static const rapidjson::Value &get_member(std::string name1, std::string name2);
    static const rapidjson::Value &get_member(std::string name1, std::string name2, std::string name3);
    static const rapidjson::Value &get_member(std::string name1, std::string name2, std::string name3, std::string name4);

    //示例：
    // offsettable::init();
    // offsettable::set_calibration("A", -9.255963134931784e61, -858993460);
    static void set_calibration(const char *light_path, float angle_num, int midline_num);

    static void set_table();
};
