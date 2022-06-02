#include "templatejson.h"
#include "../json/json.h"

rapidjson::Document templatejson::d;

std::string templatejson::template_filepath = "../conf/template/JW2109066_XS_b.json";

void templatejson::init()
{
    //读取并解析offsettable.json配置文件并初始化Document d,存入内存
    json::read(template_filepath.data(), d);
}

const rapidjson::Value &templatejson::get_member(std::string name1)
{
    const char *key1 = name1.data();
    rapidjson::Value &value1 = d[key1];
    return value1;
}
const rapidjson::Value &templatejson::get_member(std::string name1, std::string name2)
{
    const rapidjson::Value &value1 = get_member(name1);
    const char *key2 = name2.data();
    const rapidjson::Value &value2 = value1[key2];
    return value2;
}
const rapidjson::Value &templatejson::get_member(std::string name1, std::string name2, std::string name3)
{
    const rapidjson::Value &value2 = get_member(name1, name2);
    const char *key3 = name3.data();
    const rapidjson::Value &value3 = value2[key3];
    return value3;
}
const rapidjson::Value &templatejson::get_member(std::string name1, std::string name2, std::string name3, std::string name4)
{
    const rapidjson::Value &value3 = get_member(name1, name2, name3);
    const char *key4 = name4.data();
    const rapidjson::Value &value4 = value3[key4];
    return value4;
}

const rapidjson::Value &templatejson::get_member(std::string name1, std::string name2, std::string name3, std::string name4, std::string name5)
{
    const rapidjson::Value &value4 = get_member(name1, name2, name3, name4);
    const char *key5 = name5.data();
    const rapidjson::Value &value5 = value4[key5];
    return value5;
}

const rapidjson::SizeType templatejson::get_member_count(){
    return d.MemberCount();
}
