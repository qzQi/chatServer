#include "redis.hpp"
#include <iostream>

Redis::Redis() : _subscribe_context(nullptr), _publish_context(nullptr) {
}

Redis::~Redis() {
    if (_publish_context != nullptr) { redisFree(_publish_context); }
    if (_subscribe_context != nullptr) { redisFree(_subscribe_context); }
}

// 连接redis，此时就开启channel的接收线程
bool Redis::connect() {
    //
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (_publish_context == nullptr) {
        cerr << "connect redis failed !" << endl;
        return false;
    }

    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (_subscribe_context == nullptr) {
        cerr << "connect redis failed !" << endl;
        return false;
    }

    // 在独立的线程中接受所订阅channel（userid）所发布的消息，
    // 并转发给message handler处理函数
    thread receiver([this]() { this->receive_channel_message(); });
    receiver.detach();

    // debug：忘记写返回值，导致产生未定义的行为！ 进而影响在service里面的注册回调
    cout<<"redis connect success !";
    return true;
}

// 向redis指定的通道subscribe订阅消息,redis-cli是阻塞在接受channel信息上面
// 这里我们使用专门的线程进行receive
bool Redis::subscribe(int channel){
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在receive_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    if(REDIS_ERR==redisAppendCommand(_subscribe_context,"subscribe %d",channel)){
        cerr<<"subscribe failed! "<<endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(_subscribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    // redisGetReply
    // 将接受channel发来的消息放在独立的线程

    return true;
}

// 向指定的channel发送msg
bool Redis::publish(int channel, string message) {
    redisReply *reply = (redisReply *)redisCommand(
        _publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply) {
        cerr << "publish command failed!" << endl;
        return false;
    }
    // reply指向heap内存
    freeReplyObject(reply);
    return true;
}

// 用户下线时进行unsubscribe，取消订阅channel的信息，释放server为channel分配的缓冲
// 在cli里面就不能实现这个，因为cli一直阻塞在接收
bool Redis::unsubscribe(int channel) {
    if(redisAppendCommand(_subscribe_context,"unsubscribe %d",channel)==REDIS_ERR){
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(_subscribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 在独立的线程中接受订阅channel（userid）所发布的消息，
// 并转发给message handler处理函数
// 对subscribe接收到的数据进行处理
void Redis::receive_channel_message() {
    redisReply *reply = nullptr;
    while (REDIS_OK
           == redisGetReply(this->_subscribe_context, (void **)&reply)) {
        // 订阅收到的消息是一个带三元素的数组
        if (reply != nullptr && reply->element[2] != nullptr
            && reply->element[2]->str != nullptr) {
            // 给业务层上报通道上发生的消息
            _notify_message_handler(
                atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }
}

// 由service进行该注册。service进行注册该回调
void Redis::init_notify_message_handler(function<void(int, string)> f) {
    _notify_message_handler = f;
}