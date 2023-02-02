#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

// 数据库User表的操作类，
class UserModel
{
public:
    // User表的插入，新增用户, 先实现一个用户注册&User插入代码吧
    bool insert(User& user);

    // 通过id主键，查询用户信息
    User query(int id);

    // 设置用户在线状态
    bool updateState(User& user);

    // 重置用户的状态信息
    void resetState();
};

#endif