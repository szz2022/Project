#include "socket.h"

SocketClient::SocketClient()
{
    ///定义sockfd
    // sock_cli = socket(AF_INET,SOCK_STREAM, 0);
    // ///定义sockaddr_in
    // struct sockaddr_in servaddr;
    // memset(&servaddr, 0, sizeof(servaddr));
    // servaddr.sin_family = AF_INET;
    // servaddr.sin_port = htons(MYPORT);  ///服务器端口
    // servaddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);  ///服务器ip

    // //连接服务器，成功返回0，错误返回-1
    // if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    // {
    //     cout << "error" << endl;
    //     connected = false;
    //     return;
    // }
    connected = true;
}

SocketClient::~SocketClient()
{

}

void SocketClient::sendMsg(string msg) 
{
    if(!connected) {
        cout << "未连接,将进行重连";
        reconnect();
    }
    if(connected && (send(sock_cli, msg.c_str(), msg.size(), 0)) == -1) 
    {
        // 发送失败，断开重连
        if(reconnect())
            send(sock_cli, msg.c_str(), msg.size(), 0); // 重发消息
        else 
            cout << "reconect fail" << endl;
    } 
}

void SocketClient::recvMsg()
{
    cout << "recv" << endl;
    while(1){
        /*把可读文件描述符的集合清空*/
        FD_ZERO(&rfds);
        maxfd = 0;
        /*把当前连接的文件描述符加入到集合中*/
        FD_SET(sock_cli, &rfds);
        /*找出文件描述符集合中最大的文件描述符*/   
        if(maxfd < sock_cli)
            maxfd = sock_cli;
        /*设置超时时间*/
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        /*等待聊天*/
        retval = select(maxfd+1, &rfds, NULL, NULL, &tv);
        if(retval == -1){
            printf("select出错，客户端程序退出\n");
            break;
        }else if(retval == 0){
            printf("客户端没有任何输入信息，并且服务器也没有信息到来，waiting...\n");
            continue;
        }else{
            // cout << "客户端消息" << endl;
            /*服务器发来了消息*/
            if(FD_ISSET(sock_cli,&rfds)){
                char recvbuf[BUFFER_SIZE];
                int len;
                len = recv(sock_cli, recvbuf, sizeof(recvbuf),0);
                printf("%s", recvbuf);
                memset(recvbuf, 0, sizeof(recvbuf));
            }
        }
    }
}

bool SocketClient::reconnect()
{
    close(sock_cli);
    sock_cli = socket(AF_INET,SOCK_STREAM, 0);
    ///定义sockaddr_in
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(MYPORT);  ///服务器端口
    servaddr.sin_addr.s_addr = inet_addr("192.168.1.105");  ///服务器ip

    //连接服务器，成功返回0，错误返回-1
    if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        cout << "reconnect error" << endl;
        connected = false;
        return false;
    }
    connected = true;
    return true;
}

void SocketClient::log(const std::string fileName, const std::string functionName, int lineNumber, const std::string msg, LogLevel level){
    // 1. 记录日志到本地
    Logger::get_instance().log_plus(fileName, functionName, lineNumber, msg, level);
    //TODO: 2. 上传告警日志至服务器

}

