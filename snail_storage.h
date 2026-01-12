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
      const Column *col = db.columns[i].get();
      size_t size = col->getByteSize();
      const char *data = col->getRawData();

      // Write block size first (safety) or imply from Row*Len?
      // Better to write raw dump. We can calc size from Metadata on load.
      // But writing size provides sanity check. Let's write raw only to be
      // compact. LOAD logic:
      //   IntCol size = numRows * 4
      //   StrCol size = numRows * maxLen
      // So we don't strictly need to write size, but let's do it for
      // validation. Actually, requirements said "compact". Implicit size is
      // most compact. We trust numRows * Config.

      file.write(data, size);
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
      size_t byteSize = 0;

      if (col->getType() == INT_TYPE) {
        byteSize = numRows * sizeof(int);
      } else {
        // Str Type
        byteSize = numRows * db.colInfos[i].max_length;
      }

      // Zero-Copy Load
      // 1. Resize Column Storage directly
      col->resizeStorage(byteSize);

      // 2. Read directly into Column's memory
      if (byteSize > 0) {
        file.read(col->getWritableBuffer(), byteSize);
        if (!file) {
          // Truncated file or error
          return false;
        }
      }

      // Note: resizeStorage marks sorted=false by default for safety.
      // If we wanted to restore sorted state, we'd need to check it here or
      // save it in metadata. For v0.8 simplicity, loading raw marks unsorted.
      // User can manually check or we improve later.
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
