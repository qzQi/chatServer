#ifndef PUBLIC_H
#define PUBLIC_H

// server&&client代码需要的公共头文件

// 从收到的数据的msgid字段获取消息类型，为不同的类型编写不同的业务逻辑
enum EnMsgType{
    LOGIN_MSG =1,//登录消息
    
    REG_MSG,     //注册消息
    REG_MSG_ACK, //对注册消息的回应
};

#endif