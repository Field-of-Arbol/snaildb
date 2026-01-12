#ifndef SNAIL_DUMPER_H
#define SNAIL_DUMPER_H

#include "snaildb.h"
#include <iomanip>
#include <iostream>

class SnailDumper {
public:
  static void printTable(SnailDB &db, std::ostream &os = std::cout) {
    size_t originalCursor = db.getCursor();
    db.reset();

    // Print Header
    size_t cols = db.getColCount();
    for (size_t i = 0; i < cols; ++i) {
      os << db.getColName(i);
      if (i < cols - 1)
        os << "\t";
    }
    os << "\n";

    // Print Rows
    size_t rows = db.getSize();
    for (size_t i = 0; i < rows; ++i) {
      for (size_t c = 0; c < cols; ++c) {
        ColumnType type = db.getColType(c);
        if (type == INT_TYPE) {
          os << db.get<int>(c);
        } else if (type == STR_TYPE) {
          os << db.get<std::string>(c);
        } else {
          os << "ERR";
        }

        if (c < cols - 1)
          os << "\t";
      }
      os << "\n";
      db.next();
    }

    // Restore Cursor
    db.reset();
    for (size_t i = 0; i < originalCursor; ++i)
      db.next();
  }
};

#endif
