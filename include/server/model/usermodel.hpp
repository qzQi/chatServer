#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

// 数据库User表的操作类，
class UserModel
{
public:
    // User表的插入，新增用户, 先实现一个用户注册&User插入代码吧
    bool insert(User& user);
};

#endif