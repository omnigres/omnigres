/**
 * @file local_fs.c
 *
 */

#include <sys/stat.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <catalog/pg_enum.h>
#include <common/int.h>
#include <executor/executor.h>
#include <executor/spi.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <nodes/execnodes.h>
#include <utils/builtins.h>
#include <utils/syscache.h>
#include <utils/timestamp.h>

#include "pg_path.h"

#include "omni_vfs.h"

PG_FUNCTION_INFO_V1(local_fs);
PG_FUNCTION_INFO_V1(local_fs_list);
PG_FUNCTION_INFO_V1(local_fs_file_info);
PG_FUNCTION_INFO_V1(local_fs_read);

#ifdef PATH_MAX
#define PATH_MAX_ PATH_MAX
#else
#define PATH_MAX_ 4096
#endif

char *subpath(const char *parent, const char *child) {
  char *absolute_parent = make_absolute_path(parent);
  char *tmp_parent = palloc(PATH_MAX_);

  snprintf(tmp_parent, PATH_MAX_, "%s/%s", absolute_parent, child);

  char *absolute_child = make_absolute_path(tmp_parent);
  pfree(tmp_parent);
  char *subpath = pstrdup(absolute_child);
  free(absolute_child);

  size_t parent_len = strlen(absolute_parent);
  if (parent_len > strlen(subpath)) {
    ereport(ERROR, errmsg("requested path is outside of the mount point"));
  }

  if (strncmp(absolute_parent, subpath, parent_len) == 0) {
    free(absolute_parent);
    return subpath;
  } else {
    free(absolute_parent);
    ereport(ERROR, errmsg("requested path is outside of the mount point"),
            errdetail("mount point: %s, path: %s", absolute_parent, absolute_child));
  }
}

static SPIPlanPtr lookup_fs = NULL;
static SPIPlanPtr insert_fs = NULL;

Datum local_fs(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("mount must not be NULL"));
  }

  text *absolute_mount = cstring_to_text(subpath(text_to_cstring(PG_GETARG_TEXT_PP(0)), "."));

  SPI_connect();
  if (lookup_fs == NULL) {
    char *query = "select row(id)::omni_vfs.local_fs from "
                  "omni_vfs.local_fs_mounts where mount = $1";
    lookup_fs = SPI_prepare(query, 1, (Oid[1]){TEXTOID});
    SPI_keepplan(lookup_fs);
  }
  int rc = SPI_execute_plan(lookup_fs, (Datum[1]){PointerGetDatum(absolute_mount)}, (char[1]){' '},
                            false, 0);
  if (rc != SPI_OK_SELECT) {
    ereport(ERROR, errmsg("failed obtaining local_fs"),
            errdetail("%s", SPI_result_code_string(rc)));
  }

  if (SPI_tuptable->numvals == 0) {
    // The mount does not exist, try creating it
    if (insert_fs == NULL) {
      char *insert = "insert into omni_vfs.local_fs_mounts (mount) values($1) returning "
                     "row(id)::omni_vfs.local_fs";
      insert_fs = SPI_prepare(insert, 1, (Oid[1]){TEXTOID});
      SPI_keepplan(insert_fs);
    }
    rc = SPI_execute_plan(insert_fs, (Datum[1]){PointerGetDatum(absolute_mount)}, (char[1]){' '},
                          false, 0);
    if (rc != SPI_OK_INSERT_RETURNING) {
      ereport(ERROR, errmsg("failed creating local_fs"),
              errdetail("%s", SPI_result_code_string(rc)));
    }
  }

  TupleDesc tupdesc = SPI_tuptable->tupdesc;
  HeapTuple tuple = SPI_tuptable->vals[0];
  bool isnull;
  Datum local_fs = SPI_datumTransfer(SPI_getbinval(tuple, tupdesc, 1, &isnull), false, -1);

  SPI_finish();

  PG_RETURN_DATUM(local_fs);
}

static SPIPlanPtr get_fs = NULL;

static char *get_mount_path(Datum fs_id) {
  // Get mount from the local_fs_mounts table. It has Row-Level Security enabled,
  // so we can enforce policies on this.
  MemoryContext oldcontext = CurrentMemoryContext;
  SPI_connect();
  if (get_fs == NULL) {
    get_fs = SPI_prepare("select mount from omni_vfs.local_fs_mounts where id = $1", 1,
                         (Oid[1]){INT4OID});
    SPI_keepplan(get_fs);
  }
  int rc = SPI_execute_plan(get_fs, (Datum[1]){fs_id}, (char[1]){' '}, false, 0);
  if (rc != SPI_OK_SELECT) {
    ereport(ERROR, errmsg("fetching mount failed"), errdetail("%s", SPI_result_code_string(rc)));
  }
  if (SPI_tuptable->numvals == 0) {
    ereport(ERROR, errmsg("fetching mount failed"),
            errdetail("missing information in omni_vfs.local_fs_mounts"));
  }

  TupleDesc tupdesc = SPI_tuptable->tupdesc;
  HeapTuple tuple = SPI_tuptable->vals[0];
  bool isnull;
  Datum mount = SPI_getbinval(tuple, tupdesc, 1, &isnull);
  if (isnull) {
    ereport(ERROR, errmsg("mount must not be NULL"));
  }
  char *mount_path = MemoryContextStrdup(oldcontext, text_to_cstring(DatumGetTextPP(mount)));

  SPI_finish();

  return mount_path;
}

