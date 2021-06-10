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
    if (fileAddr >= file.pointer.size())
        return FAILURE;
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
        if (blockAddr < file.blockNum[fileAddr])
            fread(buf.pool[index], 4096, 1, fp);
        else
        {
            file.blockNum[fileAddr] = blockAddr + 1;
            memset(buf.pool[index], 0, pageSize);
            fwrite(buf.pool[index], 4096, 1, fp);
        }
        buf.LRU.splice(buf.LRU.begin(), buf.LRU, buf.LRUIndex[index]);
        return index;
    }
}
void BufferManager::writeMetaData(const hword fileAddr)
{
    int index = movBlock2Buffer(fileAddr, 0);
    int *meta = static_cast<int *>(buf.pool[index]);
    meta[0] = file.blockNum[fileAddr];
    meta[1] = file.recordSize[fileAddr];
    meta[2] = file.nextFree[fileAddr];
    buf.dirty[index] = true;
}
void BufferManager::bufferAdjust()
{
    buf.valid.resize(bufferSize, false);
    buf.dirty.resize(bufferSize, false);
    buf.pinned.resize(bufferSize, 0);
    buf.tag.resize(bufferSize, 0);
}
BufferManager::BufferManager()
{
    bufferAdjust();
    for (int i = 0; i < bufferSize; ++i)
        buf.pool.push_back(new char[pageSize]);
    for (int i = 0; i < bufferSize; ++i)
    {
        buf.LRU.push_front(i);
        buf.LRUIndex.push_back(buf.LRU.begin());
    }
}
BufferManager::~BufferManager()
{
    for (int i = 0; i < file.pointer.size(); ++i)
        if (file.valid[i] == true)
            writeMetaData(i);
    //TODO
}
int BufferManager::resize()
{
    bufferSize *= 2;
    if (bufferSize >= 1024 * 1024)
        return FAILURE;
    bufferAdjust();
    for (int i = bufferSize / 2; i < bufferSize; ++i)
        buf.pool.push_back(new char[pageSize]);
    for (int i = bufferSize / 2; i < bufferSize; ++i)
    {
        buf.LRU.push_front(i);
        buf.LRUIndex.push_back(buf.LRU.begin());
    }
    return SUCCESS;
}
int BufferManager::openFile(const std::string filename, const fileType type, const int recordSize)
{
    bool newFileFlag = false;
    FILE *fp = fopen64(filename.c_str(), "rb+");
    if (fp == NULL)
    {
        fp = fopen64(filename.c_str(), "wb+");
        newFileFlag = true;
    }
    if (fp == NULL || (newFileFlag == true && recordSize == 0 && type == DATA))
        return FAILURE;
    int fileAddr;
    if (file.freelist.empty())
    {
        fileAddr = file.pointer.size();
        file.pointer.push_back(fp);
        file.type.push_back(type);
        file.valid.push_back(true);
        file.blockNum.push_back(newFileFlag ? 0 : 1);
        file.recordSize.push_back(recordSize + 1);
        file.nextFree.push_back(0);
    }
    else
    {
        fileAddr = *file.freelist.rbegin();
        file.freelist.pop_back();
        file.pointer[fileAddr] = fp;
        file.type[fileAddr] = type;
        file.valid[fileAddr] = true;
        file.blockNum[fileAddr] = newFileFlag ? 0 : 1;
        file.recordSize[fileAddr] = recordSize + 1;
        file.nextFree[fileAddr] = 0;
    }
    int index = movBlock2Buffer(fileAddr, 0);
    if (!newFileFlag)
    {
        int *meta = static_cast<int *>(buf.pool[index]);
        file.blockNum[fileAddr] = meta[0];
        file.recordSize[fileAddr] = meta[1];
        file.nextFree[fileAddr] = meta[2];
    }
}
int BufferManager::removeFile(const hword fileAddr)
{
    if (fileAddr >= file.pointer.size() || file.valid[fileAddr] == false)
        return FAILURE;
    writeMetaData(fileAddr);
    for (int i = 0; i < bufferSize; ++i)
        if (extractFileAddr(buf.tag[i]) == fileAddr && buf.valid[i] == true)
        {
            if (buf.dirty[i] == true)
                writeBlock2Buffer(buf, file, i);
            buf.valid[i] = false;
            if (buf.pinned[i] > 0)
                buf.LRU.splice(buf.LRU.end(), buf.fixed, buf.LRUIndex[i]);
            else
                buf.LRU.splice(buf.LRU.end(), buf.LRU, buf.LRUIndex[i]);
        }
    file.valid[fileAddr] = false;
    file.freelist.push_front(fileAddr);
}
node BufferManager::getNextFree(const hword fileAddr)
{
    //TODO
}
node BufferManager::getRootBlock(const hword fileAddr)
{
    //TODO
}
node BufferManager::setRootBlock(const hword fileAddr, const word blockAddr)
{
    //TODO
}
node BufferManager::getBlock(const hword fileAddr, const word blockAddr)
{
    //TODO
}
int BufferManager::setDirty(const hword fileAddr, const word blockAddr)
{
    //TODO
}
int BufferManager::deleteBlock(const hword fileAddr, const word blockAddr)
{
    //TODO
}
int BufferManager::pinBlock(const hword fileAddr, const word blockAddr, const pinType type)
{
    //TODO
}
int BufferManager::deleteRecord(const hword fileAddr, const word blockAddr, const hword offset)
{
    //TODO
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
        return FAILURE;
    memcpy(dest, static_cast<char *>(phyAddr) + offset, byteSize);
    return SUCCESS;
}
int node::write(const void *const src, const int offset, const int byteSize)
{
    if (offset + byteSize > size || offset < 0 || byteSize <= 0)
        return FAILURE;
    origin.setDirty(extractFileAddr(virtAddr), extractBlockAddr(virtAddr));
    memcpy(static_cast<char *>(phyAddr) + offset, src, byteSize);
    return SUCCESS;
}

BufferManager bufMgr;