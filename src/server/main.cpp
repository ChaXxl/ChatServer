#include <iostream>
#include "signal.h"
#include "chatserver.hpp"
#include "chatservice.hpp"

using namespace std;

// 处理服务器 Ctrl + C 结束后, 重置 user 的状态信息
void resetHandler(int) {
    ChatService::instance()->reset();
    exit(0);
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
int main(int argc, char *argv[]) {

    if (argc < 3) {
        std::cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << std::endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的 IP 和 port
    char *ip = argv[1];
    uint16_t port = std::atoi(argv[2]);

    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress listenAddr(ip, port);
//    InetAddress listenAddr("127.0.0.1", 8888);
//    InetAddress listenAddr("192.168.131.131", 8888);
//    InetAddress listenAddr("0.0.0.0", 8888);

    ChatServer server(&loop, listenAddr, "chatserver");

    server.start();
    loop.loop();

    return 0;
}
