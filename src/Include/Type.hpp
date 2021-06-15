#pragma once

#include <map>
#include <string>
#include <vector>

struct attribute
{
    // 0:int
    // 1:float
    // 2-256:char(2-256) [包含'\0']
    int type;
    // 0表示NULL
    int indexRootAddr;
    // false表示不唯一，true表示唯一
    bool isUnique;
};
struct schema
{
    std::string tableName;
    std::map<std::string, int> attributeIndex;
    std::vector<attribute> column;
    int primaryKey;
    int recordSize;
};