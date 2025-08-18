/*
 * This work is inspired by postgres-sqlite by Michel Pelletier, licensed under BSD 3-Clause
 * License, but is a significant rewrite in a different language (C -> C++).
 */

#include "sqlite.hpp"

#include <ranges>

extern "C" {
PG_MODULE_MAGIC;

#include <mb/pg_wchar.h>
#include <utils/lsyscache.h>
}

postgres_function(sqlite_in, ([](const char *query) -> cppgres::expanded_varlena<sqlite> {
                    cppgres::expanded_varlena<sqlite> db;
                    sqlite sql = db;
                    char *msg = nullptr;

                    if (sqlite3_exec(sql, query, nullptr, nullptr, &msg) != SQLITE_OK) {
                      throw std::runtime_error(cppgres::fmt::format(
                          "Failed to create SQLite in-memory database: {}", msg));
                    }

                    return db;
                  }));

postgres_function(sqlite_out, ([](cppgres::expanded_varlena<sqlite> db) -> const char * {
                    sqlite &sql = db;
                    std::string s;

                    int rc;
                    if ((rc = sqlite3_db_dump(
                             sql, "main", nullptr,
                             [](const char *str, void *sinfo) {
                               auto s = reinterpret_cast<std::string *>(sinfo);
                               s->append(str);
                               return 1;
                             },
                             reinterpret_cast<void *>(&s))) != SQLITE_OK) {
                      throw std::runtime_error(
                          cppgres::fmt::format("Failed to dump SQLite: {}", sqlite3_errstr(rc)));
                    }

                    auto allocator =
                        cppgres::memory_context_allocator<char>(cppgres::memory_context(), true);
                    char *str = new (allocator.allocate(s.size() + 1)) char[s.size() + 1];
                    std::copy(s.c_str(), s.c_str() + s.size() + 1, str);
                    return str;
                  }));

postgres_function(sqlite_deserialize, ([](const cppgres::byte_array input) {
                    cppgres::expanded_varlena<sqlite> db;
                    sqlite &sql = db;

                    std::byte *sqlite3_alloc =
                        reinterpret_cast<std::byte *>(sqlite3_malloc64(input.size_bytes()));
                    std::copy(input.begin(), input.end(), sqlite3_alloc);
                    if (sqlite3_deserialize(sql, "main",
                                            reinterpret_cast<unsigned char *>(sqlite3_alloc),
                                            input.size_bytes(), input.size_bytes(),
                                            SQLITE_DESERIALIZE_RESIZEABLE |
                                                SQLITE_DESERIALIZE_FREEONCLOSE) != SQLITE_OK) {
                      throw std::runtime_error(cppgres::fmt::format(
                          "Could not deserialize SQLite database: {}", sqlite3_errmsg(sql)));
                    }

                    return db;
                  }));

static void bind_params(sqlite3_stmt *stmt, std::optional<cppgres::record> params) {
  if (params.has_value()) {
    auto p = params.value();
    for (int i = 0; i < p.attributes(); i++) {
      auto nd = p.get_attribute(i);
      if (nd.is_null()) {
        sqlite3_bind_null(stmt, i + 1);
      } else {
        auto typoid = p.attribute_type(i).oid;
        switch (typoid) {
        case INT2OID:
        case INT4OID:
        case INT8OID:
          sqlite3_bind_int64(
              stmt, i + 1,
              cppgres::datum_conversion<int64_t>::from_datum(nd, typoid, std::nullopt));
          break;
        case FLOAT4OID:
        case FLOAT8OID:
          sqlite3_bind_double(
              stmt, i + 1, cppgres::datum_conversion<double>::from_datum(nd, typoid, std::nullopt));
          break;
        case BYTEAOID: {
          auto ba =
              cppgres::datum_conversion<cppgres::byte_array>::from_datum(nd, typoid, std::nullopt);
          sqlite3_bind_blob64(stmt, i + 1, ba.data(), ba.size_bytes(), nullptr);
          break;
        }
        case TEXTOID: {
          auto str =
              cppgres::datum_conversion<std::string_view>::from_datum(nd, typoid, std::nullopt);
          auto encoding = ::GetDatabaseEncoding();

          if (encoding != PG_UTF8) {
            unsigned char *utf8_str = cppgres::ffi_guard{::pg_do_encoding_conversion}(
                reinterpret_cast<unsigned char *>(const_cast<char *>(str.data())), str.length(),
                encoding, PG_UTF8);
            if (utf8_str != nullptr) {
              str = std::string_view(reinterpret_cast<char *>(utf8_str));
            }
          }
          sqlite3_bind_text64(stmt, i + 1, str.data(), str.size(), nullptr, SQLITE_UTF8);
          break;
        }
        case UNKNOWNOID: {
          bool is_varlena;
          cppgres::oid outfun(InvalidOid);
          cppgres::ffi_guard{::getTypeOutputInfo}(typoid, &outfun.operator ::Oid &(), &is_varlena);
          auto fc = cppgres::current_postgres_function::call_info();
          auto d = cppgres::ffi_guard{::OidFunctionCall1Coll}(outfun, (*fc).collation(), nd);
          auto cc = cppgres::datum_conversion<const char *>::from_datum(cppgres::datum(d), TEXTOID,
                                                                        std::nullopt);
          sqlite3_bind_text64(stmt, i + 1, cc, ::strlen(cc), nullptr, SQLITE_UTF8);
          break;
        }
        }
      }
    }
  }
}

