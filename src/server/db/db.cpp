#include "db.hpp"
// 如果查找不到路径往往就是CMakeLists.txt没有写好
#include <muduo/base/Logging.h>

using namespace std;
using namespace muduo;

// 数据库配置信息，不想写从配置文件读取了，直接使用static 变量
// 别忘记修改了
static string ServerIP = "101.42.23.45";
static string user = "qzyDB";
static string password = "helloQzy";
static string dbname = "chat";

// 初始化数据库连接
MySQL::MySQL() {
    _conn = mysql_init(nullptr);
}
// 释放数据库连接，四次挥手
MySQL::~MySQL() {
    if (_conn != nullptr) { mysql_close(_conn); }
}

//
bool MySQL::connect() {
    MYSQL *p = mysql_real_connect(
        _conn, ServerIP.c_str(), user.c_str(), password.c_str(), dbname.c_str(),
        3306, nullptr, 0);
    
    if(p!=nullptr){
        // success
        // 设置C/C++支持中文
        mysql_query(_conn,"set names gbk");
        LOG_INFO<<"connect mysql success !";
    }else{
        LOG_INFO<<"connect mysql failed !";
    }
    return p!=nullptr;//bug code p==nullptr
}
// for :update/delete/insert,不需要返回结果,更新数据库
bool MySQL::update(string sql) {
    if(mysql_query(_conn,sql.c_str())){
        LOG_INFO<<__FILE__<<":"<<__LINE__<<":"
            <<sql<<"更新失败";
        // 如何获取更新失败的原因？
        LOG_INFO<<mysql_errno(_conn)<<mysql_error(_conn);
        return false;
    }
    return true;
}

// for select，需要返回查询结果
MYSQL_RES *MySQL::query(string sql) {
    if(mysql_query(_conn,sql.c_str())){
        LOG_INFO<<__FILE__<<":"<<__LINE__<<":"
            <<sql<<"查询失败";
        return nullptr;
    }
    return mysql_use_result(_conn);
}
