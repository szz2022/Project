#pragma once

#include "../common/header.h"

// 去除文件名的路径前缀
#define FILENAME(x) strrchr(x,'/')?strrchr(x,'/')+1:x
// 定义宏函数 LOG(...)： 调用 Logger 单例的 log_plus 方法，实现日志输出（定义宏的原因在于方便获取调用位置）
#define LOG_PLUS(...)    Logger::get_instance().log_plus((const std::string)(FILENAME(__FILE__)), (const std::string)(__FUNCTION__), (int)(__LINE__), __VA_ARGS__)

enum LogLevel{
    //运行日志
    LOGGER_INFO = 0,
    //DUBUG日志
    LOGGER_DEBUG = 1,
    //告警日志
    LOGGER_ALARM = 2,
    //错误日志
    LOGGER_ERROR = 3
};


enum LogType{
    LOGGER_ROTATINGH_TYPE = 0, //打印循环日志
    LOGGER_ASYNC_ROTATINHG_TYPE = 1 //异步打印循环日志
};

struct logger_config{
    //从配置文件加载
    std::string alarm_log_filepath = "../log/alarm.log";//告警日志
    std::string run_log_filepath = "../log/run.log";//运行日志

    // by_size按大小分割 by_day按天分割 by_hour按小时分割
    // std::string roll_type = "by_day";
    unsigned int reserve_count = 10;
    // 默认100 MB
    unsigned int roll_size = 1024 * 1024 * 100; 
    // 表示按天切割的时刻，具体是时刻通过rotation_hour:rotation_minute指定
    unsigned int rotation_hour = 0;
    unsigned int rotation_minute = 0;
    // 异步日志线程池大小
    unsigned int async_thread_pool_items_size = 8192;
    //异步日志线程池的线程数量
    unsigned int async_thread_pool_thread_size = 1;
};

//实现多参数不同类型传参，这个方法主要提供的是将日志格式形式化成字符串
template<typename... Args>
std::string format_multi_args(std::string fmt,Args... args){
    char targetString[1024];

    snprintf(targetString, sizeof(targetString), fmt.c_str(),args...);
    return targetString;
}


class Logger{
public:

    ~Logger();

    //日志参数配置初始化
    int init();

    void log(const std::string msg,LogLevel level);

    // new log: 除了输出 msg 之外，可输出 logger 的调用位置，包括文件名、函数名和行数
    void log_plus(const std::string fileName, const std::string functionName, int lineNumber, const std::string msg, LogLevel level);

    std::string format(std::string fmt,int num);
    std::string format(std::string fmt,int num1,int num2);
    
    // 单例创建日志类
    static Logger& get_instance(){
        static Logger logger; 
        return logger;
    }

private:   
    Logger(){
        init();
        std::cout << "logger初始化" << std::endl;
    }
    
    logger_config config;
   
    // 循环日志指针
    std::shared_ptr<spdlog::logger> rotating_logger;
    //异步循环日志
    std::shared_ptr<spdlog::logger> async_rotating_logger;
    //异步日志线程池
    // std::shared_ptr<spdlog::details::thread_pool> thread_pool_;
};
