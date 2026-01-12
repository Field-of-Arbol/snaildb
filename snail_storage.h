#ifndef SNAIL_STORAGE_H
#define SNAIL_STORAGE_H

#include "snaildb.h"
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

class SnailStorage {
public:
  static bool save(const SnailDB &db, const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    // 1. Magic Header
    file.write("SNAL", 4);

    // 2. Size Info
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
    for (uint32_t i = 0; i < numCols; ++i) {
      Column *col = db.columns[i].get();
      
      if (col->getType() == INT_TYPE) {
        InternalIntColumn* intCol = static_cast<InternalIntColumn*>(col);
        size_t byteSize = intCol->storage.size() * sizeof(int);
        if (byteSize > 0) {
            file.write((const char *)intCol->storage.data(), byteSize);
        }
      } else {
        InternalStrColumn* strCol = static_cast<InternalStrColumn*>(col);
        
        // Save Dictionary
        uint16_t dictSize = (uint16_t)strCol->dictionary.size();
        file.write((const char*)&dictSize, sizeof(dictSize));
        
        for(const auto& s : strCol->dictionary) {
            uint16_t sLen = (uint16_t)s.length();
            file.write((const char*)&sLen, sizeof(sLen));
            file.write(s.c_str(), sLen);
        }

        // Save Tokens
        size_t dataSize = strCol->data.size() * sizeof(uint16_t);
        if (dataSize > 0) {
            file.write((const char *)strCol->data.data(), dataSize);
        }
      }
    }

    // 5. System Vectors (Fixed for v1.0)
    // Convert bool vector to byte vector for safe writing
    std::vector<uint8_t> activeBytes;
    activeBytes.reserve(db.numRows);
    for (bool b : db.activeRows) {
        activeBytes.push_back(b ? 1 : 0);
    }
    if (!activeBytes.empty()) {
        file.write((const char *)activeBytes.data(), activeBytes.size());
    }

    // Save Timestamps
    if (!db.timestamps.empty()) {
        file.write((const char *)db.timestamps.data(), db.timestamps.size() * sizeof(uint32_t));
    }

    return true;
  }

  static bool load(SnailDB &db, const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    char magic[4];
    file.read(magic, 4);
    if (strncmp(magic, "SNAL", 4) != 0) return false;

    uint32_t numRows, numCols;
    file.read((char *)&numRows, sizeof(numRows));
    file.read((char *)&numCols, sizeof(numCols));

    // Rebuild Schema (Clear existing)
    // In a real app, you might want to verify schema match instead of rebuilding
    // For this demo, we assume we are loading into an empty DB or overwriting
    
    // 3. Read Schema
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

    db.numRows = numRows;
    db.activeRows.clear();
    db.timestamps.clear();

    // 4. Load Data
    for (uint32_t i = 0; i < numCols; ++i) {
      Column *col = db.columns[i].get();

      if (col->getType() == INT_TYPE) {
        size_t byteSize = numRows * sizeof(int);
        col->resizeStorage(byteSize);
        if (byteSize > 0) file.read(col->getWritableBuffer(), byteSize);
      } else {
        InternalStrColumn *strCol = static_cast<InternalStrColumn *>(col);
        
        // Load Dictionary
        uint16_t dictSize = 0;
        file.read((char *)&dictSize, sizeof(dictSize));
        strCol->dictionary.resize(dictSize);
        
        for (int k = 0; k < dictSize; ++k) {
          uint16_t strLen = 0;
          file.read((char *)&strLen, sizeof(strLen));
          std::string s(strLen, '\0');
          file.read(&s[0], strLen);
          strCol->dictionary[k] = s;
        }

        // Load Tokens
        size_t byteSize = numRows * sizeof(uint16_t);
        col->resizeStorage(byteSize);
        if (byteSize > 0) file.read(col->getWritableBuffer(), byteSize);
      }
    }

    // 5. Load System Vectors
    if (file.peek() != EOF) {
        std::vector<uint8_t> activeBytes(numRows);
        file.read((char *)activeBytes.data(), numRows);
        
        db.activeRows.reserve(numRows);
        for(uint8_t b : activeBytes) {
            db.activeRows.push_back(b != 0);
        }

        db.timestamps.resize(numRows);
        file.read((char *)db.timestamps.data(), numRows * sizeof(uint32_t));
    } else {
        // Fallback for old files
        db.activeRows.assign(numRows, true);
        db.timestamps.assign(numRows, 0);
    }

    return true;
  }
};

#endif