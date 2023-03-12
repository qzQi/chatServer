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
// 新增：订阅组群实现一个群消息的广播实现，需要修改usermoder==》返回用户所在的所有group，这个已实现
// 修改groupmodel实现查找出所有的不在线组员
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

            // 订阅自己的channel，在实现跨服务器的单聊时候很方便
            _redis.subscribe(to_string(id));

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

            // 新增，返回用户的好友信息；也是一次查询数据库
            vector<User> vecUser = _friendModel.query(id);
            if (!vecUser.empty()) {
                // 如何读取vector user
                // js["frinends"]=vecUser;//?User是自定义的，无法解析
                vector<string> vecUserStr;
                for (auto &u : vecUser) {
                    json strUser;
                    strUser["id"] = u.getId();
                    strUser["name"] = u.getName();
                    strUser["state"] = u.getState();
                    vecUserStr.push_back(strUser.dump());
                }
                resp["friends"] = vecUserStr; // 标准库可以直接放入json
            }

            // 供客户端登录时候显示的信息：用户组群信息
            // 新增：用户登录之后订阅自己的所有的组群！用于实现在线用户之间的广播
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty()) {
                // 每个组的id，name，desc，所有用户
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec) {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    // 订阅用户所在的所有组群，用于实现群聊的在线用户广播实现！
                    // 但是如何区分用户id与组id？改一下订阅的内容！
                    // 组群的订阅都加上group前缀
                    _redis.subscribe("group"+to_string(group.getId()));

                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers()) {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                resp["groups"] = groupV;
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

    _msgHandlerMap.insert(
        {LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});

    // 增加一对一聊天业务
    _msgHandlerMap.insert(
        {ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});

    // 增加好友业务处理函数
    _msgHandlerMap.insert(
        {ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 组群业务相关事件处理回调注册
    _msgHandlerMap.insert(
        {CREATE_GROUP_MSG,
         std::bind(&ChatService::createGroup, this, _1, _2, _3)});

    _msgHandlerMap.insert(
        {ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});

    _msgHandlerMap.insert(
        {GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // fix bug:connect()返回值未定义！！！
    if (_redis.connect()) {
        // 连接redis服务器，并进行对订阅消息处理函数的注册
        // 绑定函数别忘了取地址！！！
        _redis.init_notify_message_handler(
            std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
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

    if(user.getId()!=-1){
        _redis.unsubscribe(to_string(user.getId()));
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
    LOG_INFO << "one chat service";//debug
    // 只要知道对方的id就可以通信
    // 单机下所有客户在同一台服务器，集群下需要再查数据库
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

    // 新增：实现集群服务器之间的通信，
    // 先查询该用户在不在别的服务器上
    User user=_userModel.query(toid);
    // TODO：可新增缓存功能，使用redis来查看是否在线来减少mysql的压力
    if(user.getState()=="online"){
        // bug1：进行oneChat业务时，跨服务器通信，导致对方down了，自己没事
        _redis.publish(to_string(toid),js.dump());
        return ;
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

// 创建群组
void ChatService::createGroup(
    const TcpConnectionPtr &conn, json &js, Timestamp t) {
    // 业务细节：用户创建组群，从js里面获取相应的信息
    // 信息格式：
    int userid = js["id"].get<int>();
    string groupName = js["groupname"];
    string groupDesc = js["groupdesc"];

    // allgroup表的orm对象&&操作
    Group group(-1, groupName, groupDesc);
    if (_groupModel.createGroup(group)) {
        // 存储创建用户为creator
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组
void ChatService::addGroup(
    const TcpConnectionPtr &conn, json &js, Timestamp t) {
    // 业务细节：获取userid&&groupid，然后操作orm&&model
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群聊业务,群发信息：对方在线？直接发送，不在线存储离线
// 这一块的业务实现效率太低了，如果能够实现一个类似于广播的东西就好了
// 可以直接给在线的人使用消息队列广播，然后再查数据库看看哪些不在线的再存储离线消息！
// select * from groupuser  
// inner join user on groupuser.userid=user.id 
// where groupid=1 and state="offline";
// void ChatService::groupChat(
//     const TcpConnectionPtr &conn, json &js, Timestamp t) {
//     // 实现：从数据库获取user所要发送组群的所有用户id
//     int userid = js["id"];
//     int groupid = js["groupid"];
//     vector<int> uidVec = _groupModel.queryGroupUsers(userid, groupid);

//     lock_guard<mutex> lk(_connMutex);
//     for (int id : uidVec) {
//         auto it = _userConnMap.find(id);
//         if (it != _userConnMap.end()) {
//             // 如果组员在线直接发送，消息
//             it->second->send(js.dump());
//         } else {
//             // // 单机版本的实现，如果服务集群如何处理？
//             // // 若用户在其他服务器在线？
//             // _offlineMsgModel.insert(id, js.dump());
//             // 1、查询toid是不是在其他服务器登录
//             User user=_userModel.query(id);
//             if(user.getState()=="online"){
//                 // 向订阅该channel（也就是userid）的，发布信息
//                 _redis.publish(to_string(id),js.dump());
//             }else{
//                 // 用户不在线
//                 _offlineMsgModel.insert(id,js.dump());
//             }
//         }
//     }
// }

// 直接修改组群聊天的实现：
// 1、在线用户必然已经订阅了 eg：”group0001“  
// 2：进行一次数据库连接查询找出本组群所有不在线的用户，然后存储一下离线消息
// 组群业务的协议格式：知道那个用户 userid，在哪个群组里面群聊groupid
void ChatService::groupChat(
    const TcpConnectionPtr &conn, json &js, Timestamp t){
    int userid=js["id"];
    int groupid=js["groupid"];

    // 向在线组群在线用户广播
    _redis.publish("group"+to_string(groupid),js.dump());

    // 进行一次联合查询，查找groupid下的所有不在线用户！需要修改groupmodel，添加方法！
    vector<int> offlineUsers=_groupModel.queryOfflineUsers(groupid);
    // 但是不管怎么搞，业务上中有一些瑕疵，比如在添加离线消息时候用户上线了？
    // 这个情况很有必要用上缓存！快速的判断一个用户是否在线！
    for(int i:offlineUsers){
        // 使用redis判断用户i是否在线！在线的话直接onchat，不在的话存数据库
        _offlineMsgModel.insert(i,js.dump());
    }
}

// 处理用户注销业务
void ChatService::loginout(
    const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end()) { _userConnMap.erase(it); }
        auto itcon = _userConnMapReverse.find(conn);
        if (itcon != _userConnMapReverse.end()) {
            _userConnMapReverse.erase(itcon);
        }
    }

    // // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(to_string(userid));

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 对redis所接收到的订阅信息进行业务处理，并注册消息处理函数
// 我们使用userid作为channel，
// 从所订阅的回复消息里面可以得知channelname==》userid，message
void ChatService::handleRedisSubscribeMessage(string userid, string msg) {
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(atoi(userid.c_str()));
    if (it != _userConnMap.end()) {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.insert(atoi(userid.c_str()), msg);
}