#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include <string>
#include <vector>
#include "db.h"

using  namespace std;

// 离线消息表的操作方法
class OfflineMsgeModel {
public:
    // 存储用户的离线消息
    bool insert(int userid, string msg);

    // 删除用户的离线消息
    bool remove(int userid);

    // 查询用户的离线消息
    vector<string> query(int userid);
};


#endif
