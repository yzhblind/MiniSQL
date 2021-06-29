#pragma once
#include "BufferManager.hpp"
#include "IndexManager.hpp"
#include "Type.hpp"
class CatalogManager
{
private:
    int cataFileAddr;
    std::vector<schema> schemas;
    std::map<std::string, int> schemaIndex;
    std::map<std::string, std::string> indexSchema;
    std::map<std::string, std::string> indexAttr;
    std::map<std::string, std::string> attrIndex;

public:
    //CatalogManager();
    //~CatalogManager();
    // 启动和退出
    int startCata();
    int endCata();

    // table 相关
    int addSchema(std::string tableName, std::vector<attribute> &column, int primaryKey);
    int dropSchema(std::string &tableName);
    std::vector<attribute> &getColumn(std::string &scheName);
    bool findSchema(std::string &tableName);
    bool findSchemaColumn(std::string &tableName, std::string &attrName);
    int getRecordSize(std::string &tableName);
    int getColumnAddr(std::string &tableName, std::string &attrName);

    // 获取对应记录文件的地址
    int getFileAddr(std::string &tableName);

    // index 相关操作
    bool findIndex(std::string &indexName);
    int addIndex(std::string &indexName, std::string &tableName, std::string &attrName);
    int dropIndex(std::string &indexName);
    std::string getIndexSchemaName(std::string &indexName);
    std::string getIndexAttrName(std::string &indexName);
    std::string getIndexName(std::string &attrName);
};

extern CatalogManager ctgMgr;