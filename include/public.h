#ifndef PUBLIC_H
#define PUBLIC_H

// server&&client代码需要的公共头文件

// 从收到的数据的msgid字段获取消息类型，为不同的类型编写不同的业务逻辑
enum EnMsgType {
    LOGIN_MSG = 1, // 登录消息
    LOGIN_MSG_ACK, // 登录消息的回复
    REG_MSG,       // 注册消息
    REG_MSG_ACK,   // 对注册消息的回应
};

/*
目前使用的errno字段
0：业务正确
1：注册失败
2：登录失败
*/
#endif