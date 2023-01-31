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