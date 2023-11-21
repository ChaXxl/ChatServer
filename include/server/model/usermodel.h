#ifndef USERMODEL_H
#define USERMODEL_H

#include <iostream>
#include "user.hpp"
#include "db.h"

// User 表的数据操作类
class UserModel {
public:
    // User 表的增加方法
    bool insert(User &user);

    // 更新用户状态信息
    bool updateState(User user);

    // 根据用户 id 查询用户信息
    User query(int id);

    // 重置用户状态信息
    void resetState();

private:

};


#endif
