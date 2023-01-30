#include "chatservice.h"
#include <functional>
#include "public.h"
#include "muduo/base/Logging.h"

using namespace std;
using namespace std::placeholders;
using namespace muduo;

ChatService *ChatService::instance() {
    static ChatService service; // default init
    return &service;
}

// 返回相应的处理函数
MsgHandler ChatService::getHandler(int msgid) {
    if (_msgHandlerMap.count(msgid)) {
        return _msgHandlerMap[msgid];
    } else {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp t) {
            LOG_ERROR << "msgid: " << msgid << " can not found";
        };
    }
}

// 用户登录业务代码
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp t) {
}

// 用户注册业务代码
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp t) {
}

ChatService::ChatService() {
    // 初始化handlerMap
    _msgHandlerMap.insert(
        {LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});

    _msgHandlerMap.insert(
        {REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
}