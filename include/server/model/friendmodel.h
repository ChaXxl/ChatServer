#ifndef ADDFRIENDMODEL_H
#define ADDFRIENDMODEL_H

#include <iostream>
#include <vector>
#include "db.h"
#include "usermodel.h"

using  namespace std;

class FriendModel {
public:
    // 添加好友关系
    bool insert(int userid, int friendid);

    // 返回用户好友列表
    vector<User> query(int id);
};


#endif
