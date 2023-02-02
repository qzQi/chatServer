#ifndef PUBLIC_H
#define PUBLIC_H

// server&&client代码需要的公共头文件

// 从收到的数据的msgid字段获取消息类型，为不同的类型编写不同的业务逻辑
enum EnMsgType {
    LOGIN_MSG = 1,  // 登录消息
    LOGIN_MSG_ACK,  // 登录消息的回复
    REG_MSG,        // 注册消息
    REG_MSG_ACK,    // 对注册消息的回应
    ONE_CHAT_MSG,   // 一对一聊天业务
    ADD_FRIEND_MSG, // 添加好友消息
};

// 各个业务数据格式
/*
1: {"msgid":1,"id":userid,"password":"pwd"};
2: {"msgid":2,"errno":0/2,}
3: {"msgid":3,”name":"uname","password":"pwd"}注册消息

用户聊天json格式
msgid：
id:senderID
from:"senderName"
toid: reveiverID
msg:"  XXXXXXXX"
*/

/*
目前使用的errno字段
0：业务正确
1：注册失败 regist
2：登录失败 login
*/
#endif