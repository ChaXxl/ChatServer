#include "chatserver.hpp"

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg): _server(loop, listenAddr, nameArg), _loop(loop) {
//     注册连接回调函数
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

//     注册消息回调函数
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

//     设置线程数量
    _server.setThreadNum(6);
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
void ChatServer::start() {
    _server.start();
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
void ChatServer::onConnection(const TcpConnectionPtr &conn) {
//     用户断开连接
    if (!conn->connected()) {
        ChatService::instance()->clientCloseExecption(conn);
        conn->shutdown();
    }
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time) {
     string buf = buffer->retrieveAllAsString();

     // 数据反序列化
     json js = json::parse(buf);

     auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());

    //  回调消息绑定好的事件处理器, 来执行相应的业务处理
     msgHandler(conn, js, time);
}
