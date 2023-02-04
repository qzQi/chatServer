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
#include <mutex> //

#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendModel.hpp"
#include "groupmodel.hpp"

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
    MsgHandler getHandler(int);

    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp t);

    // 处理用户注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp t);

    // 一对一聊天服务业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp t);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp t);

    // 创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp t);

    // 加入群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp t);

    // 群聊业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp t);
    // 处理用户异常退出，需要删除该用户在在线用户map里面的信息
    void clientCloseException(const TcpConnectionPtr &);

    // 服务器CTRL+C退出时重置一下user表的状态
    // 服务器异常，业务重置方法，不用每次手动更新数据库了
    void reset();

  private:
    ChatService(); // 单例实现
    unordered_map<int, MsgHandler>
        _msgHandlerMap; // 由相应的msgId返回对应的回调

    // 数据库表操作类对象,避免频繁创建
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    // 存储在线用户的id及其tcp连接，实现在线用户之间的通信！C/S模型
    // 用户登陆时需要加入map，用户下线（主动/异常）时删除map记录
    // 看下是否意外延长了TcpConnectionPtr的生命==》没有
    unordered_map<int, TcpConnectionPtr> _userConnMap; // need mutex
    // 为了方便快速从conn找到用户id
    unordered_map<TcpConnectionPtr, int> _userConnMapReverse;
    // 定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;
};

#endif