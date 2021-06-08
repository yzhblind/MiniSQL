#pragma once

#include <list>
#include <cstdio>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
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
        int bufID, size;

    public:
        node(dword virtAddr, void *phyAddr, int bufferID, int size) : virtAddr(virtAddr), phyAddr(phyAddr), size(size){};
        inline hword getFileAddr() const { return virtAddr >> 48; }
        inline hword getOffset() const { return virtAddr & 0xFFFF; }
        inline word getBlockAddr() const { return (virtAddr >> 16) & 0xFFFFFFFF; }
        inline dword getVirtAddr() const { return virtAddr; }
        inline int getSize() const { return size; }
        inline int getBufferID() const { return bufID; }
        int read(void *const dest, const int offset, const int byteSize) const;
        int write(const void *const src, const int offset, const int byteSize);
    };

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
        std::vector<int> blockNum, recordSize, nextFree;
        std::vector<bool> fileValid;
        std::list<hword> freelist;
    };

    buffer buf;
    fileIndex file;
    int writeBlock2Buffer(buffer &buf, fileIndex &file, const int bufID);
    int movBlock2Buffer(const hword fileAddr, const word blockAddr);

public:
    static const int bufferSize;
    static const int pageSize;
    BufferManager();
    ~BufferManager();
    int init();
    int openFile(const std::string filename, const fileType type);
    int removeFile(const hword fileAddr);
    node getNextFree(const hword fileAddr);
    node getRootBlock(const hword fileAddr);
    node setRootBlock(const hword fileAddr, const word blockAddr);
    node getBlock(const hword fileAddr, const word blockAddr);
    int deleteBlock(const hword fileAddr, const word blockAddr);
    int pinBlock(const hword fileAddr, const word blockAddr, const pinType type);
    int deleteRecord(const hword fileAddr, const word blockAddr, const hword offset);
    bool checkNodeValid(const node &x) const;
};

extern BufferManager bufMgr;