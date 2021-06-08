#include "BufferManager.hpp"

static inline hword extractFileAddr(dword virtAddr) { return virtAddr >> 48; }
static inline hword extractOffset(dword virtAddr) { return virtAddr & 0xFFFF; }
static inline word extractBlockAddr(dword virtAddr) { return (virtAddr >> 16) & 0xFFFFFFFF; }

const int BufferManager::pageSize = 4096;
const int BufferManager::bufferSize = 4096;

int BufferManager::writeBlock2Buffer(buffer &buf, fileIndex &file, const int bufID)
{
    FILE *fp = file.pointer[extractFileAddr(buf.tag[bufID])];
    fseek(fp, extractBlockAddr(buf.tag[bufID]) * pageSize, SEEK_SET);
    fwrite(buf.pool[bufID], 4096, 1, fp);
}
int BufferManager::movBlock2Buffer(const hword fileAddr, const word blockAddr)
{
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
}
BufferManager::~BufferManager()
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

int BufferManager::node::read(void *const dest, const int offset, const int byteSize) const
{
    if (offset + byteSize > size || offset < 0 || byteSize <= 0)
        return -1;
    memcpy(dest, static_cast<char *>(phyAddr) + offset, byteSize);
    return 0;
}
int BufferManager::node::write(const void *const src, const int offset, const int byteSize)
{
    if (offset + byteSize > size || offset < 0 || byteSize <= 0)
        return -1;
    memcpy(static_cast<char *>(phyAddr) + offset, src, byteSize);
    return 0;
}

BufferManager bufMgr;
