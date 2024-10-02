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

typedef struct {
  int32 vl_len_;
  int16 pcre_major; /* new version might invalidate compiled pattern */
  int16 pcre_minor;
  int32 pattern_strlen; /* used to compute offset to compiled pattern */
  uint32 hash;
  char data[FLEXIBLE_ARRAY_MEMBER]; /* original pattern string
                                     * (null-terminated), followed by
                                     * compiled pattern */
} pgpcre;

#define DatumGetPcreP(X) ((pgpcre *)PG_DETOAST_DATUM(X))
#define DatumGetPcrePCopy(X) ((pgpcre *)PG_DETOAST_DATUM_COPY(X))
#define PcrePGetDatum(X) PointerGetDatum(X)
#define PG_GETARG_PCRE_P(n) DatumGetPcreP(PG_GETARG_DATUM(n))
#define PG_GETARG_PCRE_P_COPY(n) DatumGetPcrePCopy(PG_GETARG_DATUM(n))
#define PG_RETURN_PCRE_P(x) return PcrePGetDatum(x)

typedef struct {
  char *regex;
  uint32 status;
  pcre2_code *code;
} RegexHashEntry;

#define SH_PREFIX regexhash
#define SH_ELEMENT_TYPE RegexHashEntry
#define SH_KEY_TYPE char *
#define SH_KEY regex
#define SH_HASH_KEY(tb, key) string_hash(key, strlen(key))
#define SH_EQUAL(tb, a, b) (strcmp(a, b) == 0)
#define SH_SCOPE static inline
#define SH_DECLARE
#define SH_DEFINE
#include <lib/simplehash.h>

static inline pcre2_code *PCRE_CODE(pgpcre *pattern) {
  static regexhash_hash *regexhash = NULL;
  if (regexhash == NULL) {
    regexhash = regexhash_create(RegexMemoryContext, 8192, NULL);
  }

  bool found;
  RegexHashEntry *entry = regexhash_lookup_hash(regexhash, pattern->data, pattern->hash);
  if (!entry) {
    MemoryContext oldcontext = MemoryContextSwitchTo(RegexMemoryContext);
    pcre2_code *code[1];
    int rc = pcre2_serialize_decode(code, 1,
                                    (uint8_t *)(pattern->data + pattern->pattern_strlen + 1), ctx);
    if (rc <= 0) {
      // Recompile
      code[0] = PCRE_CODE(
          (pgpcre *)DatumGetPointer(DirectFunctionCall1(regex_in, CStringGetDatum(pattern->data))));
    }
    entry = regexhash_insert_hash(regexhash, pattern->data, pattern->hash, &found);
    entry->code = code[0];
    MemoryContextSwitchTo(oldcontext);
    return code[0];
  } else {
    return entry->code;
  }
}

Datum regex_in(PG_FUNCTION_ARGS) {
  char *input = PG_GETARG_CSTRING(0);
  int length = strlen(input);
  int rc;
  int err;
  PCRE2_SIZE erroffset;
  const pcre2_code *pc;

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

  PCRE2_SIZE sz;
  uint8_t *bytes;
  pcre2_serialize_encode((const pcre2_code *[]){pc}, 1, &bytes, &sz, ctx);

  int total_len = offsetof(pgpcre, data) + length + 1 + sz;
  pgpcre *result = (pgpcre *)palloc0(total_len);
  SET_VARSIZE(result, total_len);
  result->pcre_major = PCRE2_MAJOR;
  result->pcre_minor = PCRE2_MINOR;
  result->pattern_strlen = length;
  result->hash = string_hash(input, length);
  strcpy(result->data, input);
  memcpy(result->data + length + 1, bytes, sz);

  PG_RETURN_PCRE_P(result);
}

Datum regex_out(PG_FUNCTION_ARGS) {
  pgpcre *p = PG_GETARG_PCRE_P(0);

  PG_RETURN_CSTRING(pstrdup(p->data));
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
  static bool warned = false;

  if (!warned && (pattern->pcre_major != PCRE2_MAJOR || pattern->pcre_minor != PCRE2_MINOR)) {
    ereport(WARNING,
            (errmsg("PCRE version mismatch"),
             errdetail("The compiled pattern was created by PCRE version %d.%d, the current "
                       "library is version %d.%d.  According to the PCRE documentation, "
                       "\"compiling a regular expression with one version of PCRE for use with a "
                       "different version is not guaranteed to work and may cause crashes.\"  This "
                       "warning is shown only once per session.",
                       pattern->pcre_major, pattern->pcre_minor, PCRE2_MAJOR, PCRE2_MINOR),
             errhint("You might want to recompile the stored patterns by running something like "
                     "UPDATE ... SET pcre_col = pcre_col::text::pcre.")));
    warned = true;
    // TODO: recompile
  }

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