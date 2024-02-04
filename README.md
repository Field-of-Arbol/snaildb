# 🐌 snaildb

SnailDB is a lightweight, in-progress C++ project inspired by Python's pandas library and InterSystems Caché. The primary goal is to create a simple, fun-to-use database-like functionality for ESP32, although it's still a work in progress.

## Purpose

This project aims to provide a basic database structure for ESP32, making it easy and enjoyable for developers to work with data.

## Getting Started

### Prerequisites

Make sure you have a C++ compiler and ESP32 development environment set up.

## Usage

To start using SnailDB, follow these steps:

1. Initialize SnailDB:
```
     #include "snaildb.h"

   SnailDB snailDB;
   ```
2. Add Columns:
```
     snailDB.addStrColProp("Name", 10);
   snailDB.addIntColProp("Age", 3);
```   
3. Add Rows:
```
     std::vector<SnailDataType*> rowData = {
       new StrCol("John"),
       new IntCol(25)
   };

   snailDB.addRow(rowData);
   ```
4. Get Row:
```
     std::string row = snailDB.getRow();
   std::cout << row << std::endl;
   ```
   This will print the current row.

### Important Note

SnailDB is still in development, and its main purpose is for fun and experimentation. It is inspired by pandas and Intersystems Caché but is not intended for production use.

Feel free to contribute, provide feedback, or share your thoughts!
