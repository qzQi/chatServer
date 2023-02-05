#ifndef REDIS_H
#define REDIS_H
#include <hiredis/hiredis.h> //redis客户端开发头文件
#include <thread>
#include <string>
#include <functional>

using namespace std;

// 在redis的发布订阅里面，我们使用userid作为channel；
// usage：在服务器集群中，每个服务器都含有一个Redis对象作为redis客户端，
// 每个redis都要与redis-server进行通信；进行subscribe/publish的都相当于redis客户端
// 想想引入服务器中间件的目的：服务器集群下不同服务器之间的通信

class Redis {
  public:
    Redis();
    ~Redis();

    // 链接redis
    bool connect();

    // 向指定的channel发送msg
    bool publish(int channel, string message);

    // 向redis指定的通道subscribe订阅消息
    bool subscribe(int channel);

    // 用户下线时进行unsubscribe，取消订阅channel的信息，释放server为channel分配的缓冲
    bool unsubscribe(int channel);

    // 在独立的线程中接受订阅channel（userid）所发布的消息，
    // 并转发给message handler处理函数
    void receive_channel_message();

    // 由service进行该注册。
    void init_notify_message_handler(function<void(int, string)>);

  private:
  // 一个redisContext相当于我们的一个redis-cli，一个cli可以订阅很多channel
    redisContext *_publish_context;
    redisContext *_subscribe_context;

    // 回调操作，收到订阅消息后由service层处理业务。
    // 如何处理由service决定。   我们知道subscribe之后收到的数据包括三行
    function<void(int, string)> _notify_message_handler;
};
#endif