#include "chatservice.hpp"

/*
 * 函数名称: ChatService::ChatService
 * 函数功能: 注册消息以及对应的 Handler 回调操作
 * 参数:
 * 返回值: 无
 * */
ChatService::ChatService() {
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)});

    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关事件注册回调
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接 redis 服务器
    if (_redis.connect()) {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMsg, this, _1, _2));
    }
}

ChatService *ChatService::instance() {
    static ChatService *instance_ = new ChatService();
    return instance_;
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
MsgHandler ChatService::getHandler(int msgid) {
    // 记录错误日志, msgid 没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end()) {
        // 返回一个空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) {
            LOG_ERROR << "msgid: " << msgid << " can not find handler!";
        };
    } else {
        return _msgHandlerMap[msgid];
    }
}

/*
 * 函数名称:
 * 函数功能: 处理登录业务
 * 参数:
 * 返回值: 无
 * */
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int id = js["id"].get<int>();
    string password = js["password"];

    User user = _userModel.query(id);

    if (user.getId() == id && user.getPassword() == password) {
        if ("online" == user.getState()) {
            // 该用户已在线, 不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = " 该帐号已登录, 请重新输入新账号";

            conn->send(response.dump());
        } else {
            // 登录成功

            {
                lock_guard<mutex> lock(_connMutex);

                // 记录用户连接信息
                _userConnMap.insert({id, conn});
            }

            // 用户 id 登录成功后，向 redis 订阅 channel(id)
            _redis.subscribe(id);

            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["errmsg"] = "ok";
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 用户的离线消息
            vector<string> vec = _offlineMsgMode.query(id);
            if (!vec.empty()) {
                response["offlinemsg"] = vec;
                // 读取用户离线消息后, 将其离线消息删除
                _offlineMsgMode.remove(id);
            }

            // 用户的好友列表
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty()) {
                vector<string> vec2;
                for (auto &u: userVec) {
                    json js;
                    js["id"] = u.getId();
                    js["name"] = u.getName();
                    js["state"] = u.getState();

                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 用户的群组列表消息
            vector<Group> groupVec = _groupModel.queryGroups(id);
            if (!groupVec.empty()) {
                vector<string> vec3;
                for (auto &group: groupVec) {
                    json js;
                    js["id"] = group.getId();
                    js["groupname"] = group.getName();
                    js["groupdesc"] = group.getDesc();

                    // 获取群成员
                    vector<string> vec_users;
                    for (auto &groupuser: group.getUsers()) {
                        json groupUsers_json;
                        groupUsers_json["id"] = groupuser.getId();
                        groupUsers_json["name"] = groupuser.getName();
                        groupUsers_json["state"] = groupuser.getState();
                        groupUsers_json["role"] = groupuser.getRole();

                        vec_users.push_back(groupUsers_json.dump());
                    }

                    js["users"] = vec_users;

                    vec3.push_back(js.dump());
                }
                response["groups"] = vec3;
            }

            conn->send(response.dump());
        }

    } else {
        // 登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或密码错误";

        conn->send(response.dump());
    }
}

/*
 * 函数名称:
 * 函数功能: 处理注销业务
 * 参数:
 * 返回值: 无
 * */
void ChatService::logout(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int id = js["id"].get<int>();

    // 加锁操作
    {
        lock_guard<mutex> lock(_connMutex);

        // 在 map 中查找
        auto it = _userConnMap.find(id);

        // 从连接表中移除id对应的项
        if (it != _userConnMap.end()) {
            _userConnMap.erase(it);
        }
    }

    // 创建 User 对象
    User user(id, "", "", "offline");

    // 用户注销，相当于下线，则在 redis 中取消订阅通道
    _redis.unsubscribe(id);

    // 更新用户状态信息
    _userModel.updateState(user);
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);

    bool state = _userModel.insert(user);
    if (true == state) {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();

        conn->send(response.dump());

    } else {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;

        conn->send(response.dump());
    }
}

/*
 * 函数名称:
 * 函数功能: 处理客户端异常退出
 * 参数:
 * 返回值: 无
 * */
void ChatService::clientCloseExecption(const TcpConnectionPtr &conn) {
    User user;

    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it) {
            if (it->second == conn) {
                user.setId(it->first);
                // 从 map 表中删除用户的连接信息
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于下线，则在 redis 中取消订阅通道
    _redis.unsubscribe(user.getId());

    // 更新用户状态信息
    if (-1 != user.getId()) {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int toid = js["toid"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (_userConnMap.end() != it) {
            // toid 在线, 服务器主动推送消息给 toid 用户
            it->second->send(js.dump());
            return;
        }
    }

    // 如果在 ConnectionMap 中没找到该用户，则要么其下线了、要么在别的服务器登录
    // 所以要在数据库中查找其在线状态, 并把消息发到对应的通道上
    User user = _userModel.query(toid);
    if ("online" == user.getState()) {
        _redis.publish(toid, js.dump());
        return;
    }

    // toid 用户未登录, 存储离线消息
    _offlineMsgMode.insert(toid, js.dump());
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
void ChatService::reset() {
    _userModel.resetState();
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储用户好友信息
    _friendModel.insert(userid, friendid);
}

/*
 * 函数名称: ChatService::createGroup
 * 函数功能: 创建群组
 * 参数:
 * 返回值: 无
 * */
bool ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];

    Group group(-1, groupname, groupdesc);

    bool res = _groupModel.createGrop(group);

    if (res) {
        // 把创建人加入群组
        _groupModel.addGroup(userid, group.getId(), "creator");
    }

    return res;
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
bool ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    bool res = _groupModel.addGroup(userid, groupid, "normal");

    return res;
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);

    for (auto &id: useridVec) {
        auto it = _userConnMap.find(id);
        if (_userConnMap.end() != it) {
            // 用户在线, 转发群消息
            it->second->send(js.dump());
            return;
        }
        else {
            // 查询 userid 是否在线
            User user = _userModel.query(userid);
            if ("online" == user.getState()) {
                _redis.publish(id, js.dump());
                return;
            }
        }

        // 用户离线, 存储离线消息
        _offlineMsgMode.insert(id, js.dump());
    }
}

/*
 * 函数名称:
 * 函数功能: 从 redis 消息队列中获取订阅的消息
 * 参数:
 * 返回值: 无
 * */
void ChatService::handleRedisSubscribeMsg(int userid, string msg) {
    std::lock_guard<std::mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
    }

    // 存储该用户的离线消息
    _offlineMsgMode.insert(userid, msg);
}
