// snaildb.cpp
#include "snaildb.h"

SnailDB::SnailDB() {}

SnailDB::~SnailDB()
{
    // Delete allocated memory for data
    for (size_t i = 0; i < numRows; ++i)
    {
        for (size_t j = 0; j < colNames.size(); ++j)
        {
            delete data[i][j];
        }
        data[i].clear();
    }
    // No need for delete[] for the outer vector
    data.clear(); // Alternatively, you can use data.resize(0) to clear the vector
}

void SnailDB::addStrColProp(const std::string &colName, size_t max_length)
{
    std::cout << "addStrCol: " << colName << " cols:" << colNames.size() << std::endl;
    colNames.push_back(colName);
    indexedColumns.push_back(false);
    columnInfo.push_back({colName, max_length, STR_TYPE});
}

void SnailDB::addIntColProp(const std::string &colName, size_t max_length)
{
    std::cout << "addIntCol: " << colName << " cols:" << colNames.size() << std::endl;
    colNames.push_back(colName);
    indexedColumns.push_back(false);
    columnInfo.push_back({colName, max_length, INT_TYPE});
}

std::string SnailDB::getRow() const
{
    std::string result;
    for (size_t i = 0; i < colNames.size(); ++i)
    {
        result += data[cursor][i]->getValue();
        if (i != colNames.size() - 1)
        {
            result += separator;
        }
    }
    return result;
}

SnailDataType *SnailDB::getProp(size_t col) const
{
    if (col < colNames.size())
    {
        return data[cursor][col];
    }
    return nullptr;
}

void SnailDB::next()
{
    if (cursor < numRows - 1)
    {
        ++cursor;
    }
}

void SnailDB::previous()
{
    if (cursor > 0)
    {
        --cursor;
    }
}

void SnailDB::tail() { cursor = numRows - 1; }

size_t SnailDB::getCursor() const { return cursor; }

size_t SnailDB::getSize() const { return numRows; }

void SnailDB::ensureColInitialized(const std::string &colName)
{
    auto it = colIndexMap.find(colName);
    if (it == colIndexMap.end())
    {
        // Column not found, initialize it
        colIndexMap[colName] = colNames.size();
        data.push_back(std::vector<SnailDataType *>(numRows + 1, nullptr));
    }
}

void SnailDB::addRow(const std::vector<SnailDataType *> &rowData)
{
    if (rowData.size() != colNames.size())
    {
        throw std::runtime_error("Number of elements in rowData doesn't match the number of columns.");
    }

    for (size_t i = 0; i < colNames.size(); ++i)
    {
        if (data[cursor][i] == nullptr)
        {
            // Ensure column is initialized for the current row
            ensureColInitialized(columnInfo[i].name);

            // Instantiate SnailDataType based on column type
            switch (columnInfo[i].type)
            {
            case STR_TYPE:
                data[cursor][i] = new StrCol(columnInfo[i].max_length);
                break;
            case INT_TYPE:
                data[cursor][i] = new IntCol(columnInfo[i].max_length);
                break;
            }
        }

        // Set the value for the current row
        data[cursor][i]->setValue(rowData[i]->getValue());
    }

    ++numRows;
}
