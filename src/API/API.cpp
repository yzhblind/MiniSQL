#include "API.hpp"

bool API_check_schema(std::string tableName)
{
    return ctgMgr.findSchema(tableName);
}

void SQL_create_table(schema news)
{
    if(ctgMgr.addSchema(news.tableName, news.column, news.primaryKey))
        std::cout << "Error: COULD NOT CREATE TABLE" << std::endl;
}

void SQL_create_index(std::string indexName, std::string tableName, std::string attrName)
{
    if(idxMgr.createEntry(ctgMgr.getFileAddr(tableName), ctgMgr.getColumn(tableName), ctgMgr.getColumnAddr(tableName, attrName)))
        std::cout << "Error: COULD NOT CREATE INDEX" << std::endl;
    if(ctgMgr.addIndex(indexName, tableName, attrName))
        std::cout << "Error: COULD NOT CREATE INDEX" << std::endl;
}

void SQL_drop_table(std::string tableName)
{
    if(ctgMgr.dropSchema(tableName))
        std::cout << "Error: COULD NOT DROP TABLE" << std::endl;
}

void SQL_drop_index(std::string indexName)
{
    std::string tableName = ctgMgr.getIndexSchemaName(indexName);
    std::string attrName = ctgMgr.getIndexAttrName(indexName);
    std::vector<attribute>& tmp = ctgMgr.getColumn(tableName);
    int pos = ctgMgr.getColumnAddr(tableName, attrName);
    if(idxMgr.dropEntry(tmp, pos))
        std::cout << "Error: COULD NOT DROP INDEX" << std::endl;
    if(ctgMgr.dropIndex(indexName))
        std::cout << "Error: COULD NOT DROP INDEX" << std::endl;
}

void SQL_select_all(std::string tableName)
{

}

void SQL_select_cond(std::string tableName, std::vector<condExpr> condition)
{

}

void SQL_insert(std::string tableName, std::vector<element> list)
{

}

void SQL_delete_all(std::string tableName)
{

}

void SQL_delete_cond(std::string tableName, std::vector<condExpr> condition)
{

}


/*

int createEntry(const hword dataFileAddr, std::vector<attribute> &col, int pos);
    // 删除对应属性上的索引
int dropEntry(std::vector<attribute> &col, int pos);

*/