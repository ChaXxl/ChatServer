#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <functional>
#include <string>
#include "json.hpp"
#include "chatservice.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;
using json = nlohmann::json;

// 聊天服务器主类
class ChatServer {
public:
    // 初始化聊天服务器对象
    ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg);

    // 启动服务器对象
    void start();

private:
    // 上报连接相关信息的回调函数
    void onConnection(const TcpConnectionPtr &);

    // 上报读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr &, Buffer *, Timestamp);

    // 基于 muduo 库实现服务器相关功能
    TcpServer _server;

    // 指向事件循环的指针
    EventLoop *_loop;
};

#endif
