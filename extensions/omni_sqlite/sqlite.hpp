/*
 * This work is inspired by postgres-sqlite by Michel Pelletier, licensed under BSD 3-Clause
 * License, but is a significant rewrite in a different language (C -> C++).
 */

#ifndef OMNI_SQLITE_H
#define OMNI_SQLITE_H

extern "C" {
#include <sqlite3.h>
}

#ifdef __cplusplus

#include <cppgres.hpp>

struct sqlite {

  sqlite();

  operator sqlite3 *() const;

  std::size_t flat_size();

  void flatten_into(std::span<std::byte> buffer);
  static cppgres::type type();

  static sqlite restore_from(std::span<std::byte> buffer);

private:
  std::shared_ptr<sqlite3> db;
  sqlite3_int64 _flat_size = 0;
};
#endif

extern "C" {
int sqlite3_db_dump(sqlite3 *db, const char *zSchema, const char *zTable,
                    int (*xCallback)(const char *, void *), void *pArg);
}

#endif /* OMNI_SQLITE_H */
