#include "friendmodel.h"

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
bool FriendModel::insert(int userid, int friendid) {
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);

    MySQL mysql;
    if ( mysql.connect() ) {
        if ( mysql.update(sql) ) {
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
vector <User> FriendModel::query(int id) {
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid=a.id where b.userid=%d", id);

    vector<User> vec;
    MySQL mysql;
    if ( mysql.connect() ) {
        MYSQL_RES *res = mysql.query(sql);
        if (nullptr != res) {
            // 把 id 用户的所有好友放入 vec 中并返回
            MYSQL_ROW row;
            while ( nullptr != (row = mysql_fetch_row(res)) ) {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}
