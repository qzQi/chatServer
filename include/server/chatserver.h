#ifndef CHATSERVER_H
#define CHARSERVER_H

// 聊天服务器的网络模块，基于muduo库的实现
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<string>

using std::string;
using namespace muduo::net;
using namespace muduo;

class ChatServer
{
public:
    ChatServer(EventLoop* loop,const InetAddress& listedAddr,const string& name);
    void start();
    // 为connected socket 注册，消息到达时候的回调
    void onMessage(const TcpConnectionPtr& conn,Buffer* buff,Timestamp);

    // TCP连接变化时候的回调 1、创建 2、销毁    无关：TcpServer在一个连接建立/断开的时候都会输出日志
    void onConnection(const TcpConnectionPtr& conn);
private:
    TcpServer _server;//组合muduo库
    EventLoop* _loop; //    
};

#endif