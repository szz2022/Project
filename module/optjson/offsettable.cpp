#include "offsettable.h"
#include "../json/json.h"

std::string offsettable::offsettable_filepath = "../conf/table/offset_table.json";
rapidjson::Document offsettable::d;

void offsettable::init()
{
    //读取并解析offsettable.json配置文件并初始化Document d,存入内存
    json::read(offsettable_filepath.data(), d);
}

const rapidjson::Value &offsettable::get_member(std::string name1)
{
    const char *key1 = name1.data();
    const rapidjson::Value &value1 = d[key1];
    return value1;
}
const rapidjson::Value &offsettable::get_member(std::string name1, std::string name2)
{
    const rapidjson::Value &value1 = get_member(name1);
    const char *key2 = name2.data();
    const rapidjson::Value &value2 = value1[key2];
    return value2;
}
const rapidjson::Value &offsettable::get_member(std::string name1, std::string name2, std::string name3)
{
    const rapidjson::Value &value2 = get_member(name1, name2);
    const char *key3 = name3.data();
    const rapidjson::Value &value3 = value2[key3];
    return value3;
}
const rapidjson::Value &offsettable::get_member(std::string name1, std::string name2, std::string name3, std::string name4)
{
    const rapidjson::Value &value3 = get_member(name1, name2, name3);
    const char *key4 = name4.data();
    const rapidjson::Value &value4 = value3[key4];
    return value4;
}
void offsettable::set_calibration(const char *light_path, float angle_num, int midline_num)
{
    rapidjson::Value &value = d[light_path];
    rapidjson::Value &calibration = value["calibration"];
    rapidjson::Value &angle = calibration["angle"];
    rapidjson::Value &midline = calibration["midline"];
    angle.SetFloat(angle_num);
    midline.SetInt(midline_num);

    json::write(offsettable_filepath.data(), d);
    //重新读取配置文件
    init();
}