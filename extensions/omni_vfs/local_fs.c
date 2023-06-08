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
  char absolute_parent[PATH_MAX_];
  char absolute_child[PATH_MAX_];
  char *tmp_parent = palloc(PATH_MAX_);

  strncpy(tmp_parent, parent, PATH_MAX_);

  if (realpath(tmp_parent, absolute_parent) == NULL) {
    int e = errno;
    ereport(ERROR, errmsg("can't get a realpath for mount"), errdetail("%s", strerror(e)));
  }

  snprintf(tmp_parent, PATH_MAX_, "%s/%s", absolute_parent, child);

  if (realpath(tmp_parent, absolute_child) == NULL) {
    int e = errno;
    ereport(ERROR, errmsg("can't get a realpath for the given path %s", tmp_parent),
            errdetail("%s", strerror(e)));
  }

  size_t parent_len = strlen(absolute_parent);
  if (parent_len > strlen(absolute_child)) {
    ereport(ERROR, errmsg("requested path is outside of the mount point"));
  }

  if (strncmp(absolute_parent, absolute_child, parent_len) == 0) {
    return tmp_parent;
  } else {
    ereport(ERROR, errmsg("requested path is outside of the mount point"));
  }
}

Datum local_fs(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("must must not be NULL"));
  }

  MemoryContext oldcontext = CurrentMemoryContext;

  text *absolute_mount = cstring_to_text(subpath(text_to_cstring(PG_GETARG_TEXT_PP(0)), "."));

  SPI_connect();
  char *query = "select row(id)::omni_vfs.local_fs from "
                "omni_vfs.local_fs_mounts where mount = $1";
  int rc =
      SPI_execute_with_args(query, 1, (Oid[1]){TEXTOID},
                            (Datum[1]){PointerGetDatum(absolute_mount)}, (char[1]){' '}, false, 0);
  if (rc != SPI_OK_SELECT) {
    ereport(ERROR, errmsg("failed obtaining local_fs"),
            errdetail("%s", SPI_result_code_string(rc)));
  }

  if (SPI_tuptable->numvals == 0) {
    // The mount does not exist, try creating it
    char *insert = "insert into omni_vfs.local_fs_mounts (mount) values($1) returning "
                   "row(id)::omni_vfs.local_fs";
    rc = SPI_execute_with_args(insert, 1, (Oid[1]){TEXTOID},
                               (Datum[1]){PointerGetDatum(absolute_mount)}, (char[1]){' '}, false,
                               0);
    if (rc != SPI_OK_INSERT_RETURNING) {
      ereport(ERROR, errmsg("failed creating local_fs"),
              errdetail("%s", SPI_result_code_string(rc)));
    }
  }

  TupleDesc tupdesc = SPI_tuptable->tupdesc;
  HeapTuple tuple = SPI_tuptable->vals[0];
  bool isnull;
  Datum local_fs = SPI_getbinval(tuple, tupdesc, 1, &isnull);

  MemoryContext spi_context = CurrentMemoryContext;
  MemoryContextSwitchTo(oldcontext);
  SPI_datumTransfer(local_fs, false, -1);
  MemoryContextSwitchTo(spi_context);

  SPI_finish();

  PG_RETURN_DATUM(local_fs);
}

static char *get_mount_path(Datum fs_id) {
  // Get mount from the local_fs_mounts table. It has Row-Level Security enabled,
  // so we can enforce policies on this.
  MemoryContext oldcontext = CurrentMemoryContext;
  SPI_connect();
  int rc = SPI_execute_with_args("select mount from omni_vfs.local_fs_mounts where id = $1", 1,
                                 (Oid[1]){INT4OID}, (Datum[1]){fs_id}, (char[1]){' '}, false, 0);
  if (rc != SPI_OK_SELECT) {
    ereport(ERROR, errmsg("fetching mount failed"), errdetail("%s", SPI_result_code_string(rc)));
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

  text *dir = PG_GETARG_TEXT_PP(1);
  char *path = subpath(mount_path, text_to_cstring(dir));

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

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
      Datum values[2] = {
          PointerGetDatum(cstring_to_text_with_len(dirent->d_name, namlen)),
          ObjectIdGetDatum(dirent->d_type == DT_DIR ? file_kind_dir_oid() : file_kind_file_oid())};
      bool isnull[2] = {false, false};
      tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
    }

    closedir(d);
  } else {
    ereport(ERROR, errmsg("can't open directory"), errdetail("%s", path));
  }

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
    ereport(ERROR, errmsg("can't get file information"), errdetail("%s", strerror(e)));
  }

  TupleDesc header_tupledesc = TypeGetTupleDesc(file_info_oid(), NULL);
  BlessTupleDesc(header_tupledesc);

#if defined(__APPLE__)
  HeapTuple header_tuple =
      heap_form_tuple(header_tupledesc,
                      (Datum[4]){Int64GetDatum(file_stat.st_size),
                                 TimestampGetDatum(timespec_to_timestamp(file_stat.st_ctimespec)),
                                 TimestampGetDatum(timespec_to_timestamp(file_stat.st_atimespec)),
                                 TimestampGetDatum(timespec_to_timestamp(file_stat.st_mtimespec))},
                      (bool[4]){false, false, false, false});
#elif defined(__linux__)
  HeapTuple header_tuple =
      heap_form_tuple(header_tupledesc,
                      (Datum[4]){Int64GetDatum(file_stat.st_size), 0,
                                 TimestampGetDatum(timespec_to_timestamp(file_stat.st_atim)),
                                 TimestampGetDatum(timespec_to_timestamp(file_stat.st_mtim))},
                      (bool[4]){false, true, false, false});
#else
  HeapTuple header_tuple = heap_form_tuple(
      header_tupledesc,
      (Datum[4]){Int64GetDatum(file_stat.st_size), 0,
                 TimestampGetDatum(timespec_to_timestamp(
                     (struct timespec){.tv_sec = file_stat.st_atime, .tv_nsec = 0})),
                 TimestampGetDatum(timespec_to_timestamp(
                     (struct timespec){.tv_sec = file_stat.st_mtime, .tv_nsec = 0}))},
      (bool[4]){false, true, false, false});
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
    size = PG_GETARG_INT64(3);
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