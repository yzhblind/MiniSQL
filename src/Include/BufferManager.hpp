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
        //recordSize stores root block address if file type is INDEX
        std::vector<int> blockNum, recordSize, nextFree;
        std::vector<bool> valid;
        std::list<hword> freelist;
    };

    buffer buf;
    fileIndex file;
    static long long bufferSize;
    static const long long pageSize;
    int resize();
    int writeBlock2Buffer(buffer &buf, fileIndex &file, const int bufID);
    int movBlock2Buffer(const hword fileAddr, const word blockAddr);

public:
    BufferManager();
    ~BufferManager();
    int openFile(const std::string filename, const fileType type, const int recordSize = 0);
    int removeFile(const hword fileAddr);
    node getNextFree(const hword fileAddr);
    node getRootBlock(const hword fileAddr);
    node setRootBlock(const hword fileAddr, const word blockAddr);
    node getBlock(const hword fileAddr, const word blockAddr);
    int setDirty(const hword fileAddr, const word blockAddr);
    int deleteBlock(const hword fileAddr, const word blockAddr);
    int pinBlock(const hword fileAddr, const word blockAddr, const pinType type);
    int deleteRecord(const hword fileAddr, const word blockAddr, const hword offset);
    bool checkNodeValid(const node &x) const;
};

class node
{
private:
    BufferManager &origin;
    dword virtAddr;
    void *phyAddr;
    int bufID, size;

public:
    node(BufferManager &origin, dword virtAddr, void *phyAddr, int bufferID, int size) : origin(origin), virtAddr(virtAddr), phyAddr(phyAddr), size(size){};
    inline hword getFileAddr() const { return virtAddr >> 48; }
    inline hword getOffset() const { return virtAddr & 0xFFFF; }
    inline word getBlockAddr() const { return (virtAddr >> 16) & 0xFFFFFFFF; }
    inline dword getVirtAddr() const { return virtAddr; }
    inline int getSize() const { return size; }
    inline int getBufferID() const { return bufID; }
    inline bool isValid() const { return origin.checkNodeValid(*this); }
    int read(void *const dest, const int offset, const int byteSize) const;
    int write(const void *const src, const int offset, const int byteSize);
};

extern BufferManager bufMgr;