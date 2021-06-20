#pragma once
#include "Type.hpp"
#include "BufferManager.hpp"

// INDEX节点首部保留2字节标识节点类型（根节点、叶节点、中间节点）
// 4字节保留用于parent指针，仅需于每次find的路径上更新，不需要强一致
// 4字节保留记录当前节点key值数量
// 6字节ptr和定长keyValue共n-1组
// 6字节ptr

enum packing
{
    PTR_DATA,
    DATA_PTR
};

class bnode : public node
{
private:
    static void *tmpMemory;
    //0-leaf，1-inode
    //(base+1)-root
    char *base;
    word *parent;
    int *cnt;
    inline int index2offset(int index, int type) { return 10 + (type2size(type) + 6) * index; };
    //使用时保证*cnt > 0
    //返回第一个大于等于e的元素的index
    int binarySearch(const element &e);
    void *move(int start, int dir, int type, const packing packingType = PTR_DATA);
    //void *rmove(int start, int dir, int type);

public:
    friend class IndexManager;
    bnode(const node &src);
    ~bnode();
    inline int getBase() { return *base; }
    inline int getCnt() { return *cnt; }
    inline word getParent() { return *parent; }
    inline void setParent(word blockAddr) { *parent = blockAddr; }
    inline element getElement(int idx, int type) { return element(type, base + index2offset(idx, type) + 6); }
    dword find(const element &key, const packing packingType = PTR_DATA);
    //返回插入位置的index，需保证节点未满
    int insertKey(const element &key, const dword virtAddr, const packing packingType = PTR_DATA);
    int deleteKey(const element &key, const packing packingType = PTR_DATA);
    int replaceKey(const element &key, const element &newKey);
    bnode split(const element &key, const dword virtAddr);
    int splice(bnode &par, bnode &src, int type);
    //返回第一个小于父节点中应被删除索引的元素
    element merge(bnode &par, bnode &src, int type);
    //于硬盘上删除bnode对应的块
    void deleteBlock() { origin.deleteBlock(getFileAddr(), getBlockAddr()); }
};

class IndexManager
{
private:
    hword indexFileAddr;
    //返回可能存在key的叶节点块地址
    word find(const word curAddr, const word parentAddr, const element &keyValue);
    //如果更新了根节点返回根节点的blockAddr，否则返回0
    word insertInParent(bnode &cur, const element &key, const word addr);
    word erase(bnode &cur, const element &keyValue, const packing delType);
    void drop(word curAddr, const int type);

public:
    int bcnt[257];
    int bInMincnt[257];
    int bLeafMincnt[257];
    IndexManager();
    ~IndexManager();
    //设置本次运行中INDEX文件的文件地址
    void setIndexFileAddr(hword fileAddr) { indexFileAddr = fileAddr; }
    //返回记录的完整地址，若未找到返回0
    dword findAddrEntry(const hword dataFileAddr, const word rootAddr, const element &keyValue);
    //若找到记录，将其解析成node类，若未找到返回无效node
    node findRecordEntry(const hword dataFileAddr, const word rootAddr, const element &keyValue);
    //插入一组键值与记录的地址，根地址由本函数更新
    int insertEntry(attribute &attr, const element &keyValue, dword virtAddr);
    //传入一个filter，删除filter中删除记录对应的索引信息
    //根地址由本函数更新
    int deleteEntry(const hword dataFileAddr, std::vector<attribute> &col, const filter &flt);
    //在对应的属性上创建索引
    int createEntry(const hword dataFileAddr, std::vector<attribute> &col, int pos);
    //删除对应属性上的索引
    int dropEntry(std::vector<attribute> &col, int pos);
};

extern IndexManager idxMgr;