Datum local_fs_list(PG_FUNCTION_ARGS) {

  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("fs must not be NULL"));
  }

  if (PG_ARGISNULL(1)) {
    ereport(ERROR, errmsg("dir must not be NULL"));
  }

  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  HeapTupleHeader fs = PG_GETARG_HEAPTUPLEHEADER(0);

  bool isnull;

  Datum fs_id = GetAttributeByName(fs, "id", &isnull);
  if (isnull) {
    ereport(ERROR, errmsg("filesystem ID must not be NULL"));
  }

  char *mount_path = get_mount_path(fs_id);

  text *given_path = PG_GETARG_TEXT_PP(1);
  char *path = subpath(mount_path, text_to_cstring(given_path));

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  // Check if the file is a directory
  bool is_dir = true;
  {
    struct stat s;
    if (stat(path, &s) == 0) {
      is_dir = S_ISDIR(s.st_mode);
    } else {
      // can't stat the file, ignore it
      goto done;
    }
  }

  bool fail_unpermitted = true;
  if (!PG_ARGISNULL(2)) {
    fail_unpermitted = PG_GETARG_BOOL(2);
  }

  if (is_dir) {

    DIR *d = opendir(path);
    struct dirent *dirent;

    if (d) {
      while ((dirent = readdir(d)) != NULL) {
#if defined(__APPLE__)
        uint64_t namlen = dirent->d_namlen;
#elif defined(_DIRENT_HAVE_D_NAMLEN)
        unsigned char namlen = dirent->d_namlen;
#else
        size_t namlen = strlen(dirent->d_name);
#endif
        if (namlen <= 2 && strncmp(dirent->d_name, "..", namlen) == 0)
          continue;
        Datum values[2] = {PointerGetDatum(cstring_to_text_with_len(dirent->d_name, namlen)),
                           ObjectIdGetDatum(dirent->d_type == DT_DIR ? file_kind_dir_oid()
                                                                     : file_kind_file_oid())};
        bool isnull[2] = {false, false};
        tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
      }

      closedir(d);
    } else {
      int e = errno;
      int report_type = ERROR;
      if (e == EPERM && !fail_unpermitted) {
#if PG_MAJORVERSION_NUM > 13
        report_type = WARNING_CLIENT_ONLY;
#else
        report_type = WARNING;
#endif
      }
      ereport(report_type, errmsg("can't open directory: %s", strerror(e)), errdetail("%s", path));
    }
  } else {
    Datum values[2] = {PointerGetDatum(given_path), ObjectIdGetDatum(file_kind_file_oid())};
    bool isnull[2] = {false, false};
    tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
  }

done:
  tuplestore_donestoring(tupstore);

  MemoryContextSwitchTo(oldcontext);

  PG_RETURN_NULL();
}

static inline Timestamp timespec_to_timestamp(struct timespec ts) {
  const int64 epoch_diff = 946684800;

  int64 microseconds;
  if (pg_mul_s64_overflow(ts.tv_sec - epoch_diff, USECS_PER_SEC, &microseconds)) {
    goto Overflow;
  };

  if (pg_add_s64_overflow(microseconds, ts.tv_nsec / 1000, &microseconds)) {
    goto Overflow;
  }

  return microseconds;
Overflow:
  if (microseconds < PG_INT64_MIN || microseconds > PG_INT64_MAX)
    ereport(ERROR,
            (errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE), errmsg("timestamp out of range")));
  return -1;
}

