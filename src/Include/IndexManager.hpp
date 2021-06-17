#pragma once
#include "BufferManager.hpp"

class bnode : public node
{
    bnode(const node &src);
    ~bnode();
};

class IndexManager
{
private:
    hword indexFileAddr;

public:
    //设置本次运行中INDEX文件的文件地址
    int setIndexFileAddr(hword fileAddr) { indexFileAddr = fileAddr; }
    //返回记录的完整地址，若未找到返回0
    dword findAddrEntry(const hword dataFileAddr, const int recordSize, void *keyValue);
    //若找到记录，将其解析成node类，若未找到返回无效node
    node findNodeEntry(const hword dataFileAddr, const int recordSize, void *keyValue);
    //插入一组键值与记录的地址，根地址由本函数更新
    int insertEntry();
    //传入一个filter，删除filter中删除记录对应的索引信息
    //根地址由本函数更新
    int deleteEntry();
    //在对应的属性上创建索引
    int createEntry();
    //删除对应属性上的索引
    int dropEntry();
};

extern IndexManager idxMgr;