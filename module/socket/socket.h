#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <iostream>
#include <string>
#include "../logger/logger.h"


#define MYPORT  5030
#define SERVER_ADDR "192.168.1.106"
// #define SERVER_ADDR "127.0.0.1"
#define MAXLINE 5
#define BUFFER_SIZE 4096

// 去除文件名的路径前缀
#define FILENAME(x) strrchr(x,'/')?strrchr(x,'/')+1:x
// 定义宏函数 LOG_SOCKET(...)： 调用 SocketClient 单例的 log 方法，实现日志输出（定义宏的原因在于方便获取调用位置）
#define LOG_SOCKET(...)    SocketClient::getInstance().log((const std::string)(FILENAME(__FILE__)), (const std::string)(__FUNCTION__), (int)(__LINE__), __VA_ARGS__)

using namespace std;

class SocketClient 
{
    public:
        SocketClient(const SocketClient&) = delete;
        SocketClient& operator=(const SocketClient&) = delete;
        ~SocketClient();

        static SocketClient& getInstance() {
            static SocketClient socketclient;
            return socketclient;
        }

        void sendMsg(string msg);

        void recvMsg();

        void log(const std::string fileName, const std::string functionName, int lineNumber, const std::string msg, LogLevel level);

    private:
        SocketClient();

        int sock_cli;
        fd_set rfds;
        struct timeval tv;
        int retval, maxfd;

        bool connected = false;

        bool reconnect();

};
