#include "snaildb.h"
#include <algorithm>
#include <cstdlib> // std::atoi
#include <cstring> // std::memcpy

// =========================================================
// Hashing Helper (DJB2)
// =========================================================
static uint32_t hashStr(const char *str, size_t len) {
  uint32_t hash = 5381;
  for (size_t i = 0; i < len; ++i) {
    hash = ((hash << 5) + hash) + str[i];
  }
  return hash;
}

static uint32_t hashInt(int val) {
  // Simple integer hash
  return (uint32_t)val * 2654435761u;
}

// =========================================================
// Internal Column Implementations
// =========================================================

// --- InternalIntColumn ---

class InternalIntColumn : public Column {
public:
  InternalIntColumn() {}

  ColumnType getType() const override { return INT_TYPE; }
  size_t size() const override { return storage.size(); }
  void reserve(size_t n) override { storage.reserve(n); }
  bool isSorted() const override { return sorted; }
  bool isIndexed() const override { return !index.empty(); }

  void compact(const std::vector<bool> &keepMask) override {
    if (keepMask.size() != storage.size()) return;
    
    // Two-Pointer In-Place Compaction
    size_t dst = 0;
    for (size_t i = 0; i < storage.size(); ++i) {
        if (keepMask[i]) {
            if (dst != i) {
                storage[dst] = storage[i];
            }
            dst++;
        }
    }
    storage.resize(dst);
    // Index/Sort state is invalidated by compaction unless we re-verify
    // For v1.0 simplicity, mark as unsorted/unindexed
    sorted = false;
    index.clear();
  }

  void addInt(int val) override {
    if (sorted && !storage.empty()) {
      if (val < storage.back())
        sorted = false;
    }
    storage.push_back(val);
    if (!index.empty()) index.clear();
  }

  int getInt(size_t index) const override {
    if (index >= storage.size()) return 0;
    return storage[index];
  }

  int find(const std::string &pattern) const override {
    // FIX: Using atoi instead of stoi (No Exceptions)
    int val = std::atoi(pattern.c_str());
    
    if (sorted) {
        // Binary Search
        auto it = std::lower_bound(storage.begin(), storage.end(), val);
        if (it != storage.end() && *it == val) {
            return (int)std::distance(storage.begin(), it);
        }
    } else {
        // Linear Scan
        for (size_t i = 0; i < storage.size(); ++i) {
            if (storage[i] == val) return i;
        }
    }
    return -1;
  }

  void createIndex() override {
    if (storage.empty()) return;
    index.resize(storage.size());
    for (size_t i = 0; i < storage.size(); ++i) {
        index[i] = { hashInt(storage[i]), (uint16_t)i };
    }
    std::sort(index.begin(), index.end());
  }

  // Friends for Persistence
  friend class SnailStorage;

protected:
    char* getWritableBuffer() override { return reinterpret_cast<char*>(storage.data()); }
    void resizeStorage(size_t bytes) override { 
        size_t count = bytes / sizeof(int);
        storage.resize(count);
        sorted = false; 
        index.clear();
    }

private:
  std::vector<int> storage;
  std::vector<IndexEntry> index;
  bool sorted = true;
};

// --- InternalStrColumn ---

class InternalStrColumn : public Column {
public:
  InternalStrColumn(size_t maxLen) : maxLength(maxLen) {}

  ColumnType getType() const override { return STR_TYPE; }
  
  // Size is strictly rows, not bytes
  size_t size() const override { return data.size(); }
  
  void reserve(size_t n) override { data.reserve(n); }
  bool isSorted() const override { return sorted; }
  bool isIndexed() const override { return !index.empty(); }

  void compact(const std::vector<bool> &keepMask) override {
      if (keepMask.size() != data.size()) return;

      size_t dst = 0;
      for (size_t i = 0; i < data.size(); ++i) {
          if (keepMask[i]) {
              if (dst != i) {
                  data[dst] = data[i];
              }
              dst++;
          }
      }
      data.resize(dst);
      sorted = false;
      index.clear();
      // Note: Dictionary is NOT compacted in v1.0 (Append-only dict)
  }

  void addStr(const std::string &val) override {
      uint16_t token = 0;
      // 1. Try to find in existing dictionary
      int existingIdx = -1;
      // Simple linear search on dictionary is slow, but dict is small.
      // Optimization: Could use a map, but RAM usage... keeping simple for v1.0
      for(size_t i=0; i<dictionary.size(); ++i) {
          if(dictionary[i] == val) {
              existingIdx = i;
              break;
          }
      }

      if (existingIdx != -1) {
          token = (uint16_t)existingIdx;
      } else {
          // Add new
          if (dictionary.size() < 65535) {
            dictionary.push_back(val);
            token = (uint16_t)(dictionary.size() - 1);
          } else {
            token = 0; // Overflow fallback
          }
      }
      data.push_back(token);
      sorted = false; 
      if(!index.empty()) index.clear();
  }

  std::string getStr(size_t index) const override {
    if (index >= data.size()) return "";
    uint16_t token = data[index];
    if (token < dictionary.size()) {
        return dictionary[token];
    }
    return "";
  }

