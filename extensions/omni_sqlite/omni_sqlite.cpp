/*
 * This work is inspired by postgres-sqlite by Michel Pelletier, licensed under BSD 3-Clause
 * License, but is a significant rewrite in a different language (C -> C++).
 */

#include "sqlite.hpp"

#include <ranges>

extern "C" {
PG_MODULE_MAGIC;
}

postgres_function(sqlite_in, ([](const char *query) -> cppgres::expanded_varlena<sqlite> {
                    cppgres::expanded_varlena<sqlite> db;
                    sqlite sql = db;
                    char *msg = nullptr;
                    if (sqlite3_exec(sql, query, nullptr, nullptr, &msg) != SQLITE_OK) {
                      throw std::runtime_error(
                          std::format("Failed to create SQLite in-memory database: {}", msg));
                    }

                    return db;
                  }));

postgres_function(
    sqlite_out, ([](cppgres::expanded_varlena<sqlite> db) -> const char * {
      sqlite &sql = db;
      using string_type =
          std::basic_string<char, std::char_traits<char>, cppgres::memory_context_allocator<char>>;
      auto allocator = cppgres::memory_context_allocator<char>(cppgres::memory_context(), true);
      string_type s(allocator);

      int rc;
      if ((rc = sqlite3_db_dump(
               sql, "main", nullptr,
               [](const char *str, void *sinfo) {
                 auto s = reinterpret_cast<string_type *>(sinfo);
                 s->append(str);
                 return 1;
               },
               reinterpret_cast<void *>(&s))) != SQLITE_OK) {
        throw std::runtime_error(std::format("Failed to dump SQLite: {}", sqlite3_errstr(rc)));
      }

      const char *str = s.c_str();
      return str;
    }));

postgres_function(
    sqlite_deserialize, ([](const cppgres::byte_array input) {
      cppgres::expanded_varlena<sqlite> db;
      sqlite &sql = db;

      std::byte *sqlite3_alloc =
          reinterpret_cast<std::byte *>(sqlite3_malloc64(input.size_bytes()));
      std::copy(input.begin(), input.end(), sqlite3_alloc);
      if (sqlite3_deserialize(sql, "main", reinterpret_cast<unsigned char *>(sqlite3_alloc),
                              input.size_bytes(), input.size_bytes(),
                              SQLITE_DESERIALIZE_RESIZEABLE | SQLITE_DESERIALIZE_FREEONCLOSE) !=
          SQLITE_OK) {
        throw std::runtime_error(
            std::format("Could not deserialize SQLite database: {}", sqlite3_errmsg(sql)));
      }

      return db;
    }));

postgres_function(sqlite_exec, ([](cppgres::expanded_varlena<sqlite> db, std::string_view query) {
                    char *msg = NULL;
                    sqlite &sql = db;

                    if (sqlite3_exec(sql, std::string(query).c_str(), nullptr, nullptr, &msg) !=
                        SQLITE_OK) {
                      throw std::runtime_error(std::format("Failed to execute query: {}", msg));
                    }
                    return db;
                  }));

#if __has_include(<generator>)
#include <generator>
#else
#include "__generator.hpp"
#endif

std::generator<cppgres::record> query_results(int sqlite_rc, int column_count, sqlite3_stmt *stmt,
                                              cppgres::tuple_descriptor td) {
  if (sqlite_rc == SQLITE_ROW) {
    do {
      std::vector<cppgres::nullable_datum> datums;
      for (int i = 0; i < column_count; i++) {
        auto value = sqlite3_column_value(stmt, i);
        auto value_type = sqlite3_value_type(value);
        switch (value_type) {
        case SQLITE_INTEGER:
          datums.emplace_back(cppgres::into_nullable_datum(sqlite3_column_int64(stmt, i)));
          break;
        case SQLITE_FLOAT:
          datums.emplace_back(cppgres::into_nullable_datum(sqlite3_column_double(stmt, i)));
          break;
        case SQLITE_BLOB:
          datums.emplace_back(cppgres::into_nullable_datum(cppgres::byte_array(
              reinterpret_cast<std::byte *>(const_cast<void *>(sqlite3_column_blob(stmt, i))),
              sqlite3_column_bytes(stmt, i))));
          break;
        case SQLITE_TEXT:
          datums.emplace_back(cppgres::into_nullable_datum(std::string_view(
              reinterpret_cast<char *>(const_cast<unsigned char *>(sqlite3_column_text(stmt, i))),
              sqlite3_column_bytes(stmt, i))));
          break;
        case SQLITE_NULL:
          datums.emplace_back(cppgres::nullable_datum());
          break;
        }
      }

      cppgres::record rec(td, datums.begin(), datums.end());
      co_yield rec;
    } while (sqlite3_step(stmt) == SQLITE_ROW);
  }

  sqlite3_finalize(stmt);
}

static_assert(cppgres::datumable_iterator<std::generator<cppgres::record>>);

postgres_function(sqlite_query, ([](cppgres::expanded_varlena<sqlite> db, std::string_view query) {
                    sqlite &sql = db;
                    sqlite3_stmt *stmt;
                    if (sqlite3_prepare_v2(sql, query.data(), query.size(), &stmt, nullptr) !=
                        SQLITE_OK) {
                      throw std::runtime_error(std::format("Failed to prepare SQLite query: {}", sqlite3_errmsg(sql)));
                    }
                    auto column_count = sqlite3_column_count(stmt);
                    cppgres::tuple_descriptor td(column_count);

                    int rc = sqlite3_step(stmt);
                    if (rc == SQLITE_ROW || rc == SQLITE_DONE) {

                      for (int i = 0; i < column_count; i++) {
                        auto column_type = sqlite3_column_type(stmt, i);
                        auto column_name = sqlite3_column_name(stmt, i);
                        td.set_name(i, cppgres::name(column_name));
                        switch (column_type) {
                        case SQLITE_INTEGER:
                          td.set_type(i, cppgres::type{.oid = INT8OID});
                          break;
                        case SQLITE_FLOAT:
                          td.set_type(i, cppgres::type{.oid = FLOAT8OID});
                          break;
                        case SQLITE_BLOB:
                          td.set_type(i, cppgres::type{.oid = BYTEAOID});
                          break;
                        case SQLITE_TEXT:
                          td.set_type(i, cppgres::type{.oid = TEXTOID});
                          break;
                        case SQLITE_NULL:
                          td.set_type(i, cppgres::type{.oid = ANYOID});
                          break;
                        }
                      }
                    }

                    return query_results(rc, column_count, stmt, std::move(td));
                  }));

postgres_function(sqlite_serialize, ([](cppgres::expanded_varlena<sqlite> db) {
                    void *data;
                    int64_t size;

                    data = sqlite3_serialize(db.operator sqlite &(), "main", &size, 0);
                    if (data == nullptr) {
                      throw std::runtime_error(
                          std::format("Failed to serialize SQLite db: {}",
                                                           sqlite3_errmsg(db.operator sqlite &())));
                    }
                    cppgres::byte_array barr(reinterpret_cast<std::byte *>(data), size);
                    auto arr = cppgres::bytea(barr, cppgres::memory_context());
                    sqlite3_free(data);
                    return arr;
                  }));
