#pragma once
#include "Type.hpp"
#include "BufferManager.hpp"

// INDEX节点首部保留2字节标识节点类型（根节点、叶节点、中间节点）
// 4字节保留用于parent指针，仅需于每次find的路径上更新，不需要强一致
// 4字节保留记录当前节点key值数量
// 6字节ptr和定长keyValue共n-1组
// 6字节ptr

//用于指定函数执行时的打包方式
//PTR_DATA表示将key与左侧指针打包视作整体理解
//DATA_PTR表示将key与右侧指针打包视作整体理解
enum packing
{
    PTR_DATA,
    DATA_PTR
};

class bnode : public node
{
private:
    // 所有的bnode共有一块临时内存，大小=页大小+最大类型大小
    // 于IndexManager构造时申请，于其析构时释放
    // 用于存储临时超过节点限制的B+树待分裂节点
    static void *tmpMemory;
    // 0-leaf，1-inode
    // (base+1)-root
    char *base;
    word *parent;
    int *cnt;
    // 计算相应index的key左侧6字节指针的首地址
    inline int index2offset(int index, int type) { return 10 + (type2size(type) + 6) * index; };
    // 使用时保证*cnt > 0
    // 默认返回第一个大于等于e的元素的index
    // lFlag表示less flag，指定为true后将返回第一个大于e的元素的index
    // 不存在则返回*cnt
    int binarySearch(const element &e, const bool lFlag = false);
    // 按照packingType将B+树节点内部的key与指针打包，每个包的index取决于包内key的index
    // 然后move函数执行将第start个包移动dir(可正可负)个包的位置
    // 返回被移动的start包的首地址
    void *move(int start, int dir, int type, const packing packingType = PTR_DATA);
    // void *rmove(int start, int dir, int type);

public:
    // 允许友元类IndexManager直接操作节点内的私有成员
    friend class IndexManager;
    // 构造函数，一个B+树节点由一个Buffer块构造而来
    // 构造为B+树节点的Buffer块将PIN在内存中，不会被替换出去
    bnode(const node &src);
    // 用于保证拷贝构造时Buffer的PIN计数的一致性
    bnode(const bnode &src);
    // 析构，UNPIN构造本节点的块
    ~bnode();
    // 获取是否是叶节点
    inline int getBase() { return *base; }
    // 获取节点内key的数量
    inline int getCnt() { return *cnt; }
    // 获取父亲节点的块地址
    inline word getParent() { return *parent; }
    // 修改父亲节点
    inline void setParent(word blockAddr) { *parent = blockAddr; }
    // 将当前节点内的第idx个key的指针打包成element
    // 若块内发生move或者其他改变节点内容的操作，继续使用element可能导致不一致状态
    inline element getElement(int idx, int type) { return element(type, base + index2offset(idx, type) + 6); }
    // 本find函数用于获取节点内部的指针
    // 若lFlag为true，找到第一个大于key的元素；若为false，找到第一个大于等于key的元素
    // 根据equalFlag决定元素是否需要与key做相等的比较
    // 根据packingType返回元素包内指针解析成的地址，文件地址填充Index文件地址
    dword find(const element &key, const packing packingType = PTR_DATA, const bool equalFlag = false, const bool lFlag = false);
    // 返回插入位置的index，内部不做检查，需保证节点未满
    // 传入的地址中的块地址与偏移地址将解析为指针与key按packingType打包存储
    int insertKey(const element &key, const dword virtAddr, const packing packingType = PTR_DATA);
    // 删掉第一个大于等于key的元素与打包的指针
    // 返回删掉元素的index，否则返回FAILURE
    int deleteKey(const element &key, const packing packingType = PTR_DATA);
    // 查找元素key，若存在则用newKey替换它
    int replaceKey(const element &key, const element &newKey);
    // 分裂节点，返回产生的新节点，新节点中的元素大于当前节点
    bnode split(const element &key, const dword virtAddr);
    int splice(bnode &par, bnode &src, int type);
    // 返回第一个小于父节点中应被删除索引的元素
    element merge(bnode &par, bnode &src, int type);
    // 于硬盘上删除bnode对应的块
    void deleteBlock() { origin.deleteBlock(getFileAddr(), getBlockAddr()); }
};

class IndexManager
{
private:
    hword indexFileAddr;
    // 返回可能存在key的叶节点块地址
    word find(const word curAddr, const word parentAddr, const element &keyValue);
    // 如果更新了根节点返回根节点的blockAddr，否则返回0
    word insertInParent(bnode &cur, const element &key, const word addr);
    word erase(bnode &cur, const element &keyValue, const packing delType);
    void drop(word curAddr, const int type);

public:
    int bcnt[257];
    int bInMincnt[257];
    int bLeafMincnt[257];
    IndexManager();
    ~IndexManager();
    // 设置本次运行中INDEX文件的文件地址
    void setIndexFileAddr(hword fileAddr) { indexFileAddr = fileAddr; }
    // 返回记录的完整地址，若未找到返回0
    dword findAddrEntry(const hword dataFileAddr, const word rootAddr, const element &keyValue);
    // 若找到记录，将其解析成node类，若未找到返回无效node
    node findRecordEntry(const hword dataFileAddr, const word rootAddr, const element &keyValue);
    // 插入一组键值与记录的地址，根地址由本函数更新
    int insertEntry(attribute &attr, const element &keyValue, dword virtAddr);
    // 传入一个filter，删除filter中删除记录对应的索引信息
    // 根地址由本函数更新
    int deleteEntry(std::vector<attribute> &col, const filter &flt);
    // 在对应的属性上创建索引
    int createEntry(const hword dataFileAddr, std::vector<attribute> &col, int pos);
    // 删除对应属性上的索引
    int dropEntry(std::vector<attribute> &col, int pos);
};

extern IndexManager idxMgr;