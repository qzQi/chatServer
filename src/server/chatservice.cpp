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
            {
                // 新增：记录用户登录连接
                lock_guard<mutex> lk(_connMutex);
                _userConnMap.insert({id, conn});
                _userConnMapReverse.insert({conn, id}); // from O(n)-->O(1)
            }
            // 登录成功，返回成功信息，并更新数据库状态online
            //  UserModel新增update功能
            // loginUser.setState("onlien"); fix bug
            // 状态只有lnline，单词拼写错了
            loginUser.setState("online");
            _userModel.updateState(loginUser);
            json resp;
            resp["msgid"] = LOGIN_MSG_ACK;
            resp["errno"] = 0; // 目前使用了0，1，2 errno
            resp["id"] = id;
            resp["name"] = loginUser.getName();

            // 新增，用户登录时候检查是否有离线信息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty()) {
                // 读取该用户的离线消息后，把该用户的所有离线消息删除掉
                // js["offlinemsg"] = vec;
                resp["offlinemsg"] = vec;
                _offlineMsgModel.remove(id);
            }
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

    // 增加一对一聊天业务
    _msgHandlerMap.insert(
        {ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});

    // 增加好友业务处理函数
    _msgHandlerMap.insert(
        {ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
}

// 处理用户异常退出，需要删除该用户在 在线用户map里面的信息
// 业务细节，用户异常断开连接muduo库回调onConnection函数；此时仅能得知一个参数
// 用户的就是TcpConnectionPtr
void ChatService::clientCloseException(const TcpConnectionPtr &conn) {
    User user;
    {
        lock_guard<mutex> lk(_connMutex);
        // // 进行一次迭代，效率比较低；有没有办法直接从conn找到用户id？
        // // 再创建一个map 由conn-》id ：空间换时间
        // for(auto it=_userConnMap.begin();it!=_userConnMap.end();it++){
        //     if(it->second==conn){
        //         user.setId(it->first);
        //         _userConnMap.erase(it);
        //         // return;
        //         break;
        //     }
        // }
        // int id=_userConnMapReverse
        if (_userConnMapReverse.count(conn) == 0) { return; }
        int id = _userConnMapReverse[conn];
        user.setId(id);
        _userConnMap.erase(id);
        _userConnMapReverse.erase(conn);
        // _userConnMap.erase(_userConnMap.begin());可以传入一个迭代器
    }
    // 如果的到该用户的在线信息，更新数据库状态
    if (user.getId() != -1) {
        user.setState("offline");
        _userModel.updateState(user); // 更新数据库信息为offline
    }
}

// 一对一聊天服务业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp t) {
    // 由用户发来的json信息，由msgid=ONE_CHAT_MSG获取该处理函数
    // 定义用户之间通信的json格式，
    // {"msgid":ONE_CHAT_MSG,"id":senderID,"name":senderName,"toid":reveiverID,"msg":"xxx"}
    LOG_INFO << "one chat service";
    // 只要知道对方的id就可以通信
    int toid = js["toid"].get<int>();
    // 如果用户在线，直接发送；不然存入offlinemessage表
    {
        lock_guard<mutex> lk(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end()) {
            // 用户在线，直接转发信息
            it->second->send(js.dump());
            return;
        }
    }
    // 用户不在线，转存信息为offlinemessage；开发offlinemessageModel
    // 这是单机版的实现，一个服务器集群如何获取其他主机上的信息呢？
    _offlineMsgModel.insert(toid, js.dump());
}

// 服务器异常，业务重置方法，每次服务器CTRL+C退出不用手动更新数据库；
// 以后增加其他信号处理函数
void ChatService::reset() {
    _userModel.resetState();
}

// 添加好友业务，
void ChatService::addFriend(
    const TcpConnectionPtr &conn, json &js, Timestamp t) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}
