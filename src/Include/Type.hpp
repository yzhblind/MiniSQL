#pragma once

#include <map>
#include <string>
#include <vector>
#include <cstring>

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

enum op
{
    E,  // =
    NE, // <>
    L,  // <
    G,  // >
    LE, // <=
    GE  // >=
};
struct condExpr
{
    const attribute &origin;
    const op opr;
    const void *val;
    static const void *copyVal(const attribute &origin, const void *val);
    condExpr(const attribute &origin, const op opr, const void *val) : origin(origin), opr(opr), val(copyVal(origin, val)) {}
    ~condExpr() { delete[] static_cast<const char*>(val); }
};
const void *condExpr::copyVal(const attribute &origin, const void *val)
{
    void *res = origin.type <= 1 ? new char[4] : new char[origin.type];
    return memcpy(res, val, origin.type <= 1 ? 4 : origin.type);
}