postgres_function(
    sqlite_exec, ([](cppgres::expanded_varlena<sqlite> db, std::string_view query,
                     std::optional<cppgres::record> params) {
      char *msg = NULL;
      sqlite &sql = db;

      sqlite3_stmt *stmt;
      if (sqlite3_prepare_v2(sql, query.data(), query.size(), &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(
            cppgres::fmt::format("Failed to prepare SQLite query: {}", sqlite3_errmsg(sql)));
      }

      bind_params(stmt, std::move(params));

      int rc;
      do {
        CHECK_FOR_INTERRUPTS();
        rc = sqlite3_step(stmt);
      } while (rc == SQLITE_ROW);
      if (rc != SQLITE_DONE) {
        throw std::runtime_error(cppgres::fmt::format("Failed to execute query: {}", msg));
      }
      return db;
    }));

#if __has_include(<generator>)
#include <generator>
#endif
#if !defined(__cpp_lib_generator) || (__cpp_lib_generator < 202107L)
#include "__generator.hpp"
#endif

std::generator<cppgres::record> query_results(int sqlite_rc, std::vector<int> types,
                                              sqlite3_stmt *stmt, cppgres::tuple_descriptor td) {
  if (sqlite_rc == SQLITE_ROW) {
    do {
      CHECK_FOR_INTERRUPTS();
      std::vector<cppgres::nullable_datum> datums;
      for (int i = 0; i < types.size(); i++) {
        auto value = sqlite3_column_value(stmt, i);
        auto value_type = sqlite3_value_type(value);
        if (value_type != SQLITE_NULL && value_type != types[i]) {
          auto type_printer = [](int type) {
            switch (type) {
            case SQLITE_INTEGER:
              return "integer";
            case SQLITE_FLOAT:
              return "float";
            case SQLITE_BLOB:
              return "blob";
            case SQLITE_TEXT:
              return "text";
            case SQLITE_NULL:
              return "null";
            default:
              return "unknown";
            }
          };
          throw std::runtime_error(
              cppgres::fmt::format("column {} type mismatch, expected {}, got {}", i,
                                   type_printer(types[i]), type_printer(value_type)));
        }
        switch (value_type) {
        case SQLITE_INTEGER:
          datums.emplace_back(
              cppgres::into_nullable_datum(static_cast<int64_t>(sqlite3_column_int64(stmt, i))));
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

postgres_function(sqlite_query, ([](cppgres::expanded_varlena<sqlite> db, std::string_view query,
                                    std::optional<cppgres::record> params) {
                    sqlite &sql = db;
                    sqlite3_stmt *stmt;
                    if (sqlite3_prepare_v2(sql, query.data(), query.size(), &stmt, nullptr) !=
                        SQLITE_OK) {
                      throw std::runtime_error(cppgres::fmt::format(
                          "Failed to prepare SQLite query: {}", sqlite3_errmsg(sql)));
                    }

                    bind_params(stmt, std::move(params));

                    auto column_count = sqlite3_column_count(stmt);
                    cppgres::tuple_descriptor td(column_count);

                    int rc = sqlite3_step(stmt);
                    std::vector<int> column_types;

                    if (rc == SQLITE_ROW || rc == SQLITE_DONE) {

                      for (int i = 0; i < column_count; i++) {
                        auto column_type = sqlite3_column_type(stmt, i);
                        column_types.push_back(column_type);
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

                    return query_results(rc, std::move(column_types), stmt, std::move(td));
                  }));

postgres_function(sqlite_serialize, ([](cppgres::expanded_varlena<sqlite> db) {
                    void *data;
                    sqlite3_int64 size;

                    data = sqlite3_serialize(db.operator sqlite &(), "main", &size, 0);
                    if (data == nullptr) {
                      throw std::runtime_error(
                          cppgres::fmt::format("Failed to serialize SQLite db: {}",
                                               sqlite3_errmsg(db.operator sqlite &())));
                    }
                    cppgres::byte_array barr(reinterpret_cast<std::byte *>(data), size);
                    auto arr = cppgres::bytea(barr, cppgres::memory_context());
                    sqlite3_free(data);
                    return arr;
                  }));
