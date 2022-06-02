#pragma once

#include "../common/header.h"

class offset_data {
public:
    // 显式声明默认构造函数
    offset_data() = default;

    // 自定义构造函数
    offset_data(int x_coordinate, int shift_count, int stitch_index) :
            _x_coordinate(x_coordinate), _shift_count(shift_count), _stitch_index(stitch_index) {}

    // 拷贝构造函数
    offset_data(const offset_data& rhs) : _x_coordinate(rhs._x_coordinate), _shift_count(rhs._shift_count), _stitch_index(rhs._stitch_index) {}

    // 显示声明默认析构函数
    ~offset_data() = default;

    // 重载赋值运算
    offset_data& operator=(const offset_data& rhs) {
        this->_x_coordinate = rhs._x_coordinate;
        this->_shift_count = rhs._shift_count;
        this->_stitch_index = rhs._stitch_index;
        return *this;
    }

    // 为Json序列化而实现的函数
    template <typename Writer>
    void Serialize(Writer& writer) const {
        writer.StartObject();
        writer.String("x_coordinate");
        writer.Int(_x_coordinate);
        writer.String("shift_count");
        writer.Int(_shift_count);
        writer.String("stitch_index");
        writer.Int(_stitch_index);
        writer.EndObject();
    }

    int _x_coordinate;
    int _shift_count;
    int _stitch_index;
};


class calibration {
public:
    // 显式声明默认构造函数
    calibration() = default;

    // 自定义构造函数
    calibration(const double angle, int midline) : _angle(angle), _midline(midline) {}

    // 拷贝构造函数
    calibration(const calibration& rhs) : _angle(rhs._angle), _midline(rhs._midline) {}

    // 显示声明默认析构函数
    ~calibration() = default;

    // 重载赋值运算
    calibration& operator=(const calibration& rhs) {
        this->_angle = rhs._angle;
        this->_midline = rhs._midline;
        return *this;
    }

    // 为Json序列化而实现的函数
    template <typename Writer>
    void Serialize(Writer& writer) const {
        writer.StartObject();
        writer.String("angle");
        writer.Double(_angle);
        writer.String("midline");
        writer.Int(_midline);
        writer.EndObject();
    }

    double _angle;
    int _midline;
};


class sub_offset_data : protected offset_data {
public:
    // 显式声明默认构造函数
    sub_offset_data() = default;

    // 自定义构造函数
    sub_offset_data(int direction, int col) {
        string D = to_string(direction);
        string C = to_string(col);
        _DC = D + C;
    }

    // 拷贝构造函数
    sub_offset_data(const sub_offset_data& rhs) :_DC(rhs._DC), _offset_data(rhs._offset_data) {}

    // 显示声明默认析构函数
    ~sub_offset_data() = default;

    // 重载赋值运算
    sub_offset_data& operator=(const sub_offset_data& rhs) {
        this->_DC = rhs._DC;
        this->_offset_data = rhs._offset_data;
        return *this;
    }

    // 为Json序列化而实现的函数
    template <typename Writer>
    void Serialize(Writer& writer) const {
        #if RAPIDJSON_HAS_STDSTRING
        writer.String(_DC);
        #else
        writer.String(_DC.c_str(), static_cast<SizeType>(_DC.length()));
        #endif
        _offset_data.Serialize(writer);
    }

    string	_DC;
    offset_data _offset_data;
};


class light_path : protected calibration, sub_offset_data
{
public:
    // 显式声明默认构造函数
    light_path() = default;

    // 自定义构造函数
    light_path(const calibration& calibration) : _calibration(calibration) {}

    // 拷贝构造函数
    light_path(const light_path& rhs) : _calibration(rhs._calibration), _table(rhs._table) {}

    // 显示声明默认析构函数
    ~light_path() = default;

    // 重载赋值运算
    light_path& operator=(const light_path& rhs) {
        this->_calibration = rhs._calibration;
        this->_table = rhs._table;
        return *this;
    }

    void add_sub_offset_data(const sub_offset_data& sub_offset_data) {
        _table.push_back(sub_offset_data);
    }

    void add_calibration(const calibration& calibration) {
        _calibration = calibration;
    }

    // 为Json序列化而实现的函数
    template <typename Writer>
    void Serialize(Writer& writer) const {
        writer.StartObject();

        writer.String("calibration");
        _calibration.Serialize(writer);

        writer.String("table");
        writer.StartObject();
        for (vector<sub_offset_data>::const_iterator itr = _table.begin(); itr != _table.end(); ++itr)
            itr->Serialize(writer);
        writer.EndObject();

        writer.EndObject();
    }

    calibration _calibration;
    vector<sub_offset_data> _table;
};


class mult_light_path : protected light_path {
public:
    // 显式声明默认构造函数
    mult_light_path() = default;

    // 显示声明默认析构函数
    ~mult_light_path() = default;

    void add_light_path(light_path light_path) {
        _mult_light_path.push_back(light_path);
    }

    // 为Json序列化而实现的函数
    template <typename Writer>
    void Serialize(Writer& writer) const {
        writer.StartObject();
        for (int i = 0; i < LIGHT_PATH_COUNT; i++) {
            #if RAPIDJSON_HAS_STDSTRING
            writer.String(LIGHT_PATH_MAPPING[i]);
            #else
            writer.String(LIGHT_PATH_MAPPING[i].c_str(), static_cast<SizeType>(LIGHT_PATH_MAPPING[i].length()));
            #endif
            _mult_light_path[i].Serialize(writer);
        }
        writer.EndObject();
    }

    vector<light_path> _mult_light_path;
};

