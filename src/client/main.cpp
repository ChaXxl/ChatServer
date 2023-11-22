#include <vector>
#include <ctime>
#include <chrono>
#include <string>
#include <thread>
#include <iostream>
#include "json.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "chatservice.hpp"
#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

using namespace std;
using json = nlohmann::json;

// 控制主菜单界面
bool isMainMenurunning = false;

// 记录当前系统登录的用户信息
User g_currentUser;

// 记录当前登录用户的好有列表信息
vector<User> g_currentUserFriendList;

// 显示当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 用于读写线程之前的通信
sem_t rwsem;

// 记录登录状态
atomic_bool g_isLoginSuccess{false};

// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 接收线程
void readTaskHandler(int clientfd);

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();

// 聊天主页面程序
void mainMenu(int);

/*
 * 函数名称:
 * 函数功能: 聊天客户端程序实现, main 线程用作发送线程, 子线程用作接收线程
 * 参数:
 * 返回值: 无
 * */
int main(int argc, char **argv) {
    if (argc < 3) {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }
    // 解析通过命令行参数传递的 IP 和 port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建 client 端的 socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd) {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写 client 需要连接的 server 信息 ip + port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(port);
    server.sin_family = AF_INET;

    // client 和 server 进行连接
    if (-1 == connect(clientfd, (sockaddr *) &server, sizeof(sockaddr_in))) {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

   // 初始化读写线程通信用的信号量. 信号量、0表示用于线程、初始为0
    sem_init(&rwsem, 0, 0);

    // 登录成功, 启动接收线程负责接收数据
    std::thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    // main 用于接收用户输入
    for (;;) {
        // 显示首页页面菜单 登录、注册、退出Ω
        cout << endl;
        cout << "====================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "====================" << endl;
        cout << "choice:";

        int choice = 0;
        cin >> choice;
        cin.get();  // 读取缓冲区残留的回车

        switch (choice) {
            case 1: {// login 业务
                int id = 0;
                char pwd[50] = {0};

                cout << "userid:";
                cin >> id;
                cin.get();

                cout << "password:";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), request.length() + 1, 0);

                if (-1 == len) {                                        // 发送失败
                    cerr << "send login msg error:" << request << endl;
                }
                else {                                                  // 发送成功
                    char buffer[1024] = {0};
                    len = recv(clientfd, buffer, 1024, 0);
                    if (-1 == len) {                                    // 接收失败
                        cerr << "receive login response error" << endl;
                    }
                    else {                                            // 接收成功
                        json response = json::parse(buffer);
                        if (0 != response["errno"].get<int>()) {    // 登录失败
                            cerr << response["errmsg"] << endl;
                        }
                        else {                                    // 登录成功
                            // 记录当前用户的 id 和 name
                            g_currentUser.setId(response["id"].get<int>());
                            g_currentUser.setName(response["name"]);

                            // 记录当前用户的好友列表信息
                            if (response.contains("friends")) {
                                g_currentUserFriendList.clear();    // 初始化

                                vector<string> vec = response["friends"];
                                for (auto &str: vec) {
                                    json js = json::parse(str);
                                    User user;
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    g_currentUserFriendList.push_back(user);
                                }
                            }

                            // 记录当前用户的群组列表消息
                            if (response.contains("groups")) {
                                // 群
                                g_currentUserGroupList.clear(); // 初始化
                                vector<string> vec_group = response["groups"];
                                for (auto groupstr: vec_group) {
                                    json js = json::parse(groupstr);

                                    Group group;
                                    group.setId(js["id"].get<int>());
                                    group.setName(js["groupname"]);
                                    group.setDesc(js["groupdesc"]);

                                    // 群员
                                    vector<string> vec_user = js["users"];
                                    for (auto userstr: vec_user) {
                                        json js = json::parse(userstr);

                                        GroupUser user;
                                        user.setId(js["id"].get<int>());
                                        user.setName(js["name"]);
                                        user.setState(js["state"]);
                                        user.setRole(js["role"]);

                                        group.addUsers(user);
                                    }
                                    g_currentUserGroupList.push_back(group);
                                }
                            }

                            // 显示登录用户的基本信息
                            showCurrentUserData();

                            // 显示当前用户的离线消息 个人消息或者群组消息
                            if (response.contains("offlinemsg")) {
                                vector<string> vec = response["offlinemsg"];
                                for (auto &str: vec) {
                                    json js = json::parse(str);

                                    int msgType = js["msgid"].get<int>();

                                    if (ONE_CHAT_MSG == msgType) {
                                        cout << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>()
                                             << " said: " << js["msg"].get<string>() << endl;
                                    }
                                    else if (GROUP_CHAT_MSG == msgType) {
                                        cout << "群消息 [" << js["groupid"] << "]:" <<  js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>()
                                             << " said: " << js["msg"].get<string>() << endl;
                                    }
                                }
                            }

                            // 进入聊天主菜单页面
                            isMainMenurunning = true;
                            mainMenu(clientfd);
                        }
                    }
                }

                break;
            }

            case 2: {// register 业务
                char name[50] = {0};
                char pwd[50] = {0};

                cout << "username:";
                cin.getline(name, 50);

                cout << "password:";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), request.length() + 1, 0);
                if (-1 == len) {
                    cerr << "send register error: " << request << endl;
                } else {
                    char buffer[1024] = {0};
                    len = recv(clientfd, buffer, 1024, 0);
                    if (-1 == len) {
                        cerr << "receive register response error" << endl;
                    } else {
                        json response = json::parse(buffer);
                        if (0 != response["errno"].get<int>()) {
                            cerr << name << " is already exist, register error!" << endl;
                        } else {
                            cout << name << " register successfully, userid is " << response["id"]
                                 << " , do not forget it!" << endl;
                        }
                    }
                }

                break;
            }
            case 3: // 退出
                close(clientfd);
                sem_destroy(&rwsem);
                exit(0);

                break;
            default:
                cerr << "invalid input!" << endl;;
                break;
        }
    }
    return 0;
}

