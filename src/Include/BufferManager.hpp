#pragma once

#include <list>
#include <cstdio>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <unordered_map>

#define SUCCESS 0
#define FAILURE -1

typedef unsigned short int hword;
typedef unsigned int word;
typedef unsigned long long dword;

enum fileType
{
    DATA,
    INDEX,
    CATALOG
};

enum pinType
{
    PIN,
    UNPIN
};

class node;

class BufferManager
{
private:
    struct buffer
    {
        std::vector<bool> valid;
        std::vector<bool> dirty;
        std::vector<int> pinned;
        std::vector<dword> tag;
        std::unordered_map<dword, int> tagIndex;
        std::vector<void *> pool;
        std::vector<std::list<int>::iterator> LRUIndex;
        std::list<int> LRU, fixed;
    };
    struct fileIndex
    {
        std::vector<FILE *> pointer;
        std::vector<fileType> type;
        //对INDEX文件，recordSize将存储其根节点块地址
        std::vector<int> blockNum, recordSize, nextFree;
        std::vector<bool> valid;
        std::list<hword> freelist;
    };

    buffer buf;
    fileIndex file;
    static long long bufferSize;
    inline bool notValidAddr(const hword fileAddr) const { return fileAddr >= file.pointer.size() || file.valid[fileAddr] == false; }
    inline bool notValidAddr(const hword fileAddr, const word blockAddr) const { return blockAddr == 0 || blockAddr >= file.blockNum[fileAddr]; }
    void bufferAdjust();
    int resize();
    int newDataBlock(const hword fileAddr);
    int writeMetaData(const hword fileAddr);
    int writeBlock2File(const int bufID);
    int movBlock2Buffer(const hword fileAddr, const word blockAddr);

public:
    static const long long pageSize;
    BufferManager();
    ~BufferManager();
    inline int getBlockNumber(const hword fileAddr) const { if (notValidAddr(fileAddr)) return FAILURE; else return file.blockNum[fileAddr]; }
    inline int getRecordSize (const hword fileAddr) const { if (notValidAddr(fileAddr)) return FAILURE; else return file.recordSize[fileAddr] - 1; }
    //按照filename打开文件，若成功返回打开文件的fileAddr，失败则返回FAILURE
    //若文件类型为DATA，则recordSize最小为2字节，其他类型的文件请让recordSize保持默认
    //本函数的recordSize参数表示纯记录大小，DATA文件中将在记录首部追加一字节的valid位
    int openFile(const std::string filename, const fileType type, const int recordSize = 0);
    //关闭并删除文件
    //请务必保证fileAddr与filename的一致性，否则行为将未定义
    int removeFile(const hword fileAddr, const std::string filename);
    //用于获取可用的写接口
    //DATA文件，获取一个不包括valid byte的记录空间node
    //INDEX与CATALOG文件，获取一个空余的block的node
    node getNextFree(const hword fileAddr);
    //弃用，根节点信息由CatalogManager组织
    node getRootBlock(const hword fileAddr);
    //弃用，根节点信息由CatalogManager组织
    int setRootBlock(const hword fileAddr, const word blockAddr);
    //获取地址指向的block块的node，若文件类型为DATA，将去除块首的6字节metadata
    //注意DATA块中每个块首均有1字节的valid标志
    node getBlock(const hword fileAddr, const word blockAddr);
    //若地址指向的块在buffer中则将其设为dirty，否则返回FAILURE
    int setDirty(const hword fileAddr, const word blockAddr);
    //仅面向INDEX文件类型，释放地址指向的块
    int deleteBlock(const hword fileAddr, const word blockAddr);
    //可以固定块到buffer或解除固定，根据pinType给pin指数+1/-1，若pin指数大于0则被固定
    int pinBlock(const hword fileAddr, const word blockAddr, const pinType type);
    //获取指向地址的record的node，offset会自动向前寻找首地址
    node getRecord(const hword fileAddr, const word blockAddr, const hword offset);
    //删除指向地址的record，offset会自动向前寻找首地址
    int deleteRecord(const hword fileAddr, const word blockAddr, const hword offset);
    //检查node指向的空间是否仍有效
    bool checkNodeValid(const node &x) const;
};
//node类用于实现读写封装与越界检查
//不提供一致性保证，若获取类后进行了其他Buffer操作，请调用isValid()函数检查是否有效
//若类失效，请使用BufferManager函数重新获取
class node
{
private:
    BufferManager &origin;
    dword virtAddr;
    void *phyAddr;
    int bufID, size;

public:
    node(BufferManager &origin, dword virtAddr = 0, void *phyAddr = NULL, int bufferID = -1, int size = 0) : origin(origin), virtAddr(virtAddr), phyAddr(phyAddr), size(size){};
    //一组get函数，用于获取读写对象的元数据信息
    inline hword getFileAddr() const { return virtAddr >> 48; }
    inline hword getOffset() const { return virtAddr & 0xFFFF; }
    inline word getBlockAddr() const { return (virtAddr >> 16) & 0xFFFFFFFF; }
    inline dword getVirtAddr() const { return virtAddr; }
    inline int getSize() const { return size; }
    inline int getBufferID() const { return bufID; }
    //检查当前类的有效性
    inline bool isValid() const { return origin.checkNodeValid(*this); }
    //读写接口，第一个参数为要读或写的外部源，第二个参数为内部首地址偏移量，第三个参数为读写总字节数
    //若检查到读写越界，直接返回FAILURE，不进行其他任何操作
    int read(void *const dest, const int offset, const int byteSize) const;
    int write(const void *const src, const int offset, const int byteSize);
};

extern BufferManager bufMgr;