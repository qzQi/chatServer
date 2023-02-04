#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include "vector"
using namespace std;
// 组群表ORM类，增加了vector<GroupUer>
class Group {
  public:
    Group(int id = -1, string name = "", string desc = "") {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }

    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getDesc() { return this->desc; }
    // 这个目前感觉没啥作用
    vector<GroupUser> &getUsers() { return this->users; }

  private:
    int id;
    string name;
    string desc;
    // 组群用户，存放groupid下的所有用户信息。一个用户可以多个组群，每个组群里面多个组用户！
    vector<GroupUser> users;//感觉不需要这个字段;有点过度设计来？
    // 已经有一个用于查询组群里面用户id的方法了，这个没必要查询其他的信息了
    // 只是为了给客户端使用，显示一下组群信息
};
#endif