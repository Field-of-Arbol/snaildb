# 🐌 SnailDB

**High-Performance Columnar Database for Embedded Systems (ESP32, RP2040)**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-ESP32%20%7C%20ESP8266%20%7C%20RP2040-blue)](https://www.arduino.cc/)

SnailDB is a lightweight, in-memory **Columnar Database** designed specifically for 32-bit microcontrollers. Unlike traditional row-based approaches (or simple CSV logging), SnailDB uses column-oriented storage with **Dictionary Compression**, making it incredibly RAM-efficient for repetitive IoT data.

> **Note:** Inspired by Pandas and InterSystems IRIS, but built for the constraints of embedded hardware.

## 🚀 Key Features (v1.0)

* **💾 Columnar Storage:** Data is stored in contiguous arrays, optimizing cache usage and memory alignment.
* **📦 Dictionary Compression:** String columns automatically use dictionary encoding (Tokenization). Storing "Temperature_Sensor_Error" 1,000 times consumes only ~2KB, not ~24KB.
* **⚡ O(log n) Search:** Automatic Binary Search on sorted columns and Indexing support for unsorted data.
* **🛡️ Embedded Safe:**
    * **No Exceptions:** Uses return codes and checks safe for generic C++ compilers.
    * **Zero-Copy Persistence:** Loads data from Flash/SD directly into memory buffers to prevent RAM spikes.
    * **Crash Proof:** Binary format serialization ensures data integrity.
* **♻️ Lifecycle Management:** Support for **Soft Deletes** (logical removal) and **Purge** (physical memory compaction) based on timestamps.

## 📊 Performance Strategy

| Feature | SnailDB approach | Benefit |
| :--- | :--- | :--- |
| **Storage** | Column-Vector (`std::vector`) | Low overhead, fast iteration. |
| **Strings** | Dictionary Encoding (`uint16_t` tokens) | **80-90% RAM reduction** on repetitive logs. |
| **Search** | `std::lower_bound` + Hash Index | Instant lookups, no linear scanning. |
| **Persistence**| Raw Binary Dump (`.snail`) | Minimal file size, fastest load time. |

## 🛠️ Installation

### PlatformIO
(Instructions coming soon via Registry) - For now, clone into `lib/`.

### Arduino IDE
1. Download this repository as a `.zip`.
2. Go to **Sketch** -> **Include Library** -> **Add .ZIP Library**.

## 📖 Quick Start

### 1. Basic Setup & Insertion

```cpp
#include "snaildb.h"
#include "snail_dumper.h" // For debugging print

SnailDB db;

void setup() {
    Serial.begin(115200);

    // 1. Define Schema
    // (Column Name, Max Display Length for Str)
    db.addIntColProp("id", 0);
    db.addStrColProp("sensor", 10);
    db.addStrColProp("status", 8);

    // 2. Pre-allocate memory (optional, avoids fragmentation)
    db.reserve(100);

    // 3. Insert Data (Variadic API)
    // Format: insert(Args...) matching schema order
    db.insert(1, "Temp_Sensor_1", "OK");
    db.insert(2, "Temp_Sensor_2", "WARNING");
    db.insert(3, "Door_Sensor_A", "OPEN");

    // 4. Print Table
    SnailDumper::printTable(db, Serial);
}

```

### 2. Searching & Data Access

```cpp
void loop() {
    // Fast Search (Binary Search if sorted, or Index lookup)
    int rowIndex = db.findRow("sensor", "Temp_Sensor_2");

    if (rowIndex != -1) {
        // Typed Access (Safe)
        std::string status = db.get<std::string>(2, rowIndex); // Col 2 is 'status'
        Serial.print("Status found: ");
        Serial.println(status.c_str());
    }
}

```

### 3. Persistence (Save/Load)

```cpp
#include "snail_storage.h"

// Save database to LittleFS or SD
if (SnailStorage::save(db, "/log_v1.snail")) {
    Serial.println("Database saved!");
}

// Load database (Restores schema and data)
SnailDB db2;
if (SnailStorage::load(db2, "/log_v1.snail")) {
    Serial.println("Database loaded!");
}

```

### 4. Lifecycle (Cleaning Data)

```cpp
// Mark rows older than specific timestamp as deleted
// (Assumes you manage a timestamp column or use the internal system time)
uint32_t now = millis();
db.insertAt(now, 4, "System", "BOOT");

// Soft delete
db.softDelete(0); 

// Hard cleanup (Physical memory compaction)
// Moves valid data to fill holes, then resizes vectors.
db.purge(); 

```

## ⚠️ Requirements & Limitations

* **Architecture:** 32-bit Microcontrollers recommended (ESP32, RP2040, STM32).
* **RAM:** This is an **In-Memory Database**. Your dataset must fit in available RAM.
* *Not compatible* with Arduino Uno/Nano (AVR) due to extremely low RAM (2KB).


* **Filesystem:** Requires a filesystem (LittleFS, SPIFFS, SdFat) implementation for persistence.

## 🤝 Contributing

Contributions are welcome! Please check the `issues` tab for roadmap items like SQL-parser support or multi-table joins.

## 📄 License

MIT License. See `LICENSE` file for details.
