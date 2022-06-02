#include "config.h"
#include "../json/json.h"

//初始化config_filepath
std::string config::config_filepath = "../conf/cfg/config.json";

rapidjson::Document config::d;

void config::init()
{
    //读取并解析config.json配置文件并初始化Document d,存入内存
    // string类data()方法 将string转为char*;
    json::read(config_filepath.data(), d);
}

rapidjson::Value &config::get_member(std::string name)
{
    const char *key = name.data();
    assert(d.HasMember(key));
    rapidjson::Value &v = d[key];
    return v;
}

void config::set_member(std::string name, rapidjson::Value &value)
{
    const char *key = name.data();
    assert(d.HasMember(key));
    d[key].Swap(value);
    json::write(config_filepath.data(), d);
    //重新读取配置文件
    init();
}