#include "offlinemessagemodel.h"

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
bool OfflineMsgeModel::insert(int userid, std::string msg) {
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d, '%s')", userid, msg.c_str());

    MySQL mysql;
    if ( mysql.connect() ) {
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
bool OfflineMsgeModel::remove(int userid) {
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d", userid);

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
vector<string> OfflineMsgeModel::query(int userid) {
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid=%d", userid);

    vector<string> vec;
    MySQL mysql;
    if ( mysql.connect() ) {
        // 把 userid 用户中的离线消息放入 vec 中返回
        MYSQL_RES *res = mysql.query(sql);

        if (nullptr != res) {
            MYSQL_ROW row = mysql_fetch_row(res);

            while (nullptr != row) {
                vec.push_back(row[0]);
                row = mysql_fetch_row(res);
            }
        }

        mysql_free_result(res);
    }
    return vec;
}
