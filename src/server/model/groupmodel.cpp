#include "groupmodel.hpp"
#include "db.hpp"

// 创建group，并通过引用返回分组信息
bool GroupModel::createGroup(Group &group) {
    // 向allgroup表中插入数据
    char sql[1024] = {0};
    sprintf(
        sql, "insert into allgroup(groupname,groupdesc) values('%s','%s')",
        group.getName().c_str(), group.getDesc().c_str());
    MySQL mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            group.setId(mysql_insert_id(mysql.getconn()));
            return true;
        }
    }
    return false;
}

// 加入群聊：userid，groupid
void GroupModel::addGroup(int userid, int groupid, string role) {
    // sql
    char sql[1024] = {0};
    sprintf(
        sql, "insert into groupuser values(%d, %d, '%s')", groupid, userid,
        role.c_str());
    MySQL mysql;
    if (mysql.connect()) { mysql.update(sql); }
}

// 查询用户所在群组信息，一个用户可以对应多个组群；为客户端呈现群组信息。
// group类里面有组群用户信息vector<GroupUser>
// 两次sql执行：1、查询用户的组群  2、查询每个组群里面的用户（用户的群友）
vector<Group> GroupModel::queryGroups(int userid) {
    /*
    1. 先根据userid在groupuser表中查询出该用户所属的群组信息
    2.
    在根据群组信息，查询属于该群组的所有用户的userid，并且和user表进行多表联合查询，查出用户的详细信息
    */
    char sql[1024] = {0};
    sprintf(
        sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join \
         groupuser b on a.id = b.groupid where b.userid=%d",
        userid);

    vector<Group> groupVec;

    MySQL mysql;
    if (mysql.connect()) {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            // 查出userid所有的群组信息
            while ((row = mysql_fetch_row(res)) != nullptr) {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }

    // 查询群组的用户信息
    for (Group &group : groupVec) {
        sprintf(
            sql, "select a.id,a.name,a.state,b.grouprole from user a \
            inner join groupuser b on b.userid = a.id where b.groupid=%d",
            group.getId());

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

// 根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务给群组其它成员群发消息
// 组要用于群聊，给除自己外的所有人发信,需要知道群内除自己外的所有用户的id
vector<int> GroupModel::queryGroupUsers(int userid, int groupid) {
    char sql[1024] = {0};
    sprintf(
        sql, "select userid from groupuser where groupid = %d and userid != %d",
        groupid, userid);

    vector<int> idVec;
    MySQL mysql;
    if (mysql.connect()) {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}
