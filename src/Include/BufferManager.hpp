#pragma once

#include <list>
#include <cstdio>
#include <vector>
#include <string>
#include <unordered_map>

typedef unsigned short int hword;
typedef unsigned int word;
typedef unsigned long long dword;

class BufferManager
{
public:
    enum fileType
    {
        DATA,
        OTHERS
    };
    enum pinType
    {
        PIN,
        UNPIN
    };
    class node
    {
    private:
        dword virtAddr;
        void *phyAddr;
        int size;

    public:
        node(dword virtAddr, void *phyAddr, int size);
        bool isValid() const;
        hword getFileAddr() const;
        hword getOffset() const;
        word getBlockAddr() const;
        dword getVirtAddr() const;
        int read(const void *const dest, const int offset, const int byteSize) const;
        int write(void *const src, const int offset, const int byteSize);
    };

private:
    struct buffer
    {
        std::vector<bool> bufferValid;
        std::vector<bool> bufferDirty;
        std::vector<int> bufferPinned;
        std::vector<dword> bufferTag;
        std::unordered_map<dword, int> tagIndex;
        std::vector<void *> bufferPool;
        std::list<int> LRU, fixed;
        std::vector<std::list<int>::iterator> LRUIndex;
    };
    struct fileIndex
    {
        std::vector<FILE *> pointer;
        std::vector<fileType> type;
        std::vector<int> blockNum, recordSize, nextFree;
    };

    buffer buf;
    fileIndex index;

    static const int bufferSize;
    static const int pageSize;

    int replaceBlock(const hword fileAddr, const word blockAddr);

public:
    static inline int getPageSize();
    static inline int getBufferSize();
    BufferManager();
    ~BufferManager();
    int init();
    int openFile(const std::string filename, const fileType type);
    node getNextFree(const hword fileAddr);
    node getRootBlock(const hword fileAddr);
    node setRootBlock(const hword fileAddr, const word blockAddr);
    node getBlock(const hword fileAddr, const word blockAddr);
    int deleteBlock(const hword fileAddr, const word blockAddr);
    int pinBlock(const hword fileAddr, const word blockAddr, const pinType type);
    int deleteRecord(const hword fileAddr, const word blockAddr, const hword offset);
};

extern BufferManager bufMgr;