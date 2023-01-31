#ifndef CHATSERVICE_H
#define CHATSERVICE_H

/*
实现业务逻辑代码：
ChatService编写为一个单例，仅仅为了使用成员函数处理业务，不需要多个对象，全局一个即可

客户和服务器通过json通信，msgid字段表明消息类型
*/

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include "json.hpp"

#include "usermodel.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;

using json = nlohmann::json;

using MsgHandler =
    function<void(const TcpConnectionPtr &conn, json &js, Timestamp t)>;

class ChatService {
  public:
    // 单例实现
    static ChatService *instance();

    // 由消息类型从map里面返回相应的业务处理handler
    MsgHandler getHandler(int );

    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp t);

    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp t);

  private:
    ChatService();                                 // 单例实现
    unordered_map<int, MsgHandler> _msgHandlerMap; // 由相应的msgId返回对应的回调

    // 数据库表操作类对象,避免频繁创建
    UserModel _userModel;
};

#endif