/*
 * 函数名称:
 * 函数功能: 接收线程
 * 参数:
 * 返回值: 无
 * */
void readTaskHandler(int clientfd) {
    for (;;) {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (-1 == len || 0 == len) {
            close(clientfd);
            exit(-1);
        }

        json js = json::parse(buffer);
        int msgType = js["msgid"].get<int>();

        if (ONE_CHAT_MSG == msgType) {
            cout << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        else if (GROUP_CHAT_MSG == msgType) {
            cout << "群消息 [" << js["groupid"] << "]:" <<  js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
    }
}

// 显示当前登录成功用户的基本信息
/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
void showCurrentUserData() {
    cout << "=======================login user=======================" << endl;
    cout << "current login user => " << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "-----------------------friend list-----------------------" << endl;
    if (!g_currentUserFriendList.empty()) {
        for (auto &user: g_currentUserFriendList) {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "-----------------------group list-----------------------" << endl;
    if (!g_currentUserGroupList.empty()) {
        for (auto &group: g_currentUserGroupList) {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (auto &groupUser: group.getUsers()) {
                cout << groupUser.getId() << " " << groupUser.getName() << " " << groupUser.getState()
                     << " " << groupUser.getRole() << endl;
            }
        }
    }
    cout << "=======================================================" << endl;
}


void help(int fd = 0, string str = "");                    // chat command handler
void chat(int, string);             // chat command handler
void addfriend(int, string);        // addfriend command handler
void creategroup(int, string);      // creategroup command handler
void addgroup(int, string);         // addgroup command handler
void groupchat(int, string);        // groupchat command handler
void logout(int, string);           // quit command handler



// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
        {"help",        "显示所有支持的命令, 格式 help"},
        {"chat",        "一对一聊天, 格式 chat:friendid:message"},
        {"addfriend",   "添加好友, 格式 addfriend:friendid"},
        {"creategroup", "创建群组, 格式 creategroup:groupname:groupdesc"},
        {"addgroup",   "加入群组, 格式 addgroup:groupid"},
        {"groupchat",   "群组, 格式 groupchat:groupid:message"},
        {"logout",      "注销, 格式 quit"}
};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
        {"help",        help},
        {"chat",        chat},
        {"addfriend",   addfriend},
        {"creategroup", creategroup},
        {"addgroup",    addgroup},
        {"groupchat",   groupchat},
        {"quit",      logout},
};

/*
 * 函数名称:
 * 函数功能: 获取系统时间（聊天信息需要添加时间信息）
 * 参数:
 * 返回值: 无
 * */
string getCurrentTime() {
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);

    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int) ptm->tm_year + 1900, (int) ptm->tm_mon + 1, (int) ptm->tm_mday,
            (int) ptm->tm_hour + 8, (int) ptm->tm_min, (int) ptm->tm_sec
    );
    return std::string(date);
}

