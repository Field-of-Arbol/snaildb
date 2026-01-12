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
  // insertAt(ts, ...)
  db.insertAt(100, 1, "Alice", "Admin");
  db.insertAt(200, 2, "Bob", "User");
  db.insertAt(300, 3, "Charlie", "Guest");

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

  // --- Lifecycle Tests ---
  std::cout << "Testing Lifecycle..." << std::endl;
  std::cout << "Initial Active Count: " << db.getSize() << std::endl;

  // Soft Delete 'Zack' (Row 3, TS 400)
  db.softDelete(3);
  std::cout << "After Soft Delete (Zack), Count: " << db.getSize()
            << " (Expected 4)" << std::endl;
  assert(db.getSize() == 4);

  // Delete Older Than 150 (Aaron TS 50, Alice 100) -> Should delete Aaron and
  // Alice. Zack is already deleted. Remaining: Bob (200), Charlie (300).
  db.deleteOlderThan(150);
  std::cout << "After DeleteOlderThan(150), Count: " << db.getSize()
            << " (Expected 2)" << std::endl;
  SnailDumper::printTable(db);
  assert(db.getSize() == 2);

  // Persistence of Deletions
  std::cout << "Testing Persistence of Deletions..." << std::endl;
  if (SnailStorage::save(db, "test.snail")) {
    std::cout << "Saved to 'test.snail'." << std::endl;
  }

  SnailDB db2;
  // setup schema needed? Load usually rebuilds columns but we need to register
  // types if dynamic. Our Load reimplements schema from file.
  if (SnailStorage::load(db2, "test.snail")) {
    std::cout << "Loaded from 'test.snail'. Active Count: " << db2.getSize()
              << std::endl;
    assert(db2.getSize() == 2);
    SnailDumper::printTable(db2);
  } else {
    std::cerr << "Load Failed!" << std::endl;
    return 1;
  }

  // Physical Purge
  std::cout << "Testing Physical Purge..." << std::endl;
  db.purge();
  std::cout << "Purged! Active Count: " << db.getSize() << std::endl;
  assert(db.getSize() == 2);
  SnailDumper::printTable(db);

  std::cout << "Final Lifecycle Verification Passed!" << std::endl;

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
  // SnailDB db2; // db2 already declared and used above for lifecycle
  // persistence
  if (SnailStorage::load(db2, "test.snail")) {
    std::cout << "Loaded from 'test.snail'." << std::endl;

    // Verify Content
    assert(db2.getSize() == db.getSize());
    assert(db2.getColCount() == db.getColCount());

    // Verify Content (After Purge: Bob(0), Charlie(1))
    db2.reset();                                 // Cursor 0
    std::string name0 = db2.get<std::string>(1); // Name Col

    // Row 0 should be Bob
    if (name0.find("Bob") == std::string::npos) {
      std::cerr << "Persistence Mismatch Row 0: Expected Bob, got '" << name0
                << "'" << std::endl;
      return 1;
    }

    // Row 1 should be Charlie
    db2.next();
    std::string name1 = db2.get<std::string>(1);
    if (name1.find("Charlie") == std::string::npos) {
      std::cerr << "Persistence Mismatch Row 1: Expected Charlie, got '"
                << name1 << "'" << std::endl;
      return 1;
    }

    std::cout << "Persistence Verified! Rows match purged state." << std::endl;

  } else {
    std::cerr << "Failed to load!" << std::endl;
    return 1;
  }

  std::cout << "Final Merge & Purge Verification Passed!" << std::endl;
  return 0;
}
