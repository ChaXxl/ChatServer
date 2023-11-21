#include "db.h"

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
MySQL::MySQL() {
    _conn = mysql_init(nullptr);
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
MySQL::~MySQL() {
    if (nullptr != _conn) {
        mysql_close(_conn);
    }
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
MYSQL *MySQL::getConnection() {
    return _conn;
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
bool MySQL::connect() {
    MYSQL *p = mysql_real_connect(_conn,
                                  server.c_str(),
                                  user.c_str(),
                                  password.c_str(),
                                  dbname.c_str(),
                                  3306,
                                  nullptr,
                                  0
    );
    if (nullptr != p) {
        // C/C++代码默认编码是 ASCII, 设置防止中文乱码
        mysql_query(_conn, "set names gbk");
    }
    return p;
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
bool MySQL::update(string sql) {
    if (mysql_query(_conn, sql.c_str())) {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ": " <<
                    sql << "; 更新失败!";
        return false;
    }
    return true;
}

/*
 * 函数名称:
 * 函数功能:
 * 参数:
 * 返回值: 无
 * */
MYSQL_RES *MySQL::query(string sql) {
    if (mysql_query(_conn, sql.c_str())) {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ": " <<
                    sql << "; 查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}
