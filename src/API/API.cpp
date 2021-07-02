#include "API.hpp"
#include <iomanip>
#include <iostream>
#include <algorithm>

#define INT_WIDTH 9
#define CHAR_WIDTH 10
#define FLOAT_WIDTH 8

void filter::output(std::ostream &out)
{
    out.setf(std::ios::left);
    int sz = origin.size();

    for (int i = 0; i < sz; i++)
    {
        int curw = 0;
        switch (origin[i].type)
        {
        case 0:
            curw = std::max(INT_WIDTH, (int)origin[i].attrName.size() + 1);
            break;
        case 1:
            curw = std::max(FLOAT_WIDTH, (int)origin[i].attrName.size() + 1);
            break;
        default:
            curw = std::max(CHAR_WIDTH, (int)origin[i].attrName.size() + 1);
            curw = std::max(curw, origin[i].type + 1);
            break;
        }
        out << std::setw(curw) << origin[i].attrName;
    }

    out << std::endl;

    int resz = res.size();

    if (resz == 0)
        out << "No satisfying tuple" << std::endl;

    for (int i = 0; i < resz; i++)
    {
        for (int j = 0; j < sz; j++)
        {
            int curw = 0;
            switch (origin[j].type)
            {
            case 0:
                curw = std::max(INT_WIDTH, (int)origin[j].attrName.size() + 1);
                out << std::setw(curw) << *((int *)((char *)res[i] + offset[j]));
                break;
            case 1:
                curw = std::max(FLOAT_WIDTH, (int)origin[j].attrName.size() + 1);
                out << std::setw(curw) << *((float *)((char *)res[i] + offset[j]));
                break;
            default:
                curw = std::max(CHAR_WIDTH, (int)origin[j].attrName.size() + 1);
                curw = std::max(curw, origin[j].type + 1);
                out << std::setw(curw) << (char *)((char *)res[i] + offset[j]);
            }
        }
        out << std::endl;
    }
}

bool API_check_schema(std::string &tableName)
{
    return ctgMgr.findSchema(tableName);
}

void SQL_create_table(schema &news)
{
    int recordSize = 0;
    for (auto &it : news.column)
        recordSize += type2size(it.type);
    if(recordSize > bufMgr.pageSize)
    {
        std::cout << "Error: the record size is out of range: " << bufMgr.pageSize << " bytes" << std::endl;
        return ; 
    }
    if (ctgMgr.addSchema(news.tableName, news.column, news.primaryKey))
        std::cout << "Error: COULD NOT CREATE TABLE" << std::endl;
    std::vector<attribute> &col = ctgMgr.getColumn(news.tableName);
    idxMgr.createEntry(ctgMgr.getFileAddr(news.tableName), col, news.primaryKey);
    std::cout << "crt table " << news.tableName << std::endl; //测试用输出语句
}

void SQL_create_index(std::string &indexName, std::string &tableName, std::string &attrName)
{
    if (idxMgr.createEntry(ctgMgr.getFileAddr(tableName), ctgMgr.getColumn(tableName), ctgMgr.getColumnAddr(tableName, attrName)))
        std::cout << "Error: COULD NOT CREATE INDEX" << std::endl;
    if (ctgMgr.addIndex(indexName, tableName, attrName))
        std::cout << "Error: COULD NOT CREATE INDEX" << std::endl;
}

void SQL_drop_table(std::string &tableName)
{
    // map erase
    int fileAddr = ctgMgr.getFileAddr(tableName);
    std::vector<attribute> &col = ctgMgr.getColumn(tableName);
    int sz = col.size();
    for (int i = 0; i < sz; i++)
    {
        if (col[i].indexRootAddr != 0)
        {
            std::string indexName = ctgMgr.getIndexName(col[i].attrName);
            ctgMgr.dropIndex(indexName);
            idxMgr.dropEntry(col, i);
        }
    }
    bufMgr.removeFile(fileAddr, tableName);
    if (ctgMgr.dropSchema(tableName))
        std::cout << "Error: COULD NOT DROP TABLE" << std::endl;
    // !!! __ BIG PROBLEM __
}

void SQL_drop_index(std::string &indexName)
{
    std::string tableName = ctgMgr.getIndexSchemaName(indexName);
    std::string attrName = ctgMgr.getIndexAttrName(indexName);
    std::vector<attribute> &tmp = ctgMgr.getColumn(tableName);
    int pos = ctgMgr.getColumnAddr(tableName, attrName);
    if (idxMgr.dropEntry(tmp, pos))
        std::cout << "Error: COULD NOT DROP INDEX" << std::endl;
    if (ctgMgr.dropIndex(indexName))
        std::cout << "Error: COULD NOT DROP INDEX" << std::endl;
}

void SQL_select_all(std::string &tableName)
{
    hword curAddr = ctgMgr.getFileAddr(tableName);
    std::vector<attribute> &origin = ctgMgr.getColumn(tableName);
    filter curFilter(origin);
    rcdMgr.selectRecord(curAddr, curFilter);
    curFilter.output(std::cout);
}

