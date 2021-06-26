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
                curw = std::max(INT_WIDTH, (int)origin[i].attrName.size() + 1);
                out << std::setw(curw) << *((int *)(res[i] + offset[j]));
                break;
            case 1:
                curw = std::max(FLOAT_WIDTH, (int)origin[i].attrName.size() + 1);
                out << std::setw(curw) << *((float *)(res[i] + offset[j]));
                break;
            default:
                curw = std::max(CHAR_WIDTH, (int)origin[i].attrName.size() + 1);
                curw = std::max(curw, origin[i].type + 1);
                out << std::setw(curw) << *((char *)(res[i] + offset[j]));
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
    if (ctgMgr.addSchema(news.tableName, news.column, news.primaryKey))
        std::cout << "Error: COULD NOT CREATE TABLE" << std::endl;
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
    if (ctgMgr.dropSchema(tableName))
        std::cout << "Error: COULD NOT DROP TABLE" << std::endl;
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
            API_select_index(tableName, condition, i);
            return;
        }
        curFilter.addCond(condition[i]);
    }
    rcdMgr.selectRecord(ctgMgr.getFileAddr(tableName), curFilter);
    curFilter.output(std::cout);
}

void API_select_index(std::string &tableName, std::vector<condExpr> &condition, int pos)
{
    std::vector<attribute> &col = ctgMgr.getColumn(tableName);
    int colpos = condition[pos].pos;
    int curRootAddr = col[colpos].indexRootAddr;
    element curElement(col[colpos].type, (void *)condition[pos].val);
    dword tupleAddr = idxMgr.findAddrEntry(ctgMgr.getFileAddr(tableName), ctgMgr.getColumn(tableName)[condition[pos].pos].indexRootAddr, curElement);

    filter curFilter(col);
    int condsz = condition.size();
    for (int i = 0; i < condsz; i++)
        curFilter.addCond(condition[i]);
    rcdMgr.getRecord(tupleAddr, curFilter);
    curFilter.output(std::cout);
}

void SQL_insert(std::string tableName, std::vector<element> list)
{
    std::vector<attribute> &col = ctgMgr.getColumn(tableName);
    int fileAddr = ctgMgr.getFileAddr(tableName);
    int sz = col.size();
    int sizeTotal = 0;
    for (int i = 0; i < sz; i++)
    {
        if (col[i].type != list[i].type)
        {
            std::cout << "!!! API ERROR IN __INSERT__" << std::endl;
            return;
        }
        if (col[i].indexRootAddr != 0)
        {
            idxMgr.insertEntry(col[i], list[i], col[i].indexRootAddr);
        }
        sizeTotal += type2size(col[i].type);
    }
    void *data = malloc(sizeTotal);
    sizeTotal = 0;
    for (int i = 0; i < sz; i++)
    {
        switch (col[i].type)
        {
        case 0:
            *((int *)(data + sizeTotal)) = *((int *)list[i].ptr);
            break;
        case 1:
            *((float *)(data + sizeTotal)) = *((float *)list[i].ptr);
            break;
        default:
            for(int j = 0; j < col[i].type - 1; j++)
            {
                *((char *)(data + sizeTotal + j)) = *((char *)(list[i].ptr + j));
            }
            *((char *)(data + sizeTotal + col[i].type - 1)) = '\0';
            break;
        }
        sizeTotal += type2size(col[i].type);
    }
    rcdMgr.insertRecord(fileAddr, data, col);
    free(data);
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