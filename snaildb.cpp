// snaildb.cpp
#include "snaildb.h"

SnailDef::~SnailDef()
{
    for (auto &dataType : dataTypes)
    {
        delete dataType;
    }
}

void SnailDef::addStrColProp(const std::string &colName, size_t max_length)
{
    try
    {
        dataTypes.push_back(new StrCol(max_length));
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
        dataTypes.push_back(new IntCol(max_length));
        colNames.push_back(colName);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("Error adding IntCol property: ") + e.what());
    }
}

std::string SnailDef::getRow() const
{
    std::ostringstream result;
    for (size_t i = 0; i < dataTypes.size(); ++i)
    {
        result << dataTypes[i]->getValue();
        if (i != dataTypes.size() - 1)
        {
            result << separator;
        }
    }
    return result.str();
}

BaseSDataType *SnailDef::getProp(size_t index) const
{
    if (index < dataTypes.size())
    {
        return dataTypes[index];
    }
    return nullptr;
}
