/**
 * @file utils.h
 * @author lty (302702801@qq.com)
 * @brief 
 * @version 0.1
 * @date 2022-05-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include "header.h"
namespace util
{
    // Time
    /**
     * @brief Get the cur time object
     * 
     * @return chrono::steady_clock::time_point 
     */
    std::chrono::steady_clock::time_point get_cur_time();

    /**
     * @brief 
     * 
     * @param start 
     * @param end 
     * @param formats 
     */
    void cal_time_gap(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end, const char *formats);

    /**
     * @brief 
     * 
     * @return string 
     */
    std::string date_now();

    /**
     * @brief 
     * 
     * @return string 
     */
    std::string time_now();

    /**
     * @brief 
     * 
     * @param t 
     * @return string 
     */
    std::string gmtime(time_t t);

    /**
     * @brief 
     * 
     * @return string 
     */
    std::string gmtime_now();

    /**
     * @brief 
     * 
     * @param gmt 
     * @return time_t 
     */
    time_t gmtime2ctime(const std::string &gmt);

    /**
     * @brief 
     * 
     * @return long long 
     */
    long long timestamp_now();

    /**
     * @brief 
     * 
     * @return double 
     */
    double timestamp_now_float();

    /**
     * @brief 
     * 
     * @param ms 
     */
    void sleep(int ms);

    // File
    /**
     * @brief 
     * 
     * @param file 
     * @return true 
     * @return false 
     */
    bool isfile(const std::string &file);

    /**
     * @brief 
     * 
     * @param path 
     * @return true 
     * @return false 
     */
    bool delete_file(const std::string &path);

    /**
     * @brief 
     * 
     * @param path 
     * @return true 
     * @return false 
     */
    bool exists(const std::string &path);
    
    /**
     * @brief 
     * 
     * @param path 
     * @return true 
     * @return false 
     */
    bool mkdir(const std::string &path);
    
    /**
     * @brief 
     * 
     * @param path 
     * @return true 
     * @return false 
     */
    bool mkdirs(const std::string &path);
    
    /**
     * @brief 
     * 
     * @param directory 
     * @param ignore_fail 
     * @return true 
     * @return false 
     */
    bool rmtree(const std::string &directory, bool ignore_fail = false);
    
    /**
     * @brief 
     * 
     * @param fmt 
     * @param ... 
     * @return string 
     */
    std::string format(const char *fmt, ...);
    
    /**
     * @brief 
     * 
     * @param path 
     * @param mode 
     * @return FILE* 
     */
    FILE *fopen_mkdirs(const std::string &path, const std::string &mode);
    
    /**
     * @brief 
     * 
     * @param path 
     * @param include_suffix 
     * @return string 
     */
    std::string file_name(const std::string &path, bool include_suffix = true);
    
    /**
     * @brief 
     * 
     * @param path 
     * @return string 
     */
    std::string directory(const std::string &path);
    
    /**
     * @brief 
     * 
     * @param file 
     * @return size_t 
     */
    size_t file_size(const std::string &file);
    
    /**
     * @brief 
     * 
     * @param file 
     * @return time_t 
     */
    time_t last_modify(const std::string &file);
    
    /**
     * @brief 
     * 
     * @param file 
     * @return vector<uint8_t> 
     */
    std::vector<uint8_t> load_file(const std::string &file);
    
    /**
     * @brief 
     * 
     * @param file 
     * @return string 
     */
    std::string load_text_file(const std::string &file);
    
    /**
     * @brief 
     * 
     * @param str 
     * @param matcher 
     * @param igrnoe_case 
     * @return true 
     * @return false 
     */
    bool pattern_match(const char *str, const char *matcher, bool igrnoe_case = true);
    
    /**
     * @brief 
     * 
     * @param file 
     * @param data 
     * @param mk_dirs 
     * @return true 
     * @return false 
     */
    bool save_file(const std::string &file, const std::vector<uint8_t> &data, bool mk_dirs = true);
    
    /**
     * @brief 
     * 
     * @param file 
     * @param data 
     * @param mk_dirs 
     * @return true 
     * @return false 
     */
    bool save_file(const std::string &file, const std::string &data, bool mk_dirs = true);
    

    /**
     * @brief 
     * 
     * @param file 
     * @param data 
     * @param length 
     * @param mk_dirs 
     * @return true 
     * @return false 
     */
    bool save_file(const std::string &file, const void *data, size_t length, bool mk_dirs = true);
    
    /**
     * @brief 
     * 
     * @param directory 
     * @param filter 
     * @param findDirectory 
     * @param includeSubDirectory 
     * @return vector<string> 
     */
    std::vector<std::string> find_files(const std::string &directory, const std::string &filter = "*", bool findDirectory = false, bool includeSubDirectory = false);

    // Str
    /**
     * @brief 
     * 
     * @param str 
     * @param with 
     * @return true 
     * @return false 
     */
    bool begin_with(const std::string &str, const std::string &with);

    /**
     * @brief 
     * 
     * @param str 
     * @param with 
     * @return true 
     * @return false 
     */
    bool end_with(const std::string &str, const std::string &with);

    /**
     * @brief 
     * 
     * @param str 
     * @param spstr 
     * @return vector<string> 
     */
    std::vector<std::string> split_string(const std::string &str, const std::string &spstr);

    /**
     * @brief 
     * 
     * @param str 
     * @param token 
     * @param value 
     * @param nreplace 
     * @param out_num_replace 
     * @return string 
     */
    std::string replace_string(const std::string &str, const std::string &token, const std::string &value, int nreplace = -1, int *out_num_replace = nullptr);

    /**
     * @brief 
     * 
     * @param input 
     * @param align_size 
     * @param blank 
     * @return string 
     */
    std::string align_blank(const std::string &input, int align_size, char blank = ' ');

    // Image
    /**
     * @brief 
     * 
     * @param h 
     * @param s 
     * @param v 
     * @return tuple<uint8_t, uint8_t, uint8_t> 
     */
    std::tuple<uint8_t, uint8_t, uint8_t> hsv2bgr(float h, float s, float v);

    /**
     * @brief 
     * 
     * @param id 
     * @return tuple<uint8_t, uint8_t, uint8_t> 
     */
    std::tuple<uint8_t, uint8_t, uint8_t> random_color(int id);

    // Cvt
    /**
     * @brief 
     * 
     * @param v 
     * @return short 
     */
    short invert_short(short v);

    /**
     * @brief 
     * 
     * @param v 
     * @return int 
     */
    int invert_int(int v);

    /**
     * @brief 
     * 
     * @param dims 
     * @return string 
     */
    std::string join_dims(const std::vector<int> &dims);

    /**
     * @brief 
     * 
     * @param n 
     * @param align 
     * @return int 
     */
    inline int upbound(int n, int align = 32) { return (n + align - 1) / align * align; }

    // Compare
    template <class T>
    T t_max(T x, T y, T z)
    {
        return x > y ? (x > z ? x : z) : (y > z ? y : z);
    }

    template <class T>
    T t_min(T x, T y, T z)
    {
        return x < y ? (x < z ? x : z) : (y < z ? y : z);
    }
}
