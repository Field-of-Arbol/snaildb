#ifndef SNAIL_STORAGE_H
#define SNAIL_STORAGE_H

#include "snaildb.h"
#include <fstream>
#include <iostream>
#include <vector>

// Note for ESP32:
// Replace <fstream> with LittleFS/SPIFFS file headers.
// Replace std::ofstream with fs::File (write mode)
// Replace std::ifstream with fs::File (read mode)

class SnailStorage {
public:
  static bool save(const SnailDB &db, const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open())
      return false;

    // 1. Magic "SNAL"
    file.write("SNAL", 4);

    // 2. Header
    uint32_t numRows = (uint32_t)db.numRows;
    uint32_t numCols = (uint32_t)db.colInfos.size();
    file.write((const char *)&numRows, sizeof(numRows));
    file.write((const char *)&numCols, sizeof(numCols));

    // 3. Schema
    for (const auto &info : db.colInfos) {
      uint8_t type = (uint8_t)info.type;
      uint16_t maxLen = (uint16_t)info.max_length;
      uint8_t nameLen = (uint8_t)info.name.length();

      file.write((const char *)&type, sizeof(type));
      file.write((const char *)&maxLen, sizeof(maxLen));
      file.write((const char *)&nameLen, sizeof(nameLen));
      file.write(info.name.c_str(), nameLen);
    }
    // 4. Data Blocks
    for (size_t i = 0; i < numCols; ++i) {
      Column *col = db.columns[i].get();

      if (col->getType() == INT_TYPE) {
        // IntColumn: Dump Storage
        InternalIntColumn *intCol = static_cast<InternalIntColumn *>(col);
        size_t size = intCol->storage.size() * sizeof(int);
        file.write((const char *)intCol->storage.data(), size);
      } else {
        // StrColumn: Dump Dictionary + Data
        InternalStrColumn *strCol = static_cast<InternalStrColumn *>(col);

        // 1. Dict Size
        uint16_t dictSize = (uint16_t)strCol->dictionary.size();
        file.write((const char *)&dictSize, sizeof(dictSize));

        // 2. Dict Items
        for (const auto &s : strCol->dictionary) {
          uint16_t len = (uint16_t)s.length();
          file.write((const char *)&len, sizeof(len));
          file.write(s.data(), len);
        }

        // 3. Data Vector (Tokens)
        size_t size = strCol->data.size() * sizeof(uint16_t);
        file.write((const char *)strCol->data.data(), size);
      }
    }

    file.close();
    return true;
  }

  static bool load(SnailDB &db, const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
      return false;

    // 1. Magic
    char magic[4];
    file.read(magic, 4);
    if (std::strncmp(magic, "SNAL", 4) != 0)
      return false;

    // 2. Header
    uint32_t numRows, numCols;
    file.read((char *)&numRows, sizeof(numRows));
    file.read((char *)&numCols, sizeof(numCols));

    // Clear existing DB
    db.columns.clear();
    db.colNames.clear();
    db.colInfos.clear();
    db.numRows = 0;

    // 3. Schema
    for (uint32_t i = 0; i < numCols; ++i) {
      uint8_t type;
      uint16_t maxLen;
      uint8_t nameLen;

      file.read((char *)&type, sizeof(type));
      file.read((char *)&maxLen, sizeof(maxLen));
      file.read((char *)&nameLen, sizeof(nameLen));

      std::string name(nameLen, '\0');
      file.read(&name[0], nameLen);

      if (type == (uint8_t)INT_TYPE) {
        db.addIntColProp(name, maxLen);
      } else {
        db.addStrColProp(name, maxLen);
      }
    }

    // 4. Data Blocks
    db.numRows = numRows;

    for (uint32_t i = 0; i < numCols; ++i) {
      Column *col = db.columns[i].get();

      if (col->getType() == INT_TYPE) {
        // IntColumn: Simple Dump
        InternalIntColumn *intCol = static_cast<InternalIntColumn *>(col);
        size_t byteSize = numRows * sizeof(int);

        // Zero-Copy Check
        // We can't use generic resizeStorage anymore, must use spec method
        // But IntCol is a friend. Access private storage directly?
        // Yes, SnailStorage is friend of InternalIntColumn.

        intCol->storage.resize(numRows);
        if (byteSize > 0) {
          file.read((char *)intCol->storage.data(), byteSize);
        }
        intCol->sorted = false; // Reset safe
        intCol->index.clear();

      } else {
        // StrColumn: Dictionary Format
        // 1. Read Dict Size (uint16_t)
        InternalStrColumn *strCol = static_cast<InternalStrColumn *>(col);
        uint16_t dictSize = 0;
        file.read((char *)&dictSize, sizeof(dictSize));

        // 2. Read Dictionary
        strCol->dictionary.resize(dictSize);
        for (int k = 0; k < dictSize; ++k) {
          // Read Len + Bytes
          uint16_t strLen = 0; // Padded len should be maxLength?
          // To be safe/dynamic, let's read length.
          // Implementation choice: Did we write length or fixed?
          // Save logic: "Loop dictionary: Write string length + string bytes"
          // Yes, explicit length.
          file.read((char *)&strLen, sizeof(strLen));

          std::string s(strLen, '\0');
          file.read(&s[0], strLen);
          strCol->dictionary[k] = s;
        }

        // 3. Read Data Vector (uint16_t tokens)
        size_t dataSize = numRows * sizeof(uint16_t);
        strCol->data.resize(numRows);
        if (dataSize > 0) {
          file.read((char *)strCol->data.data(), dataSize);
        }

        strCol->sorted = false;
        strCol->index.clear();
      }

      if (!file)
        return false;
    }

    // Implicitly we might want to rebuild indices here or let user do it.
    // db.createIndex(); // Let's NOT force it, keep load fast. User can call if
    // needed.

    // Reset cursor
    db.reset();

    file.close();
    return true;
  }
};

#endif
