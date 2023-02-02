#include "db.hpp"
#include "offlinemessagemodel.hpp"
/*
用户登录，用户向在线用户发送信息会使用到
*/

// 存储用户的离线信息
void OfflineMsgModel::insert(int userid, string msg) {
    char sql[1024] = {0};
    sprintf(
        sql, "insert into offlinemessage values(%d, '%s')", userid,
        msg.c_str());
    MySQL mysql;
    if(mysql.connect()){
        // 这几处代码比较低效，直接用string传来传去
        mysql.update(sql);
    }
}

// 删除用户的离线信息
void OfflineMsgModel::remove(int userid) {
    char sql[1024]={0};
    sprintf(sql,"delete from offlinemessage where userid=%d",userid);
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

// 查询用户的离线信息
vector<string> OfflineMsgModel::query(int userid) {
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid=%d", userid);
    vector<string> vec;
    MySQL mysql;
    if (mysql.connect()) {
        // 成功连接数据库
        // res指向的heap区需要自己释放
        MYSQL_RES *res = mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            // 读取多行结果
            while((row=mysql_fetch_row(res))!=nullptr){
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}
