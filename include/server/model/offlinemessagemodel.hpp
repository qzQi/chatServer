#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

// 操作offlinemessage的类。 该表只有id和message
// 只需要定义，插入，查询，删除。
#include<string>
#include<vector>
using namespace std;
class OfflineMsgModel
{
public:
    // 存储用户的离线信息
    void insert(int userid,string msg);

    // 删除用户的离线信息
    void remove(int userid);

    // 查询用户的离线信息
    vector<string> query(int userid);
};


#endif