  int find(const std::string &pattern) const override {
      // 1. Find token for pattern
      int targetToken = -1;
      for(size_t i=0; i<dictionary.size(); ++i) {
          if(dictionary[i] == pattern) {
              targetToken = i;
              break;
          }
      }
      
      // Fast Fail: Token not in dict? Value not in DB.
      if (targetToken == -1) return -1;

      // 2. Find token in data
      for (size_t i = 0; i < data.size(); ++i) {
          if (data[i] == targetToken) return i;
      }
      return -1;
  }

  void createIndex() override {
      // Not implemented for v1.0 Str (Dictionary is already an index of sorts)
  }
  
  // Friends for Persistence
  friend class SnailStorage;

protected:
    char* getWritableBuffer() override { return reinterpret_cast<char*>(data.data()); }
    void resizeStorage(size_t bytes) override {
        size_t count = bytes / sizeof(uint16_t);
        data.resize(count);
        sorted = false;
    }

private:
  size_t maxLength;
  std::vector<std::string> dictionary;
  std::vector<uint16_t> data; // Tokens
  std::vector<IndexEntry> index;
  bool sorted = false; // Strings rarely inserted sorted
};

// =========================================================
// SnailDB Implementation
// =========================================================

SnailDB::SnailDB() : numRows(0), cursor(0) {}

SnailDB::~SnailDB() {}

void SnailDB::addStrColProp(const std::string &colName, size_t max_length) {
  colNames.push_back(colName);
  colInfos.push_back({colName, max_length, STR_TYPE});
  columns.push_back(std::unique_ptr<Column>(new InternalStrColumn(max_length)));
}

void SnailDB::addIntColProp(const std::string &colName, size_t max_length) {
  colNames.push_back(colName);
  colInfos.push_back({colName, max_length, INT_TYPE});
  columns.push_back(std::unique_ptr<Column>(new InternalIntColumn()));
}

void SnailDB::reserve(size_t rows) {
  for (auto &col : columns) {
    col->reserve(rows);
  }
  activeRows.reserve(rows);
  timestamps.reserve(rows);
}

void SnailDB::createIndex() {
  for (auto &col : columns) {
    col->createIndex();
  }
}

// Typed Dispatch
void SnailDB::addToCol(size_t colIdx, int val) {
  if (colIdx >= columns.size()) return;
  columns[colIdx]->addInt(val);
}

void SnailDB::addToCol(size_t colIdx, const std::string &val) {
  if (colIdx >= columns.size()) return;
  columns[colIdx]->addStr(val);
}

void SnailDB::addToCol(size_t colIdx, const char *val) {
  addToCol(colIdx, std::string(val));
}

// Lifecycle Management
void SnailDB::softDelete(size_t index) {
    if (index < activeRows.size()) {
        activeRows[index] = false;
    }
}

void SnailDB::deleteOlderThan(uint32_t threshold) {
    for(size_t i=0; i<timestamps.size(); ++i) {
        if (timestamps[i] < threshold) {
            activeRows[i] = false;
        }
    }
}

void SnailDB::purge() {
    if (activeRows.empty()) return;

    // 1. Compact all columns
    for (auto &col : columns) {
        col->compact(activeRows);
    }

    // 2. Compact timestamps
    size_t dst = 0;
    for (size_t i = 0; i < timestamps.size(); ++i) {
        if (activeRows[i]) {
            if (dst != i) timestamps[dst] = timestamps[i];
            dst++;
        }
    }
    timestamps.resize(dst);

    // 3. Reset Active Mask
    numRows = dst;
    activeRows.assign(numRows, true);
    if (cursor >= numRows && numRows > 0) cursor = numRows - 1;
}


// Navigation
void SnailDB::next() {
  if (cursor < numRows) cursor++;
  while (cursor < numRows && !activeRows[cursor]) {
    cursor++;
  }
  if (cursor >= numRows && numRows > 0) tail();
}

void SnailDB::previous() {
  if (cursor > 0) cursor--;
  while (cursor > 0 && !activeRows[cursor]) {
    cursor--;
  }
}

void SnailDB::tail() {
  if (numRows == 0) { cursor = 0; return; }
  cursor = numRows - 1;
  while (cursor > 0 && !activeRows[cursor]) {
    cursor--;
  }
}

void SnailDB::reset() {
  cursor = 0;
  if (numRows > 0 && !activeRows[cursor]) next();
}

size_t SnailDB::getCursor() const { return cursor; }

size_t SnailDB::getSize() const {
    // Return ACTIVE count
    size_t count = 0;
    for(bool b : activeRows) if(b) count++;
    return count;
}

int SnailDB::getColIndex(const std::string &name) const {
  for (size_t i = 0; i < colNames.size(); ++i) {
    if (colNames[i] == name) return i;
  }
  return -1;
}

ColumnType SnailDB::getColType(size_t idx) const {
    if (idx < colInfos.size()) return colInfos[idx].type;
    return INT_TYPE; // default
}

int SnailDB::findRow(const std::string &colName, const std::string &value) const {
  int idx = getColIndex(colName);
  if (idx == -1) return -1;
  int foundIdx = columns[idx]->find(value);
  
  // Verify if valid
  if (foundIdx != -1 && foundIdx < (int)activeRows.size() && !activeRows[foundIdx]) {
      return -1; // Found but deleted
  }
  return foundIdx;
}