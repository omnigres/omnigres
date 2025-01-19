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

#ifndef OMNI_SQLITE_H
#define OMNI_SQLITE_H

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include "funcapi.h"
#include "lib/stringinfo.h"
#include "utils/builtins.h"
#include "utils/expandeddatum.h"
#include "utils/lsyscache.h"

#include <sqlite3.h>

/* ID for debugging crosschecks */
#define sqlite_MAGIC 889276513

/* Flattened representation of sqlite, used to store to disk.

   The first 32 bits must the length of the data.  Actual flattened data
   is appended after this struct and cannot exceed 1GB.
*/
typedef struct sqlite_FlatSqlite {
  int32 vl_len_;
} sqlite_FlatSqlite;

/* Expanded representation of sqlite.

   When loaded from storage, the flattened representation is used to
   build the sqlite.  In this case, it's just a pointer to an integer.
*/
typedef struct sqlite_Sqlite {
  ExpandedObjectHeader hdr;
  int em_magic;
  sqlite3 *db;
  Size flat_size;
  unsigned char *flat_data;
} sqlite_Sqlite;

/* Create a new sqlite datum. */
sqlite_Sqlite *new_expanded_sqlite(sqlite_FlatSqlite *flat, MemoryContext parentcontext,
                                   sqlite3 *existing_db);

int sqlite3_db_dump(sqlite3 *db, const char *zSchema, const char *zTable,
                    int (*xCallback)(const char *, void *), void *pArg);

/* Helper function that either detoasts or expands. */
sqlite_Sqlite *DatumGetSqlite(Datum d);

/* Helper macro to detoast and expand sqlites arguments */
#define SQLITE_GETARG(n) DatumGetSqlite(PG_GETARG_DATUM(n))

/* Helper macro to return Expanded Object Header Pointer from sqlite. */
#define SQLITE_RETURN(A) return EOHPGetRWDatum(&(A)->hdr)

/* Helper macro to compute flat sqlite header size */
#define SQLITE_OVERHEAD() MAXALIGN(sizeof(sqlite_FlatSqlite))

/* Helper macro to get pointer to beginning of sqlite data. */
#define SQLITE_DATA(a) (((unsigned char *)(a)) + SQLITE_OVERHEAD())

/* Help macro to cast generic Datum header pointer to expanded Sqlite */
#define SqliteGetEOHP(d) (sqlite_Sqlite *)DatumGetEOHP(d);

/* Public API functions */

#endif /* OMNI_SQLITE_H */