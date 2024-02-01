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
    virtual void addStrColProp(const std::string &colName, size_t max_length);
    virtual void addIntColProp(const std::string &colName, size_t max_length);

    std::string getRow() const;
    BaseSDataType *getProp(size_t col) const;
    void next();
    void previous();
    void tail();
    size_t getCursor() const;
    size_t getSize() const;

protected:
  BaseSDataType ***data;
  std::vector<std::string> colNames;
  char separator = '|';
  size_t numRows; // Number of rows
  size_t numCols; // Number of columns
  size_t cursor;  // Current cursor position

  // Helper function to initialize the two-dimensional array
  void initializeData();
};

#endif // SNAILDB_H