// 主聊天页面程序
/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
void mainMenu(int clientfd) {
    help();

    char buffer[1024] = {0};
    while (isMainMenurunning) {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 存储命令
        int idx = commandbuf.find(":");

        if (-1 == idx) {
            command = commandbuf;
        } else {
            command = commandbuf.substr(0, idx);
        }

        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end()) {
            cerr << "invalid input command!" << endl;
            continue;
        }

        // 调用相应的事件处理回调, mainMenu 对修改封闭, 添加新功能不需要修改函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.length() - idx));
    }
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
void help(int, string) {
    cout << "show command list >>>" << endl;
    for (auto &m: commandMap) {
        cout << m.first << " : " << m.second << endl;
    }
    cout << endl;
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
void chat(int clientfd, string str) {
    int idx = str.find(":");    // friendid:message
    if (-1 == idx) {
        cerr << "chat command invalid!" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.length() - 1);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), buffer.length() + 1, 0);
    if (-1 == len) {
        cerr << "send chat msg error -> " << buffer << endl;
    }
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
void addfriend(int clientfd, string str) {
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), buffer.length() + 1, 0);
    if (-1 == len) {
        cerr << "send addfriend msg error -> " << buffer << endl;
    }
}

/*
 * 函数名称: creategroup
 * 函数功能: 创建群组
 * 参数: clientfd ; groupInfo 群名:群描述
 * 返回值: 无
 * */
void creategroup(int clientfd, string groupInfo) {
    // 查找 : 的下标
    int idx = groupInfo.find(":");

    if (-1 == idx) {
        cerr << "creategroup command invalid!" << endl;
        return;
    }

    // 提取群组名称
    string groupName = groupInfo.substr(0, idx);

    // 提取群组的描述
    string groupDesc = groupInfo.substr(idx + 1, groupInfo.length() - groupName.length() - 1);

    // 组装请求体
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupName;
    js["groupdesc"] = groupDesc;

    // 将 json 对象转为字符串
    string buffer = js.dump();

    // 通过 clientfd 发送请求
    int len = send(clientfd, buffer.c_str(), buffer.length() + 1, 0);

    // 发送错误
    if (-1 == len) {
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}

/*
 * 函数名称: addgroup
 * 函数功能: 加入群组
 * 参数:
 * 返回值: 无
 * */
void addgroup(int clientfd, string groupId) {
    // 组装请求体
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = std::stoi(groupId);

    // 将 json 对象转为字符串
    string buffer = js.dump();

    // 通过 clientfd 发送请求
    int len = send(clientfd, buffer.c_str(), buffer.length() + 1, 0);

    // 发送错误
    if (-1 == len) {
        cerr << "send addgroup msg error -> " << buffer << endl;
    }
}

/*
 * 函数名称: groupchat
 * 函数功能: 群组聊天
 * 参数:
 * 返回值: 无
 * */
void groupchat(int clientfd, string str) {
    // 查找 : 的下标
    int idx = str.find(":");

    if (-1 == idx) {
        cerr << "groupchat command invalid!" << endl;
        return;
    }

    // 提取群组id
    int groupid = std::stoi(str.substr(0, idx));

    // 提取要发送的群消息
    string message = str.substr(idx + 1, str.length() - idx);

    // 组装请求体
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    js["name"] = g_currentUser.getName();
    js["msg"] = message;
    js["time"] = getCurrentTime();

    // 将 json 对象转为字符串
    string buffer = js.dump();

    // 通过 clientfd 发送请求
    int len = send(clientfd, buffer.c_str(), buffer.length() + 1, 0);

    // 发送错误
    if (-1 == len) {
        cerr << "send groupchat msg error -> " << buffer << endl;
    }
}

/*
 * 函数名称: logout
 * 函数功能: 退出登录(注销)
 * 参数:
 * 返回值: 无
 * */
void logout(int clientfd, string str) {
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = g_currentUser.getId();

    string buffer = js.dump();


    int len = send(clientfd, buffer.c_str(), buffer.length() + 1 ,0);

    // 发送错误
    if (-1 == len) {
        cerr << "send logout msg error -> " << buffer << endl;
    }
    else {
        isMainMenurunning = false;
    }
}