// snaildb.cpp
#include "snaildb.h"
#include <algorithm>
#include <cstdlib> // for std::atoi
#include <cstring> // for std::memcpy

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

InternalIntColumn::InternalIntColumn() {}

ColumnType InternalIntColumn::getType() const { return INT_TYPE; }
size_t InternalIntColumn::size() const { return storage.size(); }
void InternalIntColumn::reserve(size_t n) { storage.reserve(n); }
bool InternalIntColumn::isSorted() const { return sorted; }
bool InternalIntColumn::isIndexed() const { return !index.empty(); }

void InternalIntColumn::addInt(int val) {
  if (sorted && !storage.empty()) {
    if (val < storage.back())
      sorted = false;
  }
  storage.push_back(val);
  if (!index.empty())
    index.clear();
}

int InternalIntColumn::getInt(size_t index) const {
  if (index >= storage.size())
    return 0;
  return storage[index];
}

void InternalIntColumn::createIndex() {
  index.clear();
  index.reserve(storage.size());
  for (size_t i = 0; i < storage.size(); ++i) {
    index.push_back({hashInt(storage[i]), (uint16_t)i});
  }
  std::sort(index.begin(), index.end());
}

int InternalIntColumn::find(const std::string &pattern) const {
  int val = std::atoi(pattern.c_str());

  // Strategy 1: Index
  if (!index.empty()) {
    uint32_t h = hashInt(val);
    auto it = std::lower_bound(index.begin(), index.end(), IndexEntry{h, 0});
    while (it != index.end() && it->hash == h) {
      if (storage[it->rowIdx] == val)
        return it->rowIdx;
      it++;
    }
    return -1;
  }

  // Strategy 2: Sorted
  if (sorted) {
    auto it = std::lower_bound(storage.begin(), storage.end(), val);
    if (it != storage.end() && *it == val) {
      return std::distance(storage.begin(), it);
    }
    return -1;
  }

  // Strategy 3: Linear
  for (size_t i = 0; i < storage.size(); ++i) {
    if (storage[i] == val)
      return i;
  }
  return -1;
}

// Persistence handled by SnailStorage friend access

// --- InternalStrColumn (Dictionary Compressed) ---

InternalStrColumn::InternalStrColumn(size_t maxLen) : maxLength(maxLen) {}

ColumnType InternalStrColumn::getType() const { return STR_TYPE; }
size_t InternalStrColumn::size() const { return data.size(); }
void InternalStrColumn::reserve(size_t n) { data.reserve(n); }
bool InternalStrColumn::isSorted() const { return sorted; }
bool InternalStrColumn::isIndexed() const { return !index.empty(); }

void InternalStrColumn::addStr(const std::string &val) {
  // 1. Truncate / Pad?
  // With Dictionary, we store the unique string as-is (padded or unpadded).
  // Previous logic enforced Left-Padding.
  // Let's maintain that behavior for consistency and sorting.

  std::string s = val;
  if (s.length() > maxLength)
    s = s.substr(0, maxLength);

  // Pad
  std::string padded = s;
  if (padded.length() < maxLength) {
    padded = std::string(maxLength - padded.length(), ' ') + padded;
  }

  // 2. Dictionary Search
  // Linear scan of dictionary is O(D) where D is small.
  // For optimization, we could use a map, but we want to avoid extra memory
  // overhead. If D is < 100, linear is fast. If D is 65k... maybe slow. But
  // this is embedded. Let's assume linear is okay or user calls createIndex?
  // Actually, for insertion speed, a transient map/hash would help, but let's
  // keep it simple O(D).

  int16_t token = -1;
  for (size_t i = 0; i < dictionary.size(); ++i) {
    if (dictionary[i] == padded) {
      token = (int16_t)i;
      break;
    }
  }

  if (token == -1) {
    // New Entry
    if (dictionary.size() >= 65535) {
      // Error: Dict Full. Fallback to token 0 or do nothing?
      // For now, fail safe.
      token = 0;
    } else {
      dictionary.push_back(padded);
      token = dictionary.size() - 1;
    }
  }

  // 3. Sorting Check
  if (sorted && !data.empty()) {
    const std::string &last = dictionary[data.back()];
    if (padded < last)
      sorted = false;
  }

  // 4. Store Token
  data.push_back((uint16_t)token);

  // 5. Invalidate Index
  if (!index.empty())
    index.clear();
}

std::string InternalStrColumn::getStr(size_t index) const {
  if (index >= data.size())
    return "";
  uint16_t token = data[index];
  if (token >= dictionary.size())
    return ""; // Error
  return dictionary[token];
}

void InternalStrColumn::createIndex() {
  index.clear();
  index.reserve(data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    // Hash the ACTUAL string value
    const std::string &val = dictionary[data[i]];
    // Note: hashing padded string
    uint32_t h = hashStr(val.c_str(), val.length());
    index.push_back({h, (uint16_t)i});
  }
  std::sort(index.begin(), index.end());
}

int InternalStrColumn::find(const std::string &pattern) const {
  // 1. Pad pattern
  std::string searchPat = pattern;
  if (searchPat.length() < maxLength) {
    searchPat = std::string(maxLength - searchPat.length(), ' ') + searchPat;
  } else if (searchPat.length() > maxLength) {
    searchPat = searchPat.substr(0, maxLength);
  }

  // OPTIMIZATION: Check if valid token exists first!
  int16_t token = -1;
  for (size_t i = 0; i < dictionary.size(); ++i) {
    if (dictionary[i] == searchPat) {
      token = (int16_t)i;
      break;
    }
  }
  if (token == -1)
    return -1; // Fast Fail: String not in DB.

  // If found, now we search for 'token' in 'data'.

  // Strategy 1: Index (Hash of string)
  // If index exists, it maps Hash(String) -> RowIdx.
  if (!index.empty()) {
    uint32_t h = hashStr(searchPat.c_str(), searchPat.length());
    auto it = std::lower_bound(index.begin(), index.end(), IndexEntry{h, 0});
    while (it != index.end() && it->hash == h) {
      // Resolve row -> token -> string
      if (data[it->rowIdx] == token)
        return it->rowIdx;
      it++;
    }
    return -1;
  }

  // Strategy 2: Sorted (Token comparison?)
  // Warning: 'sorted' means the strings are sorted, NOT the tokens.
  // e.g. Dict: ["Apple"(0), "Banana"(1)]. Data: [0, 1]. Sorted.
  // e.g. Dict: ["Banana"(0), "Apple"(1)]. Data: [1, 0]. Sorted Strings, but
  // Tokens unsorted. So we can ONLY use binary search on 'data' if we assume
  // logic... We can use std::lower_bound with a custom comparator that looks up
  // dict!
  if (sorted) {
    // lower_bound on data, comparing looked-up strings
    // Value to find: searchPat
    auto it = std::lower_bound(data.begin(), data.end(), searchPat,
                               [this](uint16_t t, const std::string &val) {
                                 return this->dictionary[t] < val;
                               });

    if (it != data.end() && dictionary[*it] == searchPat) {
      return std::distance(data.begin(), it);
    }
    return -1;
  }

  // Strategy 3: Linear Scan on Tokens (Fast integer compare)
  for (size_t i = 0; i < data.size(); ++i) {
    if (data[i] == token)
      return i;
  }

  return -1;
}

// Persistence handled by SnailStorage friend access

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
