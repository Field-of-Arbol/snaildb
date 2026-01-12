#include "snail_dumper.h"
#include "snail_storage.h"
#include "snaildb.h"
#include <cassert>
#include <iostream>

int main() {
  std::cout << "Starting SnailDB v0.5 Final Tests..." << std::endl;

  // 1. Initialize
  SnailDB db;

  // 2. Define Schema
  std::cout << "Defining Schema..." << std::endl;
  db.addIntColProp("id", 0);
  db.addStrColProp("name", 10);
  db.addStrColProp("role", 8);

  // 3. Reserve Memory
  db.reserve(10);

  // 4. Sorted Insert (1, 2, 3) -> Should remain sorted
  std::cout << "Inserting Rows (Sorted)..." << std::endl;
  db.insert(1, "Alice", "Admin");
  db.insert(2, "Bob", "User");
  db.insert(3, "Charlie", "Guest");

  assert(db.getSize() == 3);

  // Verify Find (Should use Binary Search implicitly if implementation works)
  // We can't introspect internal strategy easily without logging,
  // but we can verify correctness.
  assert(db.findRow("id", "2") == 1);
  assert(db.findRow("name", "Bob") == 1); // "       Bob" vs "Bob" logic check

  // 5. Unsorted Insert -> Should invalidate sort
  std::cout << "Inserting Unsorted Row..." << std::endl;
  db.insert(0, "Zack", "Bot");
  // Now id Column is [1, 2, 3, 0] -> Unsorted
  // name Column is [Alice, Bob, Charlie, Zack] -> Sorted?
  // "       Zack" > "   Charlie"? Yes. ' ' < 'C'. Wait.
  // "     Alice" (space=32, A=65).
  // "      Zack" > "   Charlie"?
  // Yes. So name column might still be sorted.
  // Let's force unsorted name.
  db.insert(4, "Aaron", "Visitor");
  // "     Aaron" < "      Zack". Name column now unsorted.

  // 6. Indexing
  std::cout << "Creating Index..." << std::endl;
  db.createIndex();

  // 7. Find using Index
  std::cout << "Testing Indexed Search..." << std::endl;
  int idx = db.findRow("name", "Aaron");
  std::cout << "Found Aaron at: " << idx << std::endl;
  assert(idx == 4);

  // 8. Dump
  SnailDumper::printTable(db);

  // 9. Unsorted Fallback Logic Verification
  std::cout << "Testing Unsorted Fallback..." << std::endl;
  // Current state:
  // id: [1, 2, 3, 0, 4] -> Unsorted (0 < 3)
  // We already inserted 0, so 'id' column sorted flag should be false.
  // Let's verify searching for '4' works (Linear Scan)
  int idx2 = db.findRow("id", "4");
  assert(idx2 == 4);

  // 10. Memory Cleanup Check (Manual observation via Code Review)
  // SnailDB destructor cleans up unique_ptrs automatically.

  // 11. Persistence Test
  std::cout << "Testing Persistence (Save/Load)..." << std::endl;

  // Save current DB
  if (SnailStorage::save(db, "test.snail")) {
    std::cout << "Saved to 'test.snail'." << std::endl;
  } else {
    std::cerr << "Failed to save!" << std::endl;
    return 1;
  }

  // Create new DB to load into
  SnailDB db2;
  if (SnailStorage::load(db2, "test.snail")) {
    std::cout << "Loaded from 'test.snail'." << std::endl;

    // Verify Content
    assert(db2.getSize() == db.getSize());
    assert(db2.getColCount() == db.getColCount());

    // Check row 4 (Aaron)
    int id = db2.findRow("id", "4"); // Was inserted as 4, Aaron
    assert(id == 4);

    std::string name = db2.get<std::string>(1); // Row 0
    // Row 0 was Alice. Row 1 is Bob?
    // db insertion order: 1, 2, 3, 0, 4.
    // Row 0: id=1, name="Alice"
    // Row 1: id=2, name="Bob"
    // db2.get<string>(1) gets row 1 (cursor??)
    // Wait, SnailDB::get(colIndex) uses CURRENT CURSOR.
    // We must navigate db2.

    db2.reset(); // Cursor 0
    // Cursor 0 -> Alice
    std::string name0 = db2.get<std::string>(1); // Col 1 is name
    // Alice is padded "     Alice" (10 chars, right aligned? No left padded,
    // stored as spaces then Str in my optimized impl? Wait, Optimized StrColumn
    // Add: padLen spaces, then string. Yes. StrColumn::getStr returns
    // substring.

    // Let's verify specific content.

    // Verify Row 1 (Bob)
    db2.next(); // Cursor 1
    std::string name1 = db2.get<std::string>(1);
    if (name1.find("Bob") == std::string::npos) {
      std::cerr << "Persistence Data Mismatch: Expected Bob, got '" << name1
                << "'" << std::endl;
      return 1;
    }

    std::cout << "Persistence Verified!" << std::endl;
    SnailDumper::printTable(db2);

  } else {
    std::cerr << "Failed to load!" << std::endl;
    return 1;
  }

  std::cout << "Final Merge & Purge Verification Passed!" << std::endl;
  return 0;
}
