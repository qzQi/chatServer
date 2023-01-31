#ifndef DB_H
#define DB_H
// 实现MySQL的CRUD功能
#include<mysql/mysql.h>
// #include<muduo/base/Logging.h>
#include<string>

using std::string;

class MySQL
{
public:
    //初始化数据库连接
    MySQL();
    // 释放数据库连接，四次挥手
    ~MySQL();

    // 
    bool connect();
    // for :update/delete/insert
    bool update(string sql);

    // for select
    MYSQL_RES* query(string sql);

    // 
    MYSQL* getconn(){
        return _conn;
    }
private:
 MYSQL* _conn;
};

#endif