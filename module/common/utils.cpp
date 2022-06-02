/**
 * @file utils.cpp
 * @author lty (302702801@qq.com)
 * @brief 
 * @version 0.1
 * @date 2022-05-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "utils.h"

#if defined(U_OS_LINUX)
#define __GetTimeBlock \
    time_t timep;      \
    time(&timep);      \
    tm &t = *(tm *)localtime(&timep);
#endif

#if defined(U_OS_WINDOWS)
#define __GetTimeBlock \
    tm t;              \
    _getsystime(&t);
#endif

using namespace std;

namespace util
{
    chrono::steady_clock::time_point get_cur_time()
    {
        return chrono::steady_clock::now();
    }

    void cal_time_gap(chrono::steady_clock::time_point start, chrono::steady_clock::time_point end, const char *formats)
    {
        printf("%s spend time = %.5f sec\n", formats, (chrono::duration_cast<chrono::microseconds>(end - start).count()) / 1000000.0);
    }

    string date_now()
    {
        char time_string[20];
        __GetTimeBlock;

        sprintf(time_string, "%04d-%02d-%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        return time_string;
    }

    string time_now()
    {
        char time_string[20];
        __GetTimeBlock;

        sprintf(time_string, "%04d-%02d-%02d %02d:%02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        return time_string;
    }

    string gmtime(time_t t)
    {
        t += 28800;
        tm *gmt = ::gmtime(&t);

        // http://en.cppreference.com/w/c/chrono/strftime
        // e.g.: Sat, 22 Aug 2015 11:48:50 GMT
        //       5+   3+4+   5+   9+       3   = 29
        const char *fmt = "%a, %d %b %Y %H:%M:%S GMT";
        char tstr[30];
        strftime(tstr, sizeof(tstr), fmt, gmt);
        return tstr;
    }

    string gmtime_now()
    {
        return gmtime(time(nullptr));
    }

    int get_month_by_name(char *month)
    {
        if (strcmp(month, "Jan") == 0)
            return 0;
        if (strcmp(month, "Feb") == 0)
            return 1;
        if (strcmp(month, "Mar") == 0)
            return 2;
        if (strcmp(month, "Apr") == 0)
            return 3;
        if (strcmp(month, "May") == 0)
            return 4;
        if (strcmp(month, "Jun") == 0)
            return 5;
        if (strcmp(month, "Jul") == 0)
            return 6;
        if (strcmp(month, "Aug") == 0)
            return 7;
        if (strcmp(month, "Sep") == 0)
            return 8;
        if (strcmp(month, "Oct") == 0)
            return 9;
        if (strcmp(month, "Nov") == 0)
            return 10;
        if (strcmp(month, "Dec") == 0)
            return 11;
        return -1;
    }

    int get_week_day_by_name(char *wday)
    {
        if (strcmp(wday, "Sun") == 0)
            return 0;
        if (strcmp(wday, "Mon") == 0)
            return 1;
        if (strcmp(wday, "Tue") == 0)
            return 2;
        if (strcmp(wday, "Wed") == 0)
            return 3;
        if (strcmp(wday, "Thu") == 0)
            return 4;
        if (strcmp(wday, "Fri") == 0)
            return 5;
        if (strcmp(wday, "Sat") == 0)
            return 6;
        return -1;
    }

    time_t gmtime2ctime(const string &gmt)
    {

        char week[4] = {0};
        char month[4] = {0};
        tm date;
        sscanf(gmt.c_str(), "%3s, %2d %3s %4d %2d:%2d:%2d GMT", week, &date.tm_mday, month, &date.tm_year, &date.tm_hour, &date.tm_min, &date.tm_sec);
        date.tm_mon = get_month_by_name(month);
        date.tm_wday = get_week_day_by_name(week);
        date.tm_year = date.tm_year - 1900;
        return mktime(&date);
    }

    long long timestamp_now()
    {
        return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    }

    double timestamp_now_float()
    {
        return chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
    }

    void sleep(int ms)
    {
        this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    bool mkdir(const string &path)
    {
#ifdef U_OS_WINDOWS
        return CreateDirectoryA(path.c_str(), nullptr);
#else
        return ::mkdir(path.c_str(), 0755) == 0;
#endif
    }

    bool mkdirs(const string &path)
    {
        if (path.empty())
            return false;
        if (exists(path))
            return true;

        string _path = path;
        char *dir_ptr = (char *)_path.c_str();
        char *iter_ptr = dir_ptr;

        bool keep_going = *iter_ptr not_eq 0;
        while (keep_going)
        {
            if (*iter_ptr == 0)
                keep_going = false;

#ifdef U_OS_WINDOWS
            if (*iter_ptr == '/' or *iter_ptr == '\\' or *iter_ptr == 0)
            {
#else
            if ((*iter_ptr == '/' and iter_ptr not_eq dir_ptr) or *iter_ptr == 0)
            {
#endif
                char old = *iter_ptr;
                *iter_ptr = 0;
                if (!exists(dir_ptr))
                {
                    if (!mkdir(dir_ptr))
                    {
                        if (!exists(dir_ptr))
                        {
                            // TODO: 添加日志模块打印日志
                            // INFOE("mkdirs %s return false.", dir_ptr);
                            return false;
                        }
                    }
                }
                *iter_ptr = old;
            }
            iter_ptr++;
        }
        return true;
    }

    bool isfile(const string &file)
    {
#if defined(U_OS_LINUX)
        struct stat st;
        stat(file.c_str(), &st);
        return S_ISREG(st.st_mode);
#elif defined(U_OS_WINDOWS)
        // TODO: 添加日志模块打印日志
        // INFOW("is_file has not support on windows os");
        return 0;
#endif
    }

    bool delete_file(const string &path)
    {
#if defined(U_OS_LINUX)
        return ::remove(path.c_str()) == 0;
#elif defined(U_OS_WINDOWS)
        return DeleteFileA(path.c_str());
#endif
    }

    bool exists(const string &path)
    {
#if defined(U_OS_LINUX)
        return access(path.c_str(), R_OK) == 0;
#elif defined(U_OS_WINDOWS)
        return ::PathFileExistsA(path.c_str());
#endif
    }

    bool rmtree(const string &directory, bool ignore_fail)
    {
        if (directory.empty())
            return false;
        auto files = find_files(directory, "*", false);
        auto dirs = find_files(directory, "*", true);

        bool success = true;
        for (int i = 0; i < files.size(); ++i)
        {
            if (::remove(files[i].c_str()) != 0)
            {
                success = false;

                if (!ignore_fail)
                    return false;
            }
        }

        dirs.insert(dirs.begin(), directory);
        for (int i = (int)dirs.size() - 1; i >= 0; --i)
        {
#if defined(U_OS_LINUX)
            if (::rmdir(dirs[i].c_str()) != 0)
            {
#elif defined(U_OS_WINDOWS)
            if (!::RemoveDirectoryA(dirs[i].c_str()))
            {
#endif
                success = false;
                if (!ignore_fail)
                    return false;
            }
        }
        return success;
    }

    string format(const char *fmt, ...)
    {
        va_list vl;
        va_start(vl, fmt);
        char buffer[2048];
        vsnprintf(buffer, sizeof(buffer), fmt, vl);
        return buffer;
    }

    FILE *fopen_mkdirs(const string &path, const string &mode)
    {
        FILE *f = fopen(path.c_str(), mode.c_str());
        if (f)
            return f;
#if defined(U_OS_LINUX)
        int p = path.rfind('/');
#elif defined(U_OS_WINDOWS)
        int e = path.rfind('\\');
        p = std::max(p, e);
#endif
        if (p == -1)
            return nullptr;

        string directory = path.substr(0, p);
        if (!mkdirs(directory))
            return nullptr;

        return fopen(path.c_str(), mode.c_str());
    }

    string file_name(const string &path, bool include_suffix)
    {
        if (path.empty())
            return "";
#if defined(U_OS_LINUX)
        int p = path.rfind('/');
#elif defined(U_OS_WINDOWS)
        int e = path.rfind('\\');
        p = std::max(p, e);
#endif
        p += 1;

        //include suffix
        if (include_suffix)
            return path.substr(p);

        int u = path.rfind('.');
        if (u == -1)
            return path.substr(p);

        if (u <= p)
            u = path.size();
        return path.substr(p, u - p);
    }

    string directory(const string &path)
    {
        if (path.empty())
            return ".";

#if defined(U_OS_LINUX)
        int p = path.rfind('/');
#elif defined(U_OS_WINDOWS)
        int e = path.rfind('\\');
        p = std::max(p, e);
#endif
        if (p == -1)
            return ".";

        return path.substr(0, p + 1);
    }

    size_t file_size(const string &file)
    {
#if defined(U_OS_LINUX)
        struct stat st;
        stat(file.c_str(), &st);
        return st.st_size;
#elif defined(U_OS_WINDOWS)
        WIN32_FIND_DATAA find_data;
        HANDLE hFind = FindFirstFileA(file.c_str(), &find_data);
        if (hFind == INVALID_HANDLE_VALUE)
            return 0;

        FindClose(hFind);
        return (uint64_t)find_data.nFileSizeLow | ((uint64_t)find_data.nFileSizeHigh << 32);
#endif
    }

    time_t last_modify(const string &file)
    {
#if defined(U_OS_LINUX)
        struct stat st;
        stat(file.c_str(), &st);
        return st.st_mtim.tv_sec;
#elif defined(U_OS_WINDOWS)
        INFOW("LastModify has not support on windows os");
        return 0;
#endif
    }

    std::vector<uint8_t> load_file(const string &file)
    {
        ifstream in(file, ios::in | ios::binary);
        if (!in.is_open())
            return {};

        in.seekg(0, ios::end);
        size_t length = in.tellg();

        std::vector<uint8_t> data;
        if (length > 0)
        {
            in.seekg(0, ios::beg);
            data.resize(length);

            in.read((char *)&data[0], length);
        }
        in.close();
        return data;
    }

    string load_text_file(const string &file)
    {
        ifstream in(file, ios::in | ios::binary);
        if (!in.is_open())
            return {};

        in.seekg(0, ios::end);
        size_t length = in.tellg();

        string data;
        if (length > 0)
        {
            in.seekg(0, ios::beg);
            data.resize(length);

            in.read((char *)&data[0], length);
        }
        in.close();
        return data;
    }

    bool save_file(const string &file, const void *data, size_t length, bool mk_dirs)
    {

        if (mk_dirs)
        {
#if defined(U_OS_LINUX)
            int p = (int)file.rfind('/');
#elif defined(U_OS_WINDOWS)
            int e = (int)file.rfind('\\');
            p = std::max(p, e);
#endif
            if (p != -1)
            {
                if (!mkdirs(file.substr(0, p)))
                    return false;
            }
        }

        FILE *f = fopen(file.c_str(), "wb");
        if (!f)
            return false;

        if (data && length > 0)
        {
            if (fwrite(data, 1, length, f) != length)
            {
                fclose(f);
                return false;
            }
        }
        fclose(f);
        return true;
    }

    bool save_file(const string &file, const string &data, bool mk_dirs)
    {
        return save_file(file, data.data(), data.size(), mk_dirs);
    }

    bool save_file(const string &file, const vector<uint8_t> &data, bool mk_dirs)
    {
        return save_file(file, data.data(), data.size(), mk_dirs);
    }
    
    bool alphabet_equal(char a, char b, bool ignore_case)
    {
        if (ignore_case)
        {
            a = a > 'a' and a < 'z' ? a - 'a' + 'A' : a;
            b = b > 'a' and b < 'z' ? b - 'a' + 'A' : b;
        }
        return a == b;
    }

    static bool pattern_match_body(const char *str, const char *matcher, bool igrnoe_case)
    {
        //   abcdefg.pnga          *.png      > false
        //   abcdefg.png           *.png      > true
        //   abcdefg.png          a?cdefg.png > true

        if (!matcher or !*matcher or !str or !*str)
            return false;

        const char *ptr_matcher = matcher;
        while (*str)
        {
            if (*ptr_matcher == '?')
            {
                ptr_matcher++;
            }
            else if (*ptr_matcher == '*')
            {
                if (*(ptr_matcher + 1))
                {
                    if (pattern_match_body(str, ptr_matcher + 1, igrnoe_case))
                        return true;
                }
                else
                {
                    return true;
                }
            }
            else if (!alphabet_equal(*ptr_matcher, *str, igrnoe_case))
            {
                return false;
            }
            else
            {
                if (*ptr_matcher)
                    ptr_matcher++;
                else
                    return false;
            }
            str++;
        }

        while (*ptr_matcher)
        {
            if (*ptr_matcher not_eq '*')
                return false;
            ptr_matcher++;
        }
        return true;
    }

    bool pattern_match(const char *str, const char *matcher, bool igrnoe_case)
    {
        //   abcdefg.pnga          *.png      > false
        //   abcdefg.png           *.png      > true
        //   abcdefg.png          a?cdefg.png > true

        if (!matcher or !*matcher or !str or !*str)
            return false;

        char filter[500];
        strcpy(filter, matcher);

        vector<const char *> arr;
        char *ptr_str = filter;
        char *ptr_prev_str = ptr_str;
        while (*ptr_str)
        {
            if (*ptr_str == ';')
            {
                *ptr_str = 0;
                arr.push_back(ptr_prev_str);
                ptr_prev_str = ptr_str + 1;
            }
            ptr_str++;
        }

        if (*ptr_prev_str)
            arr.push_back(ptr_prev_str);

        for (int i = 0; i < arr.size(); ++i)
        {
            if (pattern_match_body(str, arr[i], igrnoe_case))
                return true;
        }
        return false;
    }

#if defined(U_OS_WINDOWS)
    vector<string> find_files(const string &directory, const string &filter, bool findDirectory, bool includeSubDirectory)
    {

        string realpath = directory;
        if (realpath.empty())
            realpath = "./";

        char backchar = realpath.back();
        if (backchar not_eq '\\' and backchar not_eq '/')
            realpath += "/";

        vector<string> out;
        _WIN32_FIND_DATAA find_data;
        stack<string> ps;
        ps.push(realpath);

        while (!ps.empty())
        {
            string search_path = ps.top();
            ps.pop();

            HANDLE hFind = FindFirstFileA((search_path + "*").c_str(), &find_data);
            if (hFind not_eq INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (strcmp(find_data.cFileName, ".") == 0 or strcmp(find_data.cFileName, "..") == 0)
                        continue;

                    if (!findDirectory and (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) not_eq FILE_ATTRIBUTE_DIRECTORY or
                        findDirectory and (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
                    {
                        if (PathMatchSpecA(find_data.cFileName, filter.c_str()))
                            out.push_back(search_path + find_data.cFileName);
                    }

                    if (includeSubDirectory and (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
                        ps.push(search_path + find_data.cFileName + "/");

                } while (FindNextFileA(hFind, &find_data));
                FindClose(hFind);
            }
        }
        return out;
    }
#endif

#if defined(U_OS_LINUX)
    vector<string> find_files(const string &directory, const string &filter, bool findDirectory, bool includeSubDirectory)
    {
        string realpath = directory;
        if (realpath.empty())
            realpath = "./";

        char backchar = realpath.back();
        if (backchar not_eq '\\' and backchar not_eq '/')
            realpath += "/";

        struct dirent *fileinfo;
        DIR *handle;
        stack<string> ps;
        vector<string> out;
        ps.push(realpath);

        while (!ps.empty())
        {
            string search_path = ps.top();
            ps.pop();

            handle = opendir(search_path.c_str());
            if (handle not_eq 0)
            {
                while (fileinfo = readdir(handle))
                {
                    struct stat file_stat;
                    if (strcmp(fileinfo->d_name, ".") == 0 or strcmp(fileinfo->d_name, "..") == 0)
                        continue;

                    if (lstat((search_path + fileinfo->d_name).c_str(), &file_stat) < 0)
                        continue;

                    if (!findDirectory and !S_ISDIR(file_stat.st_mode) or
                        findDirectory and S_ISDIR(file_stat.st_mode))
                    {
                        if (pattern_match(fileinfo->d_name, filter.c_str()))
                            out.push_back(search_path + fileinfo->d_name);
                    }

                    if (includeSubDirectory and S_ISDIR(file_stat.st_mode))
                        ps.push(search_path + fileinfo->d_name + "/");
                }
                closedir(handle);
            }
        }
        return out;
    }
#endif

    bool begin_with(const string &str, const string &with)
    {
        if (str.length() < with.length())
            return false;
        return strncmp(str.c_str(), with.c_str(), with.length()) == 0;
    }

    bool end_with(const string &str, const string &with)
    {
        if (str.length() < with.length())
            return false;

        return strncmp(str.c_str() + str.length() - with.length(), with.c_str(), with.length()) == 0;
    }

    vector<string> split_string(const string &str, const std::string &spstr)
    {
        vector<string> res;
        if (str.empty())
            return res;
        if (spstr.empty())
            return {str};

        auto p = str.find(spstr);
        if (p == string::npos)
            return {str};

        res.reserve(5);
        string::size_type prev = 0;
        int lent = spstr.length();
        const char *ptr = str.c_str();

        while (p != string::npos)
        {
            int len = p - prev;
            if (len > 0)
                res.emplace_back(str.substr(prev, len));
            prev = p + lent;
            p = str.find(spstr, prev);
        }

        int len = str.length() - prev;
        if (len > 0)
            res.emplace_back(str.substr(prev, len));
        return res;
    }

    string replace_string(const string &str, const string &token, const string &value, int nreplace, int *out_num_replace)
    {

        if (nreplace == -1)
            nreplace = str.size();

        if (nreplace == 0)
            return str;

        string result;
        result.resize(str.size());

        char *dest = &result[0];
        const char *src = str.c_str();
        const char *value_ptr = value.c_str();
        int old_nreplace = nreplace;
        bool keep = true;
        string::size_type pos = 0;
        string::size_type prev = 0;
        size_t token_length = token.length();
        size_t value_length = value.length();

        do
        {
            pos = str.find(token, pos);
            if (pos == string::npos)
            {
                keep = false;
                pos = str.length();
            }
            else
            {
                if (nreplace == 0)
                {
                    pos = str.length();
                    keep = false;
                }
                else
                    nreplace--;
            }

            size_t copy_length = pos - prev;
            if (copy_length > 0)
            {
                size_t dest_length = dest - &result[0];

                // Extended memory space if needed.
                if (dest_length + copy_length > result.size())
                {
                    result.resize((result.size() + copy_length) * 1.2);
                    dest = &result[dest_length];
                }

                memcpy(dest, src + prev, copy_length);
                dest += copy_length;
            }

            if (keep)
            {
                pos += token_length;
                prev = pos;

                size_t dest_length = dest - &result[0];

                // Extended memory space if needed.
                if (dest_length + value_length > result.size())
                {
                    result.resize((result.size() + value_length) * 1.2);
                    dest = &result[dest_length];
                }
                memcpy(dest, value_ptr, value_length);
                dest += value_length;
            }
        } while (keep);

        if (out_num_replace)
            *out_num_replace = old_nreplace - nreplace;

        // Crop extra space
        size_t valid_size = dest - &result[0];
        result.resize(valid_size);
        return result;
    }

    string align_blank(const string &input, int align_size, char blank)
    {
        if (input.size() >= align_size)
            return input;
        string output = input;
        for (int i = 0; i < align_size - input.size(); ++i)
            output.push_back(blank);
        return output;
    }

    std::tuple<uint8_t, uint8_t, uint8_t> hsv2bgr(float h, float s, float v)
    {
        const int h_i = static_cast<int>(h * 6);
        const float f = h * 6 - h_i;
        const float p = v * (1 - s);
        const float q = v * (1 - f * s);
        const float t = v * (1 - (1 - f) * s);
        float r, g, b;
        switch (h_i)
        {
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 4:
            r = t;
            g = p;
            b = v;
            break;
        case 5:
            r = v;
            g = p;
            b = q;
            break;
        default:
            r = 1;
            g = 1;
            b = 1;
            break;
        }
        return make_tuple(static_cast<uint8_t>(b * 255), static_cast<uint8_t>(g * 255), static_cast<uint8_t>(r * 255));
    }

    std::tuple<uint8_t, uint8_t, uint8_t> random_color(int id)
    {
        float h_plane = ((((unsigned int)id << 2) ^ 0x937151) % 100) / 100.0f;
        float s_plane = ((((unsigned int)id << 3) ^ 0x315793) % 100) / 100.0f;
        return hsv2bgr(h_plane, s_plane, 1);
    }

    short invert_short(short v)
    {
        return ((v & 0x00FF) << 8) | ((v & 0xFF00) >> 8);
    }

    int invert_int(int v)
    {
        return ((v & 0x000000FF) << 24) | ((v & 0x0000FF00) << 8) | ((v & 0x00FF0000) >> 8) | ((v & 0xFF000000) >> 24);
    }

    string join_dims(const vector<int> &dims)
    {
        stringstream output;
        char buf[64];
        const char *fmts[] = {"%d", " x %d"};
        for (int i = 0; i < dims.size(); ++i)
        {
            snprintf(buf, sizeof(buf), fmts[i != 0], dims[i]);
            output << buf;
        }
        return output.str();
    }
}