void SQL_select_cond(std::string &tableName, std::vector<condExpr> &condition)
{
    int condsz = condition.size();
    std::vector<attribute> &col = ctgMgr.getColumn(tableName);
    filter curFilter(col);
    for (int i = 0; i < condsz; i++)
    {
        condExpr tmp = condition[i];
        if (tmp.origin[tmp.pos].indexRootAddr)
        {
            API_select_on_index(tableName, condition, i);
            return;
        }
        curFilter.addCond(condition[i]);
    }
    rcdMgr.selectRecord(ctgMgr.getFileAddr(tableName), curFilter);
    curFilter.output(std::cout);
}

void API_select_on_index(std::string &tableName, std::vector<condExpr> &condition, int &pos)
{
    std::vector<attribute> &col = ctgMgr.getColumn(tableName);
    int colpos = condition[pos].pos;
    int curRootAddr = col[colpos].indexRootAddr;
    element curElement(col[colpos].type, (void *)condition[pos].val);
    dword tupleAddr = idxMgr.findAddrEntry(ctgMgr.getFileAddr(tableName), col[colpos].indexRootAddr, curElement);

    filter curFilter(col);
    int condsz = condition.size();
    for (int i = 0; i < condsz; i++)
        curFilter.addCond(condition[i]);
    rcdMgr.getRecord(tupleAddr, curFilter);
    curFilter.output(std::cout);
}

void SQL_insert(std::string &tableName, void *data)
{
    std::vector<attribute> &col = ctgMgr.getColumn(tableName);
    int fileAddr = ctgMgr.getFileAddr(tableName);
    int sz = col.size();
    int curSize = 0;
    for (int i = 0; i < sz; i++)
    {
        if (col[i].indexRootAddr != 0)
        {
            element curElement(col[i].type, (char *)data + curSize);
            dword anoAddr = idxMgr.findAddrEntry(fileAddr, col[i].indexRootAddr, curElement);
            if (anoAddr != 0)
            {
                std::cout << "Error: the tuple does not satisfy the index unique restriction" << std::endl;
                return;
            }
        }
        curSize += type2size(col[i].type);
    }

    dword curAddr = rcdMgr.insertRecord(fileAddr, data, col);
    curSize = 0;
    for (int i = 0; i < sz; i++)
    {
        if (col[i].indexRootAddr != 0)
        {
            element curElement(col[i].type, (char *)data + curSize);
            idxMgr.insertEntry(col[i], curElement, curAddr);
        }
        curSize += type2size(col[i].type);
    }

    // int
}

void SQL_delete_all(std::string &tableName)
{
    std::vector<attribute> &col = ctgMgr.getColumn(tableName);
    filter curFilter(col);
    int fileAddr = ctgMgr.getFileAddr(tableName);
    rcdMgr.deleteRecord(fileAddr, curFilter);
    idxMgr.deleteEntry(col, curFilter);
    curFilter.output(std::cout);
}

void SQL_delete_cond(std::string &tableName, std::vector<condExpr> &condition)
{
    int condsz = condition.size();
    std::vector<attribute> &col = ctgMgr.getColumn(tableName);
    filter curFilter(col);
    for (int i = 0; i < condsz; i++)
    {
        condExpr tmp = condition[i];
        if (tmp.origin[tmp.pos].indexRootAddr)
        {
            API_delete_on_index(tableName, condition, i);
            return;
        }
        curFilter.addCond(condition[i]);
    }
    rcdMgr.deleteRecord(ctgMgr.getFileAddr(tableName), curFilter);
    idxMgr.deleteEntry(col, curFilter);
    curFilter.output(std::cout);
}

void API_delete_on_index(std::string &tableName, std::vector<condExpr> &condition, int &pos)
{
    std::vector<attribute> &col = ctgMgr.getColumn(tableName);
    int colpos = condition[pos].pos;
    int curRootAddr = col[colpos].indexRootAddr;
    element curElement(col[colpos].type, (void *)condition[pos].val);
    dword tupleAddr = idxMgr.findAddrEntry(ctgMgr.getFileAddr(tableName), col[colpos].indexRootAddr, curElement);

    filter curFilter(col);
    int condsz = condition.size();
    for (int i = 0; i < condsz; i++)
        curFilter.addCond(condition[i]);
    rcdMgr.getRecord(tupleAddr, curFilter);
    if (!curFilter.res.empty())
    {
        curFilter.output(std::cout);
        idxMgr.deleteEntry(col, curFilter);
        rcdMgr.deleteRecord(tupleAddr);
    }
    else
    {
        std::cout << "No satisfying tuples" << std::endl;
    }
}

/*

int createEntry(const hword dataFileAddr, std::vector<attribute> &col, int pos);
    // 删除对应属性上的索引
int dropEntry(std::vector<attribute> &col, int pos);

*/