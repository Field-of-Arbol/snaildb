// snaildb.cpp
#include "snaildb.h"
#include <algorithm>
#include <cstdlib> // for std::atoi

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

// Internal Integer Column
class InternalIntColumn : public Column {
public:
  InternalIntColumn() {}

  ColumnType getType() const override { return INT_TYPE; }
  size_t size() const override { return storage.size(); }
  void reserve(size_t n) override { storage.reserve(n); }

  bool isSorted() const override { return sorted; }
  bool isIndexed() const override { return !index.empty(); }

  void addInt(int val) override {
    // Check sorted state
    if (sorted && !storage.empty()) {
      if (val < storage.back())
        sorted = false;
    }
    storage.push_back(val);
    // Note: Adding invalidates index if we maintain it live.
    // For simple embedded DB, we usually rebuild index or append.
    // Here, we invalidate index on add for simplicity.
    if (!index.empty())
      index.clear();
  }

  int getInt(size_t index) const override {
    if (index >= storage.size())
      return 0;
    return storage[index];
  }

  void createIndex() override {
    // Build index vector
    index.clear();
    index.reserve(storage.size());
    for (size_t i = 0; i < storage.size(); ++i) {
      index.push_back({hashInt(storage[i]), (uint16_t)i});
    }
    // Sort by hash for binary search
    std::sort(index.begin(), index.end());
  }

  int find(const std::string &pattern) const override {
    // Safety Fix: No exceptions
    // Use atoi/atol. If pattern is not a number, it return 0.
    // Ideally we check isdigit, but for embedded speed/size, atoi is standard
    // fallback. If strict validation is needed, we would walk the string.
    int val = std::atoi(pattern.c_str());

    // Strategy 1: Index Search (Hash)
    if (!index.empty()) {
      uint32_t h = hashInt(val);
      // Binary search in index
      auto it = std::lower_bound(index.begin(), index.end(), IndexEntry{h, 0});
      // Check for collisions loop
      while (it != index.end() && it->hash == h) {
        if (storage[it->rowIdx] == val)
          return it->rowIdx;
        it++;
      }
      return -1; // Not found in index
    }

    // Strategy 2: Sorted Search (Binary Search on Data)
    if (sorted) {
      auto it = std::lower_bound(storage.begin(), storage.end(), val);
      if (it != storage.end() && *it == val) {
        return std::distance(storage.begin(), it);
      }
      return -1;
    }

    // Strategy 3: Linear Scan
    for (size_t i = 0; i < storage.size(); ++i) {
      if (storage[i] == val)
        return i;
    }
    return -1;
  }

private:
  std::vector<int> storage;
  std::vector<IndexEntry> index;
  bool sorted = true;
};

// Internal String Column
class InternalStrColumn : public Column {
public:
  InternalStrColumn(size_t maxLen) : maxLength(maxLen) {}

  ColumnType getType() const override { return STR_TYPE; }
  size_t size() const override {
    return maxLength == 0 ? 0 : storage.size() / maxLength;
  }
  void reserve(size_t n) override { storage.reserve(n * maxLength); }

  bool isSorted() const override { return sorted; }
  bool isIndexed() const override { return !index.empty(); }

  void addStr(const std::string &val) override {
    // Optimization: Zero-Copy Logic where possible

    // 1. Calculate effective length (Truncate)
    size_t len = val.length();
    if (len > maxLength)
      len = maxLength;

    // 2. Sorting Check (Compare new vs last)
    // We need to construct the would-be padded string virtually to compare.
    // Or just compare bytes.
    // Left Padding -> spaces then string.
    // If we sort, we must compare the full padded representation.
    if (sorted && size() > 0) {
      // Retrieve last row efficiently
      // getStr creates a string, which is a copy, but safe for logic reuse.
      // To be purely optimal we would scan storage directly, but getStr is
      // reliable.
      std::string last = getStr(size() - 1);

      // Construct padded temp only if necessary for comparison
      // (Actually, comparing unpadded 'val' vs padded 'last' is tricky
      // manually) Let's perform the copy for the check logic to remain robust,
      // as the sorting check is less frequent/critical than the insertion bulk.
      // BUT, user asked for optimization.

      // Let's prioritize correct sorting check without full string alloc if
      // possible. Or just accept one alloc here. Wait, for strict optimization,
      // we want to write directly to storage logic first.
    }

    // For now, let's keep the sorted logic simple (it might alloc),
    // but optimize the STORAGE WRITE to not alloc intermediate strings.

    // Check Sorted (Simplified Logic)
    // To strictly avoid allocs would require complex iterator logic.
    // Let's alloc JUST for the check if sorted.
    if (sorted && size() > 0) {
      std::string last = getStr(size() - 1);
      // We need to compare "    val" vs "last"
      // It's hard to do without constructing "    val".
      std::string paddedVal =
          std::string(maxLength - len, ' ') + val.substr(0, len);
      if (paddedVal < last)
        sorted = false;
    }

    // 3. Write to Storage (No intermediate allocs for the massive vector)
    size_t currentEnd = storage.size();
    storage.resize(currentEnd + maxLength); // allocate raw chars

    // Fill Pad (Left Padding)
    size_t padLen = maxLength - len;
    // std::fill isn't needed if we loop, but explicit loop is fine.
    for (size_t i = 0; i < padLen; ++i) {
      storage[currentEnd + i] = ' ';
    }

    // Fill String
    for (size_t i = 0; i < len; ++i) {
      storage[currentEnd + padLen + i] = val[i];
    }

    // Invalidate Index
    if (!index.empty())
      index.clear();
  }

