#pragma once

#include "../common/header.h"

class json {
public:
    static bool read(const char* file_name, rapidjson::Document& dom_tree);

    static bool write(const char* file_name, rapidjson::Document& json);

    // 函数模板声明和定义需要在一起
    template <typename T>
    static void parse(rapidjson::Document& DOM, T& DS) {
        rapidjson::StringBuffer SB;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(SB);
        DS.Serialize(writer);
        DOM.Parse(SB.GetString());
    }

private:
};
