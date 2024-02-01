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
    virtual ~SnailDef();
    char separator = '|';

    virtual void addStrColProp(const std::string &colName, size_t max_length);
    virtual void addIntColProp(const std::string &colName, size_t max_length);

    virtual std::string getRow() const;
    BaseSDataType *getProp(size_t index) const;

protected:
    std::vector<BaseSDataType *> dataTypes;
    std::vector<std::string> colNames;
};

#endif // SNAILDB_H