  std::string getStr(size_t index) const override {
    size_t offset = index * maxLength;
    if (offset >= storage.size())
      return "";
    return std::string(storage.begin() + offset,
                       storage.begin() + offset + maxLength);
  }

  void createIndex() override {
    index.clear();
    size_t rows = size();
    index.reserve(rows);
    for (size_t i = 0; i < rows; ++i) {
      // Hash specific slice
      size_t offset = i * maxLength;
      uint32_t h = hashStr(&storage[offset], maxLength);
      index.push_back({h, (uint16_t)i});
    }
    std::sort(index.begin(), index.end());
  }

  int find(const std::string &pattern) const override {
    // Pad search pattern to match storage
    std::string searchPat = pattern;
    if (searchPat.length() < maxLength) {
      searchPat = std::string(maxLength - searchPat.length(), ' ') + searchPat;
    } else if (searchPat.length() > maxLength) {
      searchPat = searchPat.substr(0, maxLength);
    }

    // Strategy 1: Index Search
    if (!index.empty()) {
      uint32_t h = hashStr(searchPat.c_str(), maxLength);
      auto it = std::lower_bound(index.begin(), index.end(), IndexEntry{h, 0});
      while (it != index.end() && it->hash == h) {
        // Collision check
        size_t offset = it->rowIdx * maxLength;
        bool match = true;
        for (size_t k = 0; k < maxLength; ++k) {
          if (storage[offset + k] != searchPat[k]) {
            match = false;
            break;
          }
        }
        if (match)
          return it->rowIdx;
        it++;
      }
      return -1;
    }

    // Strategy 2: Sorted Search
    // Caveat: We are storing flattened vector. std::lower_bound logic is
    // complex here. We need a custom iterator or comparator that accesses the
    // flat vector. Implementing strict Binary Search manually for flat vector
    // is easier.
    if (sorted) {
      int left = 0, right = size() - 1;
      while (left <= right) {
        int mid = left + (right - left) / 2;
        std::string midVal = getStr(mid);
        if (midVal == searchPat)
          return mid;
        if (midVal < searchPat)
          left = mid + 1;
        else
          right = mid - 1;
      }
      return -1;
    }

    // Strategy 3: Linear Scan
    size_t rows = size();
    for (size_t i = 0; i < rows; ++i) {
      size_t offset = i * maxLength;
      bool match = true;
      for (size_t k = 0; k < maxLength; ++k) {
        if (storage[offset + k] != searchPat[k]) {
          match = false;
          break;
        }
      }
      if (match)
        return i;
    }
    return -1;
  }

private:
  std::vector<char> storage;
  std::vector<IndexEntry> index;
  bool sorted = true;
  size_t maxLength;
};

// =========================================================
// SnailDB Implementation
// =========================================================

SnailDB::SnailDB() : numRows(0), cursor(0) {}

SnailDB::~SnailDB() { columns.clear(); }

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
}

void SnailDB::createIndex() {
  for (auto &col : columns) {
    col->createIndex();
  }
}

// Typed Dispatch
void SnailDB::addToCol(size_t colIdx, int val) {
  if (colIdx >= columns.size())
    return;
  columns[colIdx]->addInt(val);
}

void SnailDB::addToCol(size_t colIdx, const std::string &val) {
  if (colIdx >= columns.size())
    return;
  columns[colIdx]->addStr(val);
}

void SnailDB::addToCol(size_t colIdx, const char *val) {
  addToCol(colIdx, std::string(val));
}

// Navigation
void SnailDB::next() {
  if (cursor < numRows - 1)
    cursor++;
}
void SnailDB::previous() {
  if (cursor > 0)
    cursor--;
}
void SnailDB::tail() {
  if (numRows > 0)
    cursor = numRows - 1;
}
void SnailDB::reset() { cursor = 0; }
size_t SnailDB::getCursor() const { return cursor; }
size_t SnailDB::getSize() const { return numRows; }

int SnailDB::getColIndex(const std::string &name) const {
  for (size_t i = 0; i < colNames.size(); ++i) {
    if (colNames[i] == name)
      return i;
  }
  return -1;
}

ColumnType SnailDB::getColType(size_t idx) const {
  if (idx >= columns.size())
    return (ColumnType)-1;
  return columns[idx]->getType();
}

int SnailDB::findRow(const std::string &colName,
                     const std::string &value) const {
  int colIdx = getColIndex(colName);
  if (colIdx == -1)
    return -1;
  return columns[colIdx]->find(value);
}
