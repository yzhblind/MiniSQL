#pragma once

#include <map>
#include <string>
#include <vector>
#include <cstring>

struct attribute
{
    std::string attrName;
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
    int fileAddr;
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
    const std::vector<attribute> &origin;
    const int pos;
    const op opr;
    const void *val;
    static const void *copyVal(const std::vector<attribute> &origin, const int pos, const void *val);
    condExpr(const std::vector<attribute> &origin, const int pos, const op opr, const void *val) : origin(origin), pos(pos), opr(opr), val(copyVal(origin, pos, val)) {}
    ~condExpr() { delete[] static_cast<const char *>(val); }
    condExpr(const condExpr &obj) : origin(obj.origin), pos(obj.pos), opr(obj.opr), val(copyVal(obj.origin, obj.pos, obj.val)) {}
};

struct element
{
    int type;
    void *ptr;
    element(int type, void *ptr) : type(type), ptr(ptr) {}
};