Datum local_fs_file_info(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("fs must not be NULL"));
  }

  if (PG_ARGISNULL(1)) {
    ereport(ERROR, errmsg("path must not be NULL"));
  }

  bool isnull;

  HeapTupleHeader fs = PG_GETARG_HEAPTUPLEHEADER(0);

  Datum fs_id = GetAttributeByName(fs, "id", &isnull);
  if (isnull) {
    ereport(ERROR, errmsg("filesystem ID must not be NULL"));
  }

  char *mount_path = get_mount_path(fs_id);

  text *path = PG_GETARG_TEXT_PP(1);
  char *fullpath = subpath(mount_path, text_to_cstring(path));

  struct stat file_stat;
  int err = stat(fullpath, &file_stat);
  if (err != 0) {
    int e = errno;
    if (e != ENOENT) {
      ereport(ERROR, errmsg("can't get file information for %s", fullpath),
              errdetail("%s", strerror(e)));
    } else {
      PG_RETURN_NULL();
    }
  }

  TupleDesc header_tupledesc = TypeGetTupleDesc(file_info_oid(), NULL);
  BlessTupleDesc(header_tupledesc);

  Datum kind =
      ObjectIdGetDatum(S_ISDIR(file_stat.st_mode) ? file_kind_dir_oid() : file_kind_file_oid());

#if defined(__APPLE__)
  HeapTuple header_tuple = heap_form_tuple(
      header_tupledesc,
      (Datum[5]){Int64GetDatum(file_stat.st_size),
                 TimestampGetDatum(timespec_to_timestamp(file_stat.st_ctimespec)),
                 TimestampGetDatum(timespec_to_timestamp(file_stat.st_atimespec)),
                 TimestampGetDatum(timespec_to_timestamp(file_stat.st_mtimespec)), kind},
      (bool[5]){false, false, false, false, false});
#elif defined(__linux__)
  HeapTuple header_tuple =
      heap_form_tuple(header_tupledesc,
                      (Datum[5]){Int64GetDatum(file_stat.st_size), 0,
                                 TimestampGetDatum(timespec_to_timestamp(file_stat.st_atim)),
                                 TimestampGetDatum(timespec_to_timestamp(file_stat.st_mtim)), kind},
                      (bool[5]){false, true, false, false, false});
#else
  HeapTuple header_tuple = heap_form_tuple(
      header_tupledesc,
      (Datum[5]){Int64GetDatum(file_stat.st_size), 0,
                 TimestampGetDatum(timespec_to_timestamp(
                     (struct timespec){.tv_sec = file_stat.st_atime, .tv_nsec = 0})),
                 TimestampGetDatum(timespec_to_timestamp(
                     (struct timespec){.tv_sec = file_stat.st_mtime, .tv_nsec = 0})),
                 kind},
      (bool[5]){false, true, false, false, false});
#endif

  PG_RETURN_DATUM(HeapTupleGetDatum(header_tuple));
}

Datum local_fs_read(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("fs must not be NULL"));
  }

  if (PG_ARGISNULL(1)) {
    ereport(ERROR, errmsg("path must not be NULL"));
  }

  bool isnull;

  HeapTupleHeader fs = PG_GETARG_HEAPTUPLEHEADER(0);

  Datum fs_id = GetAttributeByName(fs, "id", &isnull);
  if (isnull) {
    ereport(ERROR, errmsg("filesystem ID must not be NULL"));
  }

  char *mount_path = get_mount_path(fs_id);

  text *path = PG_GETARG_TEXT_PP(1);
  char *fullpath = subpath(mount_path, text_to_cstring(path));

  FILE *fp = fopen(fullpath, "r");
  if (fp == NULL) {
    int e = errno;
    ereport(ERROR, errmsg("can't open file"), errdetail("%s", strerror(e)));
  }

  long offset = 0;
  if (!PG_ARGISNULL(2)) {
    offset = PG_GETARG_INT64(2);
  }

  if (fseek(fp, offset, SEEK_SET) == -1) {
    int e = errno;
    fclose(fp);
    ereport(ERROR, errmsg("can't seek to a given offset"), errdetail("%s", strerror(e)));
  }

  long size;
  if (!PG_ARGISNULL(3)) {
    size = PG_GETARG_INT32(3);
  } else {
    if (fseek(fp, 0, SEEK_END) == -1) {
      int e = errno;
      fclose(fp);
      ereport(ERROR, errmsg("can't seek to the end of the file"));
    }
    size = ftell(fp) - offset;
    if (fseek(fp, offset, SEEK_SET) == -1) {
      int e = errno;
      fclose(fp);
      ereport(ERROR, errmsg("can't seek to a given offset"), errdetail("%s", strerror(e)));
    }
  }

  if (size > 1024 * 1024 * 1024) {
    fclose(fp);
    ereport(ERROR, errmsg("chunk_size can't be over 1GB"));
  }

  void *data = palloc(VARHDRSZ + size);
  long actual_size = fread(VARDATA(data), 1, size, fp);
  if (actual_size != size) {
    data = repalloc(data, actual_size);
  }

  SET_VARSIZE(data, actual_size + VARHDRSZ);

  fclose(fp);

  PG_RETURN_POINTER(data);
}