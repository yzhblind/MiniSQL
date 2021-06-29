#include "CatalogManager.hpp"

#include <sstream>
#include <iostream>

int CatalogManager::startCata()
{
    cataFileAddr = bufMgr.openFile("CATALOG", CATALOG);
    int indexFileAddr = bufMgr.openFile("INDEX", INDEX);
    idxMgr.setIndexFileAddr(indexFileAddr);
    char *readBuff;
    int schemaNum = 0;
    int blockNum;
    std::stringstream ss;

    node firstBlock = bufMgr.getBlock(cataFileAddr, 1);
    blockNum = bufMgr.getBlockNumber(cataFileAddr);

    readBuff = (char *)malloc(sizeof(char) * BufferManager::pageSize);
    if (readBuff == NULL)
        return FAILURE;
    memset(readBuff, 0, sizeof(readBuff));
    

    firstBlock.read(readBuff, 0, BufferManager::pageSize);
    ss.write(readBuff, BufferManager::pageSize);

    ss >> schemaNum;
    ss.str("");
    ss.clear();
    // 一次性全部拿进来
    for (int i = 2; i < blockNum; i++)
    {
        node schemaBlock = bufMgr.getBlock(cataFileAddr, i);
        schemaBlock.read(readBuff, 0, BufferManager::pageSize);
        ss.write(readBuff, BufferManager::pageSize);
    }
    //ss.clear();

    for (int i = 0; i < schemaNum; i++)
    {
        schema tmpSchema;
        attribute tmpAttr;
        int columnNum;
        ss >> tmpSchema.tableName;
        ss >> columnNum;
        ss >> tmpSchema.primaryKey;
        ss >> tmpSchema.recordSize;

        ctgMgr.schemaIndex[tmpSchema.tableName] = i;

        for (int j = 0; j < columnNum; j++)
        {
            ss >> tmpAttr.attrName;
            ss >> tmpAttr.type;
            ss >> tmpAttr.isUnique;
            ss >> tmpAttr.indexRootAddr;
            if (tmpAttr.indexRootAddr != 0 && j != tmpSchema.primaryKey)
            {
                std::string indexName;
                ss >> indexName;
                ctgMgr.attrIndex[tmpAttr.attrName] = indexName;
                ctgMgr.indexAttr[indexName] = tmpAttr.attrName;
                ctgMgr.indexSchema[indexName] = tmpSchema.tableName;
            }
            tmpSchema.column.push_back(tmpAttr);
            tmpSchema.attributeIndex[tmpAttr.attrName] = j;
        }
        //tmpSchema.fileAddr = bufMgr.openFile(tmpSchema.tableName, DATA, tmpSchema.recordSize);
        ctgMgr.schemas.push_back(tmpSchema);
        ctgMgr.schemas[ctgMgr.schemas.size() - 1].fileAddr = bufMgr.openFile(tmpSchema.tableName, DATA, tmpSchema.recordSize);
        ctgMgr.schemaIndex[tmpSchema.tableName] = i;
    }
    ss.str("");
    ss.clear();
    free(readBuff);
    return SUCCESS;
}

/*
std::string tableName;
std::map<std::string, int> attributeIndex;
std::vector<attribute> column;
int fileAddr;
int primaryKey;
int recordSize;
*/

int CatalogManager::endCata()
{
    std::stringstream ss;
    char *writeBuff = (char *)malloc(sizeof(char) * BufferManager::pageSize);
    if (writeBuff == NULL)
        return FAILURE;
    memset(writeBuff, 0, sizeof(writeBuff));
    int schemaNum = ctgMgr.schemas.size();
    int blockNum = bufMgr.getBlockNumber(cataFileAddr);
    node firstBlock = bufMgr.getBlock(cataFileAddr, 1);

    ss << schemaNum;
    ss << " ";
    ss.read(writeBuff, BufferManager::pageSize);
    ss.clear();
    firstBlock.write(writeBuff, 0, BufferManager::pageSize);

    for (int i = 0; i < schemaNum; i++)
    {

        ss << ctgMgr.schemas[i].tableName;
        ss << " ";
        ss << ctgMgr.schemas[i].column.size();
        ss << " ";
        ss << ctgMgr.schemas[i].primaryKey;
        ss << " ";
        ss << ctgMgr.schemas[i].recordSize;
        ss << " ";
        int columnNum = ctgMgr.schemas[i].column.size();
        for (int j = 0; j < columnNum; j++)
        {
            ss << ctgMgr.schemas[i].column[j].attrName;
            ss << " ";
            ss << ctgMgr.schemas[i].column[j].type;
            ss << " ";
            ss << ctgMgr.schemas[i].column[j].isUnique;
            ss << " ";
            ss << ctgMgr.schemas[i].column[j].indexRootAddr;
            ss << " ";
            if (ctgMgr.schemas[i].column[j].indexRootAddr != 0 && j != ctgMgr.schemas[i].primaryKey)
            {
                ss << ctgMgr.attrIndex[ctgMgr.schemas[i].column[j].attrName];
                ss << " ";
            }
        }
    }
    int blockCNT = 2;
    while (!ss.eof())
    {
        ss.read(writeBuff, BufferManager::pageSize);
        node schemaBlock = blockCNT < blockNum ? bufMgr.getBlock(cataFileAddr, blockCNT) : bufMgr.getNextFree(cataFileAddr);
        schemaBlock.write(writeBuff, 0, BufferManager::pageSize);
        blockCNT++;
    }
    free(writeBuff);
    return SUCCESS;
}

