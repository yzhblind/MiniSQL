#include "BufferManager.hpp"
#include "CatalogManager.hpp"

static inline hword extractFileAddr(dword virtAddr) { return virtAddr >> 48; }
static inline hword extractOffset(dword virtAddr) { return virtAddr & 0xFFFF; }
static inline word extractBlockAddr(dword virtAddr) { return (virtAddr >> 16) & 0xFFFFFFFF; }
static inline dword combileVirtAddr(hword fileAddr, word blockAddr, hword offset) { return (static_cast<dword>(fileAddr) << 48) | (static_cast<dword>(blockAddr) << 16) | static_cast<dword>(offset); }

const long long BufferManager::pageSize = 4096;
long long BufferManager::bufferSize = 1024;

int BufferManager::writeBlock2Buffer(buffer &buf, fileIndex &file, const int bufID)
{
    FILE *fp = file.pointer[extractFileAddr(buf.tag[bufID])];
    fseeko64(fp, extractBlockAddr(buf.tag[bufID]) * pageSize, SEEK_SET);
    return fwrite(buf.pool[bufID], 4096, 1, fp);
}
int BufferManager::movBlock2Buffer(const hword fileAddr, const word blockAddr)
{
    dword tag = combileVirtAddr(fileAddr, blockAddr, 0);
    auto iter = buf.tagIndex.find(tag);
    if (iter != buf.tagIndex.end())
    {
        int index = iter->second;
        if (buf.pinned[index] == 0)
            buf.LRU.splice(buf.LRU.begin(), buf.LRU, buf.LRUIndex[index]);
        return index;
    }
    else
    {
        if (buf.LRU.empty())
            resize();
        int index = *buf.LRU.rbegin();
        if (buf.valid[index] == true)
        {
            if (buf.dirty[index] == true)
                writeBlock2Buffer(buf, file, index);
            buf.tagIndex.erase(buf.tag[index]);
        }
        buf.valid[index] = true;
        buf.dirty[index] = false;
        buf.pinned[index] = 0;
        buf.tag[index] = tag;
        buf.tagIndex[tag] = index;
        FILE *fp = file.pointer[fileAddr];
        fseeko64(fp, blockAddr * pageSize, SEEK_SET);
        fread(buf.pool[index], 4096, 1, fp);
        buf.LRU.splice(buf.LRU.begin(), buf.LRU, buf.LRUIndex[index]);
        return index;
    }
}

BufferManager::BufferManager()
{
    buf.valid.resize(bufferSize, false);
    buf.dirty.resize(bufferSize, false);
    buf.pinned.resize(bufferSize, 0);
    buf.tag.resize(bufferSize, 0);
    buf.tagIndex.clear();
    for (int i = 0; i < bufferSize; ++i)
        buf.pool.push_back(new char[pageSize]);
    for (int i = 0; i < bufferSize; ++i)
    {
        buf.LRU.push_front(i);
        buf.LRUIndex.push_back(buf.LRU.begin());
    }
    buf.fixed.clear();

    file.pointer.clear();
    file.type.clear();
    file.blockNum.clear();
    file.recordSize.clear();
    file.nextFree.clear();
    file.valid.clear();
    file.freelist.clear();
}
BufferManager::~BufferManager()
{
}
int BufferManager::resize()
{
    bufferSize *= 2;
    buf.valid.resize(bufferSize, false);
    buf.dirty.resize(bufferSize, false);
    buf.pinned.resize(bufferSize, 0);
    buf.tag.resize(bufferSize, 0);
    for (int i = bufferSize / 2; i < bufferSize; ++i)
        buf.pool.push_back(new char[pageSize]);
    for (int i = bufferSize / 2; i < bufferSize; ++i)
    {
        buf.LRU.push_front(i);
        buf.LRUIndex.push_back(buf.LRU.begin());
    }
}
int BufferManager::openFile(const std::string filename, const fileType type)
{
    FILE *fp = fopen64(filename.c_str(), "rb+");
    if (fp == NULL)
        fp = fopen64(filename.c_str(), "wb+");
    if (fp == NULL)
        return -1;
    int fileAddr=file.pointer.size();
    file.pointer.push_back(fp);
    
}
int BufferManager::removeFile(const hword fileAddr)
{
}
node BufferManager::getNextFree(const hword fileAddr)
{
}
node BufferManager::getRootBlock(const hword fileAddr)
{
}
node BufferManager::setRootBlock(const hword fileAddr, const word blockAddr)
{
}
node BufferManager::getBlock(const hword fileAddr, const word blockAddr)
{
}
int BufferManager::setDirty(const hword fileAddr, const word blockAddr)
{
}
int BufferManager::deleteBlock(const hword fileAddr, const word blockAddr)
{
}
int BufferManager::pinBlock(const hword fileAddr, const word blockAddr, const pinType type)
{
}
int BufferManager::deleteRecord(const hword fileAddr, const word blockAddr, const hword offset)
{
}
bool BufferManager::checkNodeValid(const node &x) const
{
    if (x.getSize() <= 0)
        return false;
    if (buf.valid[x.getBufferID()] == true && buf.tag[x.getBufferID()] == x.getVirtAddr())
        return true;
    else
        return false;
}

int node::read(void *const dest, const int offset, const int byteSize) const
{
    if (offset + byteSize > size || offset < 0 || byteSize <= 0)
        return -1;
    memcpy(dest, static_cast<char *>(phyAddr) + offset, byteSize);
    return 0;
}
int node::write(const void *const src, const int offset, const int byteSize)
{
    if (offset + byteSize > size || offset < 0 || byteSize <= 0)
        return -1;
    origin.setDirty(extractFileAddr(virtAddr), extractBlockAddr(virtAddr));
    memcpy(static_cast<char *>(phyAddr) + offset, src, byteSize);
    return 0;
}

BufferManager bufMgr;