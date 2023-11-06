#include <string.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <port/pg_bswap.h>
#include <utils/builtins.h>

#include "omni_containers.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(docker_stream_to_text);

Datum docker_stream_to_text(PG_FUNCTION_ARGS) {
  bytea *stream = PG_GETARG_BYTEA_PP(0);
  // The actual output will be smaller, but this is the absolute
  // upper bound and we don't want to constantly re-allocate it to grow
  Size size = VARSIZE_ANY_EXHDR(stream);
  char *output = palloc0(size);
  char *data = (char *)VARDATA_ANY(stream);
  char *current = data;
  long i = 0;
  // If there's any stream data at all
  if (size >= 8) {
    // Keep reading frames
    while (true) {
      uint32_t sz = pg_ntoh32(*((uint32_t *)(current + 4)));
      memcpy(output + i, current + 8, sz);
      i += sz;
      current += 8 + sz;
      // until we run out of data
      if (current - data == size) {
        break;
      }
    }
  }

  PG_RETURN_TEXT_P(cstring_to_text_with_len(output, i));
}