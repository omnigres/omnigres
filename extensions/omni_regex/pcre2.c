/**
 * \file pcre2.c
 *
 * The code below is a fork of [pgpcre](https://github.com/petere/pgpcre) by
 * Peter Eisentraut with [unmerged] contributions from Christoph Berg (pcre2 support),
 * modified to support named capture groups, parallelization and PCRE2.
 *
 *
 * The original code is licensed under the terms of The PostgreSQL License reproduced below:
 *
 * Copyright © 2013, Peter Eisentraut <peter@eisentraut.org>
 *
 * (The PostgreSQL License)
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written
 * agreement is hereby granted, provided that the above copyright notice
 * and this paragraph and the following two paragraphs appear in all
 * copies.
 *
 * IN NO EVENT SHALL THE AUTHOR(S) OR ANY CONTRIBUTOR(S) BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF THIS
 * SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE AUTHOR(S) OR
 * CONTRIBUTOR(S) HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHOR(S) AND CONTRIBUTOR(S) SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE AUTHOR(S) AND
 * CONTRIBUTOR(S) HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT,
 * UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */
// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <common/hashfn.h>
#include <mb/pg_wchar.h>
#include <miscadmin.h>
#include <nodes/execnodes.h>
#include <utils/array.h>
#include <utils/builtins.h>
#include <utils/memutils.h>
#if PG_MAJORVERSION_NUM >= 16
#include <varatt.h>
#endif

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

PG_FUNCTION_INFO_V1(regex_in);
PG_FUNCTION_INFO_V1(regex_out);
PG_FUNCTION_INFO_V1(regex_named_groups);
PG_FUNCTION_INFO_V1(regex_match);
PG_FUNCTION_INFO_V1(regex_matches);
PG_FUNCTION_INFO_V1(regex_text_matches);
PG_FUNCTION_INFO_V1(regex_matches_text);
PG_FUNCTION_INFO_V1(regex_text_matches_not);
PG_FUNCTION_INFO_V1(regex_matches_text_not);

static pcre2_general_context *ctx;
static pcre2_compile_context *compile_ctx;
static MemoryContext RegexMemoryContext;

typedef text *pgpcre;

#define DatumGetPcreP(X) ((pgpcre *)PG_DETOAST_DATUM(X))
#define DatumGetPcrePCopy(X) ((pgpcre *)PG_DETOAST_DATUM_COPY(X))
#define PcrePGetDatum(X) PointerGetDatum(X)
#define PG_GETARG_PCRE_P(n) DatumGetPcreP(PG_GETARG_DATUM(n))
#define PG_GETARG_PCRE_P_COPY(n) DatumGetPcrePCopy(PG_GETARG_DATUM(n))
#define PG_RETURN_PCRE_P(x) return PcrePGetDatum(x)
#define cstring_to_pcre(s) ((pgpcre *)cstring_to_text(s))
#define pcre_to_cstring(p) (text_to_cstring((text *)p))

typedef struct {
  char *regex;
  size_t regex_length;
  uint32 status;
  pcre2_code *code;
} RegexHashEntry;

#define SH_PREFIX regexhash
#define SH_ELEMENT_TYPE RegexHashEntry
#define SH_KEY_TYPE char *
#define SH_KEY regex
#define SH_HASH_KEY(tb, key) string_hash(key, tb->data->regex_length)
#define SH_EQUAL(tb, a, b) (strcmp(a, b) == 0)
#define SH_SCOPE static inline
#define SH_DECLARE
#define SH_DEFINE
#include <lib/simplehash.h>

