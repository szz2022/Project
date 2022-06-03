#include "logger.h"
#include <mutex>

Logger::~Logger()
{
    std::cout << "~logger" << std::endl;
    spdlog::drop_all();
}


int Logger::init()
{
    //1.首先先从配置文件中加载我们需要的参数来进行初始化
    
    //2.一次性创建所有的logger
    // try 
    // {
    //     // 可以使用std::make_shared来分配对象
    //     rotating_logger = spdlog::rotating_logger_mt("rotating_logger", config.run_log_filepath , 
    //                                                   config.roll_size, config.reserve_count); 
        
    //     //遇到err日志立马刷盘
    //     rotating_logger->flush_on(spdlog::level::err);
        
    //     // spdlog::register_logger(rotating_logger);
    // }
    // catch (const spdlog::spdlog_ex& ex)
    // {
    //     std::cout << "Rotating Logger initialization failed: " << ex.what() << std::endl;
    // }

    try 
    {
        spdlog::init_thread_pool(config.async_thread_pool_items_size,config.async_thread_pool_thread_size);
        // async_rotating_logger = spdlog::rotating_logger_mt<spdlog::async_factory>("async_rotating_logger", config.run_log_filepath , 
        //                                               config.roll_size, config.reserve_count); 

        async_rotating_logger = spdlog::daily_logger_mt<spdlog::async_factory>("async_rotating_logger", config.run_log_filepath , 
                                                      config.rotation_hour, config.rotation_minute); 
        // spdlog::register_logger(async_rotating_logger);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "Async Rotating Logger initialization failed: " << ex.what() << std::endl;
    }
    spdlog::flush_every(std::chrono::seconds(2));

    return 1;
}

std::string Logger::format(std::string fmt,int num)
{    
    char targetString[1024];
    snprintf(targetString, sizeof(targetString), fmt.c_str(),num);
    return targetString;
}

std::string Logger::format(std::string fmt,int num1,int num2)
{    
    char targetString[1024];
    snprintf(targetString, sizeof(targetString), fmt.c_str(),num1,num2);
    return targetString;
}

void Logger::log(const std::string msg,LogLevel level)
{
    
    //1.提供循环写文件功能 
    if(level == LOGGER_INFO)
    {
        async_rotating_logger->info(msg);
    }
    else if(level == LOGGER_ALARM)
    {
        async_rotating_logger->warn(msg);
    }
    else if(level == LOGGER_ERROR)
    {
        async_rotating_logger->error(msg);
    }

    //2.向远端发送请求，日志格式是啥，[厂商ID 设备号 时间 msg]

}

void Logger::log_plus(const std::string fileName, const std::string functionName, int lineNumber, const std::string msg, LogLevel level){
    // 在原有 msg 基础上添加文件名、函数名和行号
    std::string msgPlus = "["+fileName+"] ["+functionName+": "+std::to_string(lineNumber)+"] "+msg;
    // 调用原有 log 方法
    log(msgPlus, level);
}



