// snaildb.h
#ifndef SNAILDB_H
#define SNAILDB_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Type Definitions (Migrated from snail_datatypes)
enum ColumnType { STR_TYPE, INT_TYPE };

struct ColumnInfo {
  std::string name;
  size_t max_length;
  ColumnType type;
};

// Indexing Structures
struct IndexEntry {
  uint32_t hash;
  uint16_t rowIdx;

  // For sorting the index
  bool operator<(const IndexEntry &other) const { return hash < other.hash; }
};

// Abstract Base Column Definition
class Column {
public:
  virtual ~Column() {}

  // Type Info
  virtual ColumnType getType() const = 0;
  virtual size_t size() const = 0;
  virtual void reserve(size_t n) = 0;

  // Optimization Flags
  virtual bool isSorted() const = 0;
  virtual bool isIndexed() const = 0;

  // Actions
  virtual void createIndex() = 0;

  // Typed Accessors (No Exceptions - Return Success/Fail or Default)
  // For simplicity in this embedded ctx, mismatched calls do nothing or return
  // default.
  virtual void addInt(int val) {}
  virtual void addStr(const std::string &val) {}

  virtual int getInt(size_t index) const { return 0; }
  virtual std::string getStr(size_t index) const { return ""; }

  // Search (Generic)
  virtual int find(const std::string &pattern) const = 0;
};

// SnailDB Main Class
class SnailDB {
public:
  SnailDB();
  virtual ~SnailDB();

  // Setup Schema
  void addStrColProp(const std::string &colName, size_t max_length);
  void addIntColProp(const std::string &colName, size_t max_length);

  // Memory & Optimization
  void reserve(size_t rows);
  void createIndex(); // Create indices for all supported columns

  // Variadic Insert
  template <typename... Args> void insert(Args... args) {
    if (sizeof...(args) != colNames.size()) {
      return; // Error: Mismatch (Silently fail or log in debug)
    }
    insertImpl(0, args...);
    numRows++;
  }

  // Typed Data Access
  template <typename T> T get(size_t colIndex) const;

  // Navigation
  void next();
  void previous();
  void tail();
  void reset();
  size_t getCursor() const;
  size_t getSize() const;

  // Metadata Access for Dumper
  size_t getColCount() const { return colNames.size(); }
  std::string getColName(size_t idx) const { return colNames[idx]; }
  ColumnType getColType(size_t idx) const;

  // Helpers
  int findRow(const std::string &colName, const std::string &value) const;

protected:
  std::vector<std::unique_ptr<Column>> columns;
  std::vector<std::string> colNames;
  std::vector<ColumnInfo> colInfos;

  size_t numRows;
  size_t cursor;

  int getColIndex(const std::string &name) const;

private:
  // Recursive variadic unpacker
  void insertImpl(size_t colIdx) {} // Base case

  template <typename T, typename... Rest>
  void insertImpl(size_t colIdx, T val, Rest... rest) {
    addToCol(colIdx, val);
    insertImpl(colIdx + 1, rest...);
  }

  // Typed dispatch helpers
  void addToCol(size_t colIdx, int val);
  void addToCol(size_t colIdx, const std::string &val);
  void addToCol(size_t colIdx, const char *val);
};

// Template Specializations / Definitions
template <> inline int SnailDB::get<int>(size_t colIndex) const {
  if (colIndex >= columns.size())
    return 0;
  return columns[colIndex]->getInt(cursor);
}

template <>
inline std::string SnailDB::get<std::string>(size_t colIndex) const {
  if (colIndex >= columns.size())
    return "";
  return columns[colIndex]->getStr(cursor);
}

#endif // SNAILDB_H