static inline pcre2_code *compile_expr(const char *input, size_t length) {
  PCRE2_SIZE erroffset;
  pcre2_code *pc;
  int err;

  if (GetDatabaseEncoding() == PG_UTF8) {
    pc = pcre2_compile((PCRE2_SPTR)input, length, PCRE2_UTF | PCRE2_UCP, &err, &erroffset,
                       compile_ctx);
  } else if (GetDatabaseEncoding() == PG_SQL_ASCII) {
    pc = pcre2_compile((PCRE2_SPTR)input, length, 0, &err, &erroffset, NULL);
  } else {
    char *utf8string;
    utf8string = (char *)pg_do_encoding_conversion((unsigned char *)input, length,
                                                   GetDatabaseEncoding(), PG_UTF8);
    pc = pcre2_compile((PCRE2_SPTR)utf8string, strlen(utf8string), PCRE2_UTF | PCRE2_UCP, &err,
                       &erroffset, NULL);
    if (utf8string != input)
      pfree(utf8string);
  }

  if (!pc) {
    PCRE2_UCHAR buf[255];

    pcre2_get_error_message(err, buf, sizeof(buf));
    ereport(ERROR, errmsg("regex compile error: %s", buf));
  }

  return pc;
}

static inline pcre2_code *PCRE_CODE(pgpcre *pattern) {
  static regexhash_hash *regexhash = NULL;
  if (regexhash == NULL) {
    regexhash = regexhash_create(RegexMemoryContext, 8192, NULL);
  }

  bool found;
  char *expr = VARDATA_ANY(pattern);
  size_t length = VARSIZE_ANY_EXHDR(pattern);
  uint32 hash = string_hash(expr, length);
  RegexHashEntry *entry = regexhash_lookup_hash(regexhash, expr, hash);
  if (!entry) {
    MemoryContext oldcontext = MemoryContextSwitchTo(RegexMemoryContext);
    pcre2_code *code;
    code = compile_expr(expr, length);
    entry = regexhash_insert_hash(regexhash, expr, hash, &found);
    entry->code = code;
    MemoryContextSwitchTo(oldcontext);
    return code;
  } else {
    return entry->code;
  }
}

Datum regex_in(PG_FUNCTION_ARGS) {
  char *input = PG_GETARG_CSTRING(0);

  pgpcre *expr = cstring_to_pcre(input);
  PCRE_CODE(expr); // discard the result, we're only checking

  PG_RETURN_PCRE_P(expr);
}

Datum regex_out(PG_FUNCTION_ARGS) {
  pgpcre *p = PG_GETARG_PCRE_P(0);

  PG_RETURN_CSTRING(pcre_to_cstring(p));
}

Datum regex_named_groups(PG_FUNCTION_ARGS) {
  pgpcre *p = PG_GETARG_PCRE_P(0);

  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  pcre2_code *pc = PCRE_CODE(p);

  // Retrieve the nametable
  PCRE2_SPTR nametable;
  uint32_t namecount, nameentrysize;
  pcre2_pattern_info(pc, PCRE2_INFO_NAMECOUNT, &namecount);
  if (namecount > 0) {

    pcre2_pattern_info(pc, PCRE2_INFO_NAMETABLE, &nametable);
    pcre2_pattern_info(pc, PCRE2_INFO_NAMEENTRYSIZE, &nameentrysize);

    // Iterate over the named groups
    PCRE2_SPTR entry = nametable;
    for (uint32_t i = 0; i < namecount; i++) {
      int group_number = (entry[0] << 8) | entry[1];
      Datum values[2] = {CStringGetDatum((char *)(entry + 2)), Int32GetDatum(group_number)};
      bool isnull[2] = {false, false};
      tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
      entry += nameentrysize;
    }
  }

#if PG_MAJORVERSION_NUM < 17
  tuplestore_donestoring(tupstore);
#endif

  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_NULL();
}

