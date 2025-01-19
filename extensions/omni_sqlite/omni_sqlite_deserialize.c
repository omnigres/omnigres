/*
This work is derived from postgres-sqlite, please see the license terms below:

BSD 3-Clause License

Copyright (c) 2025, Michel Pelletier

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "omni_sqlite.h"

PG_FUNCTION_INFO_V1(sqlite_deserialize);

Datum sqlite_deserialize(PG_FUNCTION_ARGS) {
  sqlite_Sqlite *sqlite;
  sqlite3 *db;
  bytea *input;
  char *data;
  size_t size;

  input = PG_GETARG_BYTEA_P(0);
  data = VARDATA(input);
  size = VARSIZE(input) - VARHDRSZ;

  if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
    ereport(ERROR, (errmsg("Failed to create SQLite in-memory database: %s", sqlite3_errmsg(db))));
  }

  if (sqlite3_deserialize(db, "main", (unsigned char *)data, size, size,
                          SQLITE_DESERIALIZE_RESIZEABLE) != SQLITE_OK) {
    ereport(ERROR, errmsg("could not deserialize SQLite database: %s", sqlite3_errmsg(db)));
  }

  sqlite = new_expanded_sqlite(NULL, CurrentMemoryContext, db);
  SQLITE_RETURN(sqlite);
}