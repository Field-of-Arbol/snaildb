#ifndef SNAILDB_H
#define SNAILDB_H

#include "snail_datatypes.h"
#include <sstream>
#include <stdexcept> // For exception handling
#include <vector>

class SnailDef;

class SnailDef
{
public:
    virtual ~SnailDef()
    {
        for (auto &dataType : dataTypes)
        {
            delete dataType;
        }
    }
    char separator = '|';

    virtual void addStrColProp(const std::string &colName, size_t max_length)
    {
        try
        {
            dataTypes.push_back(new StrCol(max_length));
            colNames.push_back(colName);
        }
        catch (const std::exception &e)
        {
            // Handle the exception, log the error, or throw it further.
            std::cerr << "Error adding StrCol property: " << e.what() << std::endl;
        }
    }

    virtual void addIntColProp(const std::string &colName, size_t max_length)
    {
        try
        {
            dataTypes.push_back(new IntCol(max_length));
            colNames.push_back(colName);
        }
        catch (const std::exception &e)
        {
            // Handle the exception, log the error, or throw it further.
            std::cerr << "Error adding IntCol property: " << e.what() << std::endl;
        }
    }

    // Get the concatenated and compacted values
    virtual std::string getRow() const
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
    };

    BaseSDataType *getColProp(size_t index) const
    {
        if (index < dataTypes.size())
        {
            return dataTypes[index];
        }
        return nullptr;
    }

protected:
    std::vector<BaseSDataType *> dataTypes;
    std::vector<std::string> colNames;
};

#endif // SNAILDB_H