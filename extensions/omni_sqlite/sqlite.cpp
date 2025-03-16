#include "sqlite.hpp"

std::size_t sqlite::flat_size() {
  /* Use cached value if already computed */
  if (_flat_size) {
    return _flat_size;
  }

  unsigned char *v = sqlite3_serialize(db.get(), "main", &_flat_size, 0);
  if (v == nullptr) {
    throw std::runtime_error(
        std::format("Failed to serialize SQLite: {}", sqlite3_errmsg(db.get())));
  }

  return static_cast<std::size_t>(_flat_size);
}

void sqlite::flatten_into(std::span<std::byte> buffer) {
  auto ptr = sqlite3_serialize(db.get(), "main", &_flat_size, 0);
  if (ptr == nullptr) {
    throw std::runtime_error(
        std::format("Failed to serialize SQLite: {}", sqlite3_errmsg(db.get())));
  }
  std::span<std::byte> bytes(reinterpret_cast<std::byte *>(ptr),
                             static_cast<std::size_t>(_flat_size));
  std::copy(bytes.begin(), bytes.end(), buffer.begin());
}

cppgres::type sqlite::type() { return cppgres::named_type("omni_sqlite", "sqlite"); }

sqlite sqlite::restore_from(std::span<std::byte> buffer) {
  sqlite sql;
  std::byte *sqlite3_alloc = reinterpret_cast<std::byte *>(sqlite3_malloc64(buffer.size_bytes()));
  std::copy(buffer.begin(), buffer.end(), sqlite3_alloc);
  if (sqlite3_deserialize(sql, "main", reinterpret_cast<unsigned char *>(sqlite3_alloc),
                          buffer.size_bytes(), buffer.size_bytes(),
                          SQLITE_DESERIALIZE_RESIZEABLE | SQLITE_DESERIALIZE_FREEONCLOSE) !=
      SQLITE_OK) {
    throw std::runtime_error(std::format("can't deserialize SQLite: {}", sqlite3_errmsg(sql)));
  }
  return sql;
}

sqlite::sqlite()
    : db([]() {
        sqlite3 *db;
        if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
          throw std::runtime_error(
              std::format("can't create a new SQLite database: {}", sqlite3_errmsg(db)));
        }
        return std::shared_ptr<sqlite3>(db, sqlite3_close);
      }()) {}

sqlite::operator sqlite3 *() const { return db.get(); }