static inline bool matches_internal(text *subject, pgpcre *pattern, char ***return_matches,
                                    int *num_captured, pcre2_match_data **match_data) {
  pcre2_code *pc;
  pcre2_match_data *md;
  int rc;
  uint32_t captures = 0;
  PCRE2_SIZE *ovector;
  int ovecsize;
  char *utf8string;

  pc = PCRE_CODE(pattern);

  if (num_captured != NULL) {
    if ((rc = pcre2_pattern_info(pc, PCRE2_INFO_CAPTURECOUNT, &captures)) != 0) {
      ereport(ERROR, errmsg("pcre2_pattern_info error: %d", rc));
    }
  }

  if (*match_data == NULL) {
    if (return_matches) {
      ovecsize = (captures + 1) * 3;
      md = pcre2_match_data_create(ovecsize, NULL);
    } else {
      md = pcre2_match_data_create_from_pattern(pc, NULL);
    }
  } else {
    md = *match_data;
  }

  uint32_t flags = PCRE2_NO_UTF_CHECK; // Postgres or this extension have already validated UTF

  int offset = 0;
  if (*match_data != NULL) {
    PCRE2_SIZE *last_ovector = pcre2_get_ovector_pointer(*match_data);
    offset = last_ovector[1];

    flags |= (PCRE2_ANCHORED | PCRE2_NOTEMPTY_ATSTART);

    // Stop if the start offset is at or exceeds the subject length
    if (offset >= VARSIZE_ANY_EXHDR(subject)) {
      return false;
    }
  }
  if (GetDatabaseEncoding() == PG_UTF8 || GetDatabaseEncoding() == PG_SQL_ASCII) {
    utf8string = VARDATA_ANY(subject);
    rc = pcre2_match(pc, (PCRE2_SPTR)VARDATA_ANY(subject), VARSIZE_ANY_EXHDR(subject), offset,
                     flags, md, NULL);
  } else {
    utf8string = (char *)pg_do_encoding_conversion((unsigned char *)VARDATA_ANY(subject),
                                                   VARSIZE_ANY_EXHDR(subject),
                                                   GetDatabaseEncoding(), PG_UTF8);
    rc = pcre2_match(pc, (PCRE2_SPTR)utf8string, strlen(utf8string), offset, flags, md, NULL);
  }

  if (rc == PCRE2_ERROR_NOMATCH) {
    return false;
  } else if (rc < 0)
    elog(ERROR, "PCRE match error: %d", rc);

  *match_data = md;

  if (return_matches) {
    char **matches;

    if (num_captured && captures > 0) {
      int i;

      *num_captured = captures;
      matches = palloc(captures * sizeof(*matches));
      ovector = pcre2_get_ovector_pointer(md);

      for (i = 1; i <= captures; i++) {
        if ((int)ovector[i * 2] < 0)
          matches[i - 1] = NULL;
        else {
          PCRE2_UCHAR *xmatch;
          PCRE2_SIZE l;

          pcre2_substring_get_bynumber(md, i, &xmatch, &l);
          matches[i - 1] = (char *)xmatch;
        }
      }

    } else {
      PCRE2_UCHAR *xmatch;
      PCRE2_SIZE l;

      matches = palloc(1 * sizeof(*matches));
      pcre2_substring_get_bynumber(md, 0, &xmatch, &l);
      matches[0] = (char *)xmatch;

      if (num_captured) {
        *num_captured = 1;
      }
    }

    *return_matches = matches;
  }

done:

  return true;
}

Datum regex_match(PG_FUNCTION_ARGS) {
  text *subject = PG_GETARG_TEXT_PP(0);
  pgpcre *pattern = PG_GETARG_PCRE_P(1);
  char **matches;
  int num_captured;

  pcre2_match_data *md = NULL;
  if (matches_internal(subject, pattern, &matches, &num_captured, &md)) {
    ArrayType *result;
    int dims[1];
    int lbs[1];
    Datum *elems;
    bool *nulls;
    int i;

    dims[0] = num_captured;
    lbs[0] = 1;

    elems = palloc(num_captured * sizeof(*elems));
    nulls = palloc(num_captured * sizeof(*nulls));
    for (i = 0; i < num_captured; i++) {
      if (matches[i]) {
        elems[i] = PointerGetDatum(cstring_to_text(matches[i]));
        nulls[i] = false;
      } else
        nulls[i] = true;
    }

    result = construct_md_array(elems, nulls, 1, dims, lbs, TEXTOID, -1, false, 'i');

    PG_RETURN_ARRAYTYPE_P(result);
  } else
    PG_RETURN_NULL();
}