int CatalogManager::addSchema(std::string tableName, std::vector<attribute> &column, int primaryKey)
{
    int recordSize = 0;
    schema tmpSchema;
    tmpSchema.tableName = tableName;
    tmpSchema.column = column;
    tmpSchema.primaryKey = primaryKey;
    tmpSchema.column[tmpSchema.primaryKey].isUnique = true;

    for (auto &it : column)
        recordSize += type2size(it.type);
    tmpSchema.recordSize = recordSize;
    tmpSchema.fileAddr = bufMgr.openFile(tableName, DATA, recordSize);

    ctgMgr.schemas.push_back(tmpSchema);
    ctgMgr.schemaIndex[tableName] = ctgMgr.schemas.size() - 1;

    int colsz = column.size();
    int schsz = ctgMgr.schemas.size();
    for (int i = 0; i < colsz; i++)
    {
        ctgMgr.schemas[schsz - 1].attributeIndex[column[i].attrName] = i;
    }

    return SUCCESS;
}

int CatalogManager::dropSchema(std::string &tableName)
{
    if (ctgMgr.schemaIndex.find(tableName) == ctgMgr.schemaIndex.end())
        return FAILURE;
    ctgMgr.schemas.erase(ctgMgr.schemas.begin() + ctgMgr.schemaIndex[tableName]);
    ctgMgr.schemaIndex.erase(tableName);
    return SUCCESS;
}

std::vector<attribute> &CatalogManager::getColumn(std::string &scheName)
{
    return ctgMgr.schemas[ctgMgr.schemaIndex[scheName]].column;
}

bool CatalogManager::findSchema(std::string &tableName)
{
    return ctgMgr.schemaIndex.find(tableName) != ctgMgr.schemaIndex.end();
}

bool CatalogManager::findSchemaColumn(std::string &tableName, std::string &attrName)
{
    if (!findSchema(tableName))
        return false;
    return ctgMgr.schemas[ctgMgr.schemaIndex[tableName]].attributeIndex.find(attrName) != ctgMgr.schemas[ctgMgr.schemaIndex[tableName]].attributeIndex.end();
}

int CatalogManager::getColumnAddr(std::string &tableName, std::string &attrName)
{
    return ctgMgr.schemas[ctgMgr.schemaIndex[tableName]].attributeIndex[attrName];
}

int CatalogManager::getFileAddr(std::string &scheName)
{
    return ctgMgr.schemas[ctgMgr.schemaIndex[scheName]].fileAddr;
}

bool CatalogManager::findIndex(std::string &indexName)
{
    return ctgMgr.indexAttr.find(indexName) != ctgMgr.indexAttr.end() &&
           ctgMgr.indexSchema.find(indexName) != ctgMgr.indexSchema.end();
}

int CatalogManager::addIndex(std::string &indexName, std::string &tableName, std::string &attrName)
{
    if (findIndex(indexName))
        return FAILURE;
    ctgMgr.indexSchema[indexName] = tableName;
    ctgMgr.indexAttr[indexName] = attrName;
    ctgMgr.attrIndex[attrName] = indexName;
    return SUCCESS;
}

int CatalogManager::dropIndex(std::string &indexName)
{
    if (!findIndex(indexName))
        return FAILURE;
    ctgMgr.attrIndex.erase(ctgMgr.indexAttr[indexName]);
    ctgMgr.indexSchema.erase(indexName);
    ctgMgr.indexAttr.erase(indexName);
    return SUCCESS;
}

std::string CatalogManager::getIndexSchemaName(std::string &indexName)
{
    return ctgMgr.indexSchema[indexName];
}

std::string CatalogManager::getIndexAttrName(std::string &indexName)
{
    return ctgMgr.indexSchema[indexName];
}

int CatalogManager::getRecordSize(std::string& tableName)
{
    return ctgMgr.schemas[ctgMgr.schemaIndex[tableName]].recordSize;
}

std::string CatalogManager::getIndexName(std::string &attrName)
{
    return ctgMgr.attrIndex[attrName];
}

CatalogManager ctgMgr;