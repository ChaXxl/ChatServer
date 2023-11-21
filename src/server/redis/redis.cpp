
#include "redis.hpp"

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
Redis::Redis() : _publish_context(nullptr), _subscribe_context(nullptr) {

}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
Redis::~Redis() {
    if (nullptr != _publish_context) {
        redisFree(_publish_context);
    }
    if (nullptr != _subscribe_context) {
        redisFree(_subscribe_context);
    }
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
bool Redis::connect() {
    // 负责 publish 发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379);

    if (nullptr == _publish_context) {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    // 密码
    redisReply *authReply = nullptr;
    authReply = static_cast<redisReply *>(redisCommand(_publish_context, "AUTH %s", "123"));
    if (authReply->type == REDIS_REPLY_ERROR) {
        std::cerr << "Failed to connect to Redis server with provided password." << std::endl;
        freeReplyObject(authReply);
        return false;
    }

    // 负责 subscribe 订阅消息的上下文连接
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _subscribe_context) {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    // 密码
    authReply = nullptr;
    authReply = static_cast<redisReply *>(redisCommand(_subscribe_context, "AUTH %s", "123"));
    if (authReply->type == REDIS_REPLY_ERROR) {
        std::cerr << "Failed to connect to Redis server with provided password." << std::endl;
        freeReplyObject(authReply);
        return false;
    }
    freeReplyObject(authReply);

    // 在单独的线程中监听通道上的事件，有消息则给业务层进行上报
    std::thread t([&]() {
       observer_channel_message();
    });
    t.detach();

    std::cout << "connect redis-server success!" << std::endl;

    return true;
}

/*
 * 函数名称:
 * 函数功能: 向 redis 指定的通道 channel 发布消息
 * 参数:
 * 返回值: 无
 * */
bool Redis::publish(int channel, std::string message) {
    redisReply *reply = (redisReply*) redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply) {
        std::cerr << "publish command failed!" << std::endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

/*
 * 函数名称:
 * 函数功能: 向 redis 指定通道 subscribe 订阅消息
 * 参数:
 * 返回值: 无
 * */
bool Redis::subscribe(int channel) {
   // subscribe 命令本身会造成线程阻塞等待里面发生消息，这里只做订阅通道，不接收通道消息
   // 通道消息的接收专门在 observer_channel_message 函数中的独立线程中进行
   // 只负责发送命令，不阻塞接收 redis server 响应消息，否则和 notifyMsg 线程抢占响应资源

   if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel)) {
       std::cerr << "subscribe command failed!" << std::endl;
       return false;
   }

   // redisBufferWrite 可以循环发送缓冲区，直到缓冲区数据发送完毕（done 被置为 1）
   int done = 0;
   while (1 != done) {
       if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done)) {
           std::cerr << "subscribe command failed!" << std::endl;
           return false;
       }
   }

    return true;
}

/*
 * 函数名称:
 * 函数功能: 向 redis 指定通道 unsubscribe 取消订阅消息
 * 参数:
 * 返回值: 无
 * */
bool Redis::unsubscribe(int channel) {
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel)) {
        std::cerr << "unsubscribe command failed!" << std::endl;
        return false;
    }
    // redisBufferWrite 可以循环发送缓冲区，直到缓冲区数据发送完毕（done 被置为 1）
    int done = 0;
    while (1 != done) {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done)) {
            std::cerr << "unsubscribe command failed!" << std::endl;
            return false;
        }
    }
    return true;
}

/*
 * 函数名称:
 * 函数功能: 在独立线程中接收订阅通道中的消息
 * 参数:
 * 返回值: 无
 * */
void Redis::observer_channel_message() {
    redisReply *reply = nullptr;
    while(REDIS_OK == redisGetReply(this->_subscribe_context, (void **)&reply)) {
        // 订阅收到的消息是一个带3个元素的数组
        if(nullptr != reply && nullptr != reply->element[2] && nullptr != reply->element[2]->str) {
            // 给业务层上报通道上发生的消息 通道号、消息
            _notify_message_handler(std::atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    std::cerr << ">>>>>>>>>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<<<<<<<<" << std::endl;
}

/*
 * 函数名称:
 * 函数功能: 初始化向业务层上报通道消息的回调对象
 * 参数:
 * 返回值: 无
 * */
void Redis::init_notify_handler(std::function<void(int, std::string)> fn) {
    this->_notify_message_handler = fn;
}
