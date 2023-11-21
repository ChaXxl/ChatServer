#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <functional>
#include <unordered_map>
#include <mutex>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Logging.h>
#include "json.hpp"
#include "public.hpp"

#include "redis.hpp"
#include "usermodel.h"
#include "offlinemessagemodel.h"
#include "friendmodel.h"
#include "groupmodel.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// 表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

// 聊天服务器业务类
class ChatService {
public:
    // 处理客户端异常退出
    void clientCloseExecption(const TcpConnectionPtr &conn);

    // 获取单例对象的接口函数
    static ChatService* instance();

    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注销业务
    void logout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 服务端异常，业务重置方法
    void reset();

    //创建群组业务
    bool createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 加入群组业务
    bool addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    // 从 redis 消息队列中获取订阅的消息
    void handleRedisSubscribeMsg(int userid, string msg);

private:
    ChatService();

    // 存储已登录用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 存储消息 id 和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 定义互斥锁, 保证 _userConnMap 的线程安全
    mutex _connMutex;

    UserModel _userModel;               // 用户数据模型
    OfflineMsgeModel _offlineMsgMode;   // 用户离线消息数据模型
    FriendModel _friendModel;           // 添加好友数据模型
    GroupModel _groupModel;             // 群组数据模型

    // redis 操作对象
    Redis _redis;
};

#endif
