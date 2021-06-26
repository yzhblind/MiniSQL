#pragma once

#include <map>
#include <string>
#include <vector>
#include <cstring>

#define SUCCESS 0
#define FAILURE -1

typedef unsigned short int hword;
typedef unsigned int word;
typedef unsigned long long dword;

inline hword extractFileAddr(dword virtAddr) { return virtAddr >> 48; }
inline hword extractOffset(dword virtAddr) { return virtAddr & 0xFFFF; }
inline word extractBlockAddr(dword virtAddr) { return (virtAddr >> 16) & 0xFFFFFFFF; }
inline dword combileVirtAddr(hword fileAddr, word blockAddr, hword offset) { return (static_cast<dword>(fileAddr) << 48) | (static_cast<dword>(blockAddr) << 16) | static_cast<dword>(offset); }
inline int type2size(int type) { return type <= 1 ? 4 : type; }

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
class node;
class filter
{
private:
    const std::vector<attribute> &origin;
    std::vector<int> offset;
    std::vector<condExpr> cond;
    // int keyPos;
    bool check(const condExpr &c, void *record);

public:
    std::vector<void *> res;
    std::vector<dword> resAddr;
    filter(const std::vector<attribute> &origin);
    ~filter();
    inline int getOffset(int pos) { return offset[pos]; }
    inline int getSize() { return *offset.rbegin(); }
    int addCond(const condExpr &c);
    int push(const node &in);
    int push(void *record, dword vAddr, bool delFlag = false);
    // inline void setKeyPos(int pos) { keyPos = pos; }
};
struct element
{
    int type;
    void *ptr;
    element(int type, void *ptr) : type(type), ptr(ptr) {}
    element(const element &src) : type(src.type), ptr(src.ptr) {}
    void cpy(const element &src) { memcpy(ptr, src.ptr, type2size(type)); }
    friend bool operator<=(const element &a, const element &b);
    friend bool operator==(const element &a, const element &b);
    friend bool operator<(const element &a, const element &b);
};