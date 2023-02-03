#ifndef GROUPUSER_H
#define GROUPUSER_H

// 中间表：组群&用户是多对多的关系。

#include "user.hpp"

// 群组用户，多了一个role角色信息，从User类直接继承，复用User的其它信息
class GroupUser : public User {
  public:
    void setRole(string role) { this->role = role; }
    string getRole() { return this->role; }

  private:
    // 该用户在组群的角色
    string role;
};

#endif