#include "groupmodel.hpp"

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
bool GroupModel::createGrop(Group &group) {
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s','%s')",
            group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
bool GroupModel::addGroup(int userid, int groupid, string role) {
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values(%d, %d,'%s')",
            groupid, userid, role.c_str());

    MySQL mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            return true;
        }
    }
    return false;
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
vector<Group> GroupModel::queryGroups(int userid) {
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc \
                 from allgroup a inner join groupuser b \
                 on a.id=b.groupid where b.userid=%d", userid
    );


    vector<Group> groupVec;
    MySQL mysql;
    if (mysql.connect()) {
        MYSQL_RES *res = mysql.query(sql);
        MYSQL_ROW row;
        if (nullptr != res) {
            while (nullptr != (row = mysql_fetch_row(res))) {
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
    for (auto &group: groupVec) {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole \
                               from user a inner join groupuser b \
                               on a.id=b.userid where b.groupid=%d\
        ", group.getId());
        MYSQL_RES *res = mysql.query(sql);
        MYSQL_ROW row;
        if (nullptr != res) {
            while (nullptr != (row = mysql_fetch_row(res))) {
                GroupUser groupuser;
                groupuser.setId(atoi(row[0]));
                groupuser.setName(row[1]);
                groupuser.setState(row[2]);
                groupuser.setRole(row[3]);

                group.addUsers(groupuser);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
vector<int> GroupModel::queryGroupUsers(int userid, int groupid) {
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid=%d and userid!=%d",
            groupid, userid);

    vector<int> idVec;
    MySQL mysql;
    if (mysql.connect()) {
        MYSQL_RES *res = mysql.query(sql);
        if (nullptr != res) {
            MYSQL_ROW row;
            while (nullptr != (row = mysql_fetch_row(res))) {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}


