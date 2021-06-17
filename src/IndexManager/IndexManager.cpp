#include "IndexManager.hpp"

bnode::bnode(const node &src) : node(src)
{
    origin.pinBlock(getFileAddr(), getBlockAddr(), PIN);
    //TODO
}
bnode::~bnode()
{
    //TODO
    origin.pinBlock(getFileAddr(), getBlockAddr(), UNPIN);
}

IndexManager idxMgr;