Datum regex_matches(PG_FUNCTION_ARGS) {
  text *subject = PG_GETARG_TEXT_PP(0);
  pgpcre *pattern = PG_GETARG_PCRE_P(1);
  char **matches;
  int num_captured;

  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  pcre2_match_data *md = NULL;
  while (matches_internal(subject, pattern, &matches, &num_captured, &md)) {
    CHECK_FOR_INTERRUPTS();
    ArrayType *result;
    int dims[1];
    int lbs[1];
    Datum *elems;
    bool *nulls;
    int i;

    dims[0] = num_captured;
    lbs[0] = 1;

    elems = palloc(num_captured * sizeof(*elems));
    nulls = palloc(num_captured * sizeof(*nulls));
    for (i = 0; i < num_captured; i++) {
      if (matches[i]) {
        elems[i] = PointerGetDatum(cstring_to_text(matches[i]));
        nulls[i] = false;
      } else
        nulls[i] = true;
    }

    result = construct_md_array(elems, nulls, 1, dims, lbs, TEXTOID, -1, false, 'i');

    Datum values[1] = {PointerGetDatum(result)};
    bool isnull[1] = {false};
    tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
  }

#if PG_MAJORVERSION_NUM < 17
  tuplestore_donestoring(tupstore);
#endif

  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_NULL();
}

Datum regex_text_matches(PG_FUNCTION_ARGS) {
  text *subject = PG_GETARG_TEXT_PP(0);
  pgpcre *pattern = PG_GETARG_PCRE_P(1);

  pcre2_match_data *md = NULL;
  PG_RETURN_BOOL(matches_internal(subject, pattern, NULL, NULL, &md));
}

Datum regex_matches_text(PG_FUNCTION_ARGS) {
  pgpcre *pattern = PG_GETARG_PCRE_P(0);
  text *subject = PG_GETARG_TEXT_PP(1);

  pcre2_match_data *md = NULL;
  PG_RETURN_BOOL(matches_internal(subject, pattern, NULL, NULL, &md));
}

Datum regex_text_matches_not(PG_FUNCTION_ARGS) {
  text *subject = PG_GETARG_TEXT_PP(0);
  pgpcre *pattern = PG_GETARG_PCRE_P(1);

  pcre2_match_data *md = NULL;
  PG_RETURN_BOOL(!matches_internal(subject, pattern, NULL, NULL, &md));
}

Datum regex_matches_text_not(PG_FUNCTION_ARGS) {
  pgpcre *pattern = PG_GETARG_PCRE_P(0);
  text *subject = PG_GETARG_TEXT_PP(1);

  pcre2_match_data *md = NULL;
  PG_RETURN_BOOL(!matches_internal(subject, pattern, NULL, NULL, &md));
}

static void *pcre2_malloc(PCRE2_SIZE size, void *data) { return palloc(size); }
static void pcre2_free(void *ptr, void *data) {
  if (ptr != NULL) {
    pfree(ptr);
  }
}

void _PG_init(void) {
  RegexMemoryContext =
      AllocSetContextCreate(TopMemoryContext, "RegexMemoryContext", ALLOCSET_DEFAULT_SIZES);
  MemoryContext oldcontext = MemoryContextSwitchTo(RegexMemoryContext);
  ctx = pcre2_general_context_create(pcre2_malloc, pcre2_free, NULL);
  compile_ctx = pcre2_compile_context_create(ctx);
  MemoryContextSwitchTo(oldcontext);
}