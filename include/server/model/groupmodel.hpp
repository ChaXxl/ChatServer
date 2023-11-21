#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include <vector>
#include "group.hpp"
#include "db.h"

using namespace std;

// 群组信息的操作接口
class GroupModel {
public:
    // 创建群组
    bool createGrop(Group &group);

    // 加入群组
    bool addGroup(int userid, int groupid, string role);

    // 查询用户所在群组的信息
    vector<Group> queryGroups(int userid);

    // 根据 groupid 查询群组用户 id 列表(除了自己)(用于给群成员发送消息)
    vector<int> queryGroupUsers(int userid, int groupid);
};


#endif //GROUPMODEL_H
