#include "usermodel.hpp"
#include "db.hpp" //use db operator
// User表的插入，新增用户；
// UserModel操控User对象，用户传入的仅有name/password
bool UserModel::insert(User &user) {
    // make sql
    char sql[1024] = {0};

    // bug:写的sql语句有问题，已修复
    sprintf(
        sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
        user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
    MySQL mysql;

    // 待改进使用数据库连接池！！！
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            // 获取刚刚插入的用户，生成的id
            user.setId(mysql_insert_id(mysql.getconn()));
            return true;
        }
    }
    return false;
}

// 通过用户id查询User表的信息，查询数据库代码 select
User UserModel::query(int id) {
    User user;
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id=%d", id);
    MySQL mysql;
    if (mysql.connect()) {
        // 成功连接数据库
        // res指向的heap区需要自己释放
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr) {
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
            }
        }
    }
    return user;
}

// 通过User对象更新数据库，更新数据库update
bool UserModel::updateState(User &user) {
    char sql[1024] = {0};
    sprintf(
        sql, "update user set state='%s' where id=%d", user.getState().c_str(),
        user.getId());
    
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            return true;
        }
    }
    return false;
}