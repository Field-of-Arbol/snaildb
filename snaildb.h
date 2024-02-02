// snaildb.h
#ifndef SNAILDB_H
#define SNAILDB_H

#include "snail_datatypes.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <map>

class SnailDB
{
public:
    virtual ~SnailDB();

    void addStrColProp(const std::string &colName, size_t max_length);
    void addIntColProp(const std::string &colName, size_t max_length);
    std::string getRow() const;
    SnailDataType *getProp(size_t col) const;
    void addRow(const std::vector<SnailDataType *> &rowData);
    void next();
    void previous();
    void tail();
    void add();
    size_t getCursor() const;
    size_t getSize() const;

protected:
    SnailDataType ***data;                     // Two-dimensional array of pointers
    std::map<std::string, size_t> colIndexMap; // Map to store column indices
    std::vector<std::string> colNames;
    char separator = '|';
    size_t numRows = 0; // Number of rows
    size_t numCols;     // Number of columns
    size_t cursor = 0;  // Current cursor position
    void ensureColInitialized(const std::string &colName, size_t requiredRows);
    std::vector<bool> indexedColumns; // Vector to track indexed columns

    //
};

#endif // SNAILDB_H