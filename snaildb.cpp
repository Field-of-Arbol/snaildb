// snaildb.cpp
#include "snaildb.h"

SnailDef::~SnailDef()
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

void SnailDef::initializeData()
{
    // Initialize the two-dimensional array
    data = new BaseSDataType **[numRows];
    for (size_t i = 0; i < numRows; ++i)
    {
        data[i] = new BaseSDataType *[numCols];
    }
}

void SnailDef::addStrColProp(const std::string &colName, size_t max_length)
{
    try
    {
        data[cursor][colNames.size()] = new StrCol(max_length);
        colNames.push_back(colName);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("Error adding StrCol property: ") + e.what());
    }
}

void SnailDef::addIntColProp(const std::string &colName, size_t max_length)
{
    try
    {
        data[cursor][colNames.size()] = new IntCol(max_length);
        colNames.push_back(colName);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("Error adding IntCol property: ") + e.what());
    }
}

std::string SnailDef::getRow() const
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

BaseSDataType *SnailDef::getProp(size_t col) const
{
    if (cursor < numRows && col < numCols)
    {
        return data[cursor][col];
    }
    return nullptr;
}

void SnailDef::next()
{
    if (cursor < numRows - 1)
    {
        ++cursor;
    }
}

void SnailDef::previous()
{
    if (cursor > 0)
    {
        --cursor;
    }
}

void SnailDef::tail()
{
    cursor = numRows - 1;
}

size_t SnailDef::getCursor() const
{
    return cursor;
}

size_t SnailDef::getSize() const
{
    return numRows;
}