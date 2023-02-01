#include "chatservice.h"
#include <functional>
#include "public.h"
#include "muduo/base/Logging.h"
// #include "json.hpp" //每次都会打开一次吗？

using namespace std;
using namespace std::placeholders;
using namespace muduo;

// 实现单例模式
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

// 用户登录业务代码，必然伴随着数据库的查询，如何将数据层与业务实现分离？
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp t) {
    LOG_INFO << "login service !"; // for debug
    /*
    用户登录消息，
    msgid:1  id=user.id  password=user.password通过查询数据库对用户身份进行验证
    y用户通过id和密码登录
    */
    User loginUser;
    int id = js["id"];
    string password = js["password"];
    loginUser = _userModel.query(id);
    // 用户存在，验证信息正确
    if (loginUser.getId() == id && loginUser.getPwd() == password) {
        if (loginUser.getState() == "online") {
            // 已登录用户不允许重复登录，返回登录失败信息
            json resp;
            resp["msgid"] = LOGIN_MSG_ACK;
            resp["errno"] = 2; // 目前使用了0，1，2 errno
            resp["errmsg"] = "this account is using, input another!";
            conn->send(resp.dump());
        } else {
            // 登录成功，返回成功信息，并更新数据库状态online
            //  UserModel新增update功能
            // loginUser.setState("onlien"); fix bug 状态只有lnline，单词拼写错了
            loginUser.setState("online");
            _userModel.updateState(loginUser);
            json resp;
            resp["msgid"] = LOGIN_MSG_ACK;
            resp["errno"] = 0; // 目前使用了0，1，2 errno
            resp["id"] = id;
            resp["name"] = loginUser.getName();
            conn->send(resp.dump());
        }
    } else {
        // 登录失败
        json resp;
        resp["msgid"] = LOGIN_MSG_ACK;
        resp["errno"] = 2; // 目前使用了0，1，2 errno
        resp["errmsg"] = "id or password is invalid!";
        conn->send(resp.dump());
    }
}

// 用户注册业务代码，只需用户提供name和，password；自动生成用户id，向数据库插入一条记录
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp t) {
    // LOG_INFO << "register service !";//for debug
    // 处理用户注册逻辑，实现与数据层操作的解耦
    // 这里要知道client--server之间通信的数据协议
    // 用户发送的格式有客户端控制&验证；必然包含name/pwd字段
    string name = js["name"];
    string pwd = js["password"];

    // 使用数据库表对象操作数据库
    User user;
    user.setName(name);
    user.setPwd(pwd);
    // userModel对象就是一个操作user表的方法，不需要频繁创建
    bool state = _userModel.insert(user);
    if (state) {
        // 成功注册数据，的处理逻辑
        // 预先规定好信息格式/协议  msgid字段
        LOG_INFO << user.getName() << " registe success !";
        json resp;
        resp["msgid"] = REG_MSG_ACK;
        resp["errno"] = 0;
        resp["id"] = user.getId();
        conn->send(resp.dump());

    } else {
        // 注册失败的处理逻辑
        LOG_INFO << user.getName() << " registe failed !";
        json resp;
        resp["msgid"] = REG_MSG_ACK;
        resp["errno"] = 1;
        resp["id"] = user.getId();
        conn->send(resp.dump());
    }
}

ChatService::ChatService() {
    // 初始化handlerMap
    _msgHandlerMap.insert(
        {LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});

    _msgHandlerMap.insert(
        {REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
}