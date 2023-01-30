#include "chatserver.h"
#include <muduo/base/Logging.h>
#include <functional>
#include "json.hpp"
#include "chatservice.h"

using namespace std;
using namespace placeholders;

using json = nlohmann::json;

ChatServer::ChatServer(
    EventLoop *loop, const InetAddress &listedAddr, const string &name)
    : _server(loop, listedAddr, name), _loop(loop) {
    _server.setConnectionCallback(
        std::bind(&ChatServer::onConnection, this, _1));

    _server.setMessageCallback(
        std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    _server.setThreadNum(4);
}

void ChatServer::start() {
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn) {
    // 处理客户的TCP连接变化 1、建立（不用管） 2、客户断开连接
}

// 收到客户发来的消息，已连接套接字可读的回调
void ChatServer::onMessage(
    const TcpConnectionPtr &conn, Buffer *buff, Timestamp t) {
    // 处理用户发过来的数据，网络层模块和业务层模块的交界处
    // 网络模块就负责数据的读写，具体的业务交给业务模块
    // 这个回调实在IO线程池/sub reactor里执行的，可以直接就在这执行；

    /*
    可以直接在这处理客户数据，但是这样就没法把网络模块与业务处理模块解耦。
    解决：可以让业务模块根据客户的数据/信息 来返回不同的回调函数，供这里使用
    返回业务处理函数，而不是暴漏所有的业务细节；使用接口/回调 解耦
    */
    // string msg = buff->retrieveAllAsString();
    // json js =
    // json::parse(msg);//bug：json解析不对，服务器直接down了；处理抛出的异常？
    // LOG_INFO << js.dump();

    // 打开vscode的报错功能吧
    string bufStr = buff->retrieveAllAsString();
    LOG_INFO << bufStr; // 测试代码

    json js = json::parse(bufStr);

    // 解耦！！！
    auto handler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    handler(conn, js, t);
}