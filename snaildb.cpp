// snaildb.cpp
#include "snaildb.h"

SnailDB::~SnailDB()
{
    // Delete allocated memory for data
    for (size_t i = 0; i < numRows; ++i)
    {
        for (size_t j = 0; j < numCols; ++j)
        {
            delete data[i][j];
        }
        delete[] data[i];
    }
    delete[] data;
}

void SnailDB::addStrColProp(const std::string &colName, size_t max_length)
{
    try
    {
        data[cursor][colNames.size()] = new StrCol(max_length);
        colNames.push_back(colName);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("Error adding StrCol property: ") +
                                 e.what());
    }
}

void SnailDB::addIntColProp(const std::string &colName, size_t max_length)
{
    try
    {
        data[cursor][colNames.size()] = new IntCol(max_length);
        colNames.push_back(colName);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("Error adding IntCol property: ") +
                                 e.what());
    }
}

void SnailDB::addRow(const std::vector<SnailDataType *> &rowData)
{
    size_t requiredCols = colNames.size();

    // Increment numRows if adding a new row
    if (cursor >= numRows)
    {
        ++numRows;
    }

    // Ensure proper initialization of columns for the new row
    for (size_t i = 0; i < requiredCols; ++i)
    {
        ensureColInitialized(colNames[i], numRows);
    }

    // Add values to the row if provided
    for (size_t i = 0; i < std::min(rowData.size(), requiredCols); ++i)
    {
        size_t colIndex = colIndexMap.at(colNames[i]);
        if (rowData[i])
        {
            if (data[colIndex][cursor])
            {
                delete data[colIndex][cursor];
            }
            data[colIndex][cursor] = new SnailDataType(*rowData[i]); // Assuming a copy constructor is available
        }
        else
        {
            data[colIndex][cursor] = nullptr;
        }
    }
}

std::string SnailDB::getRow() const
{
    std::string result;
    for (size_t i = 0; i < numCols; ++i)
    {
        result += data[cursor][i]->getValue();
        if (i != numCols - 1)
        {
            result += separator;
        }
    }
    return result;
}

SnailDataType *SnailDB::getProp(size_t col) const
{
    if (cursor < numRows && col < numCols)
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

void SnailDB::add()
{
    if (cursor < numRows - 1)
    {
        ++cursor;
    }
    else
    {
        // Allocate memory for a new row
        data[cursor + 1] = new SnailDataType *[numCols];
        for (size_t i = 0; i < numCols; ++i)
        {
            data[cursor + 1][i] = nullptr; // Initialize with nullptr for now
        }
        ++numRows;
    }
}

void SnailDB::tail() { cursor = numRows - 1; }

size_t SnailDB::getCursor() const { return cursor; }

size_t SnailDB::getSize() const { return numRows; }

void SnailDB::ensureColInitialized(const std::string &colName, size_t requiredRows)
{
    if (colIndexMap.find(colName) == colIndexMap.end())
    {
        colIndexMap[colName] = colNames.size();
        colNames.push_back(colName);
        indexedColumns.push_back(false); // Initialize indexedColumns vector

        // Add a new column (vector of SnailDataType*) to data
        /*
        std::vector<SnailDataType*> newColumn;
        newColumn.reserve(requiredRows);
        for (size_t i = 0; i < requiredRows; ++i) {
          newColumn.push_back(nullptr); // Initialize with nullptr for now
        }
        data.push_back(newColumn);
        */
    }
}