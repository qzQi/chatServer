#include "friendModel.hpp"
#include "db.hpp"

// 添加好友关系
void FriendModel::insert(int userid, int friendid) {
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d,%d);", userid, friendid);
    MySQL mysql;
    if (mysql.connect()) { mysql.update(sql); }
}

// 返回用户好友列表
vector<User> FriendModel::query(int userid) {
    // 联合查询，userid用户的好友信息
    // select id,name,state from user
    // inner join friend on friend.friendid=user.id
    // where friend.userid=13;
    // 1、组装sql 2、进行数据库查询 3、提取数据库查询信息
    char sql[1024] = {0};
    sprintf(
        sql,
        "select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d",
        userid);
    MySQL mysql;
    vector<User> vec;
    if (mysql.connect()) {
        MYSQL_RES *res = mysql.query(sql);
        // MYSQL_ROW row=mysqlqure
        if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
        }
    }
    return vec;
}