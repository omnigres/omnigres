/**
 * \file uuidv7.c
 *
 * This file is derived from the v27 version of the patch to Postgres:
 *
 * https://www.postgresql.org/message-id/flat/CAAhFRxitJv=yoGnXUgeLB_O+M7J2BJAmb5jqAT9gZ3bij3uLDA@mail.gmail.com
 *
 * Should the new version of this patch appear, we can merged it. Once merged in all supported
 * versions of Postgres, we can remove it.
 *
 * It is licensed under the terms of PostgreSQL license and the original implementation is by
 *
 * * Andrey * M. Borodin
 *
 */
// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <utils/fmgrprotos.h>
#include <utils/timestamp.h>
#include <utils/uuid.h>

#include <sys/time.h>

PG_MODULE_MAGIC;

static uint64 get_real_time_ns();

#ifndef WIN32
#include <time.h>

static uint64 get_real_time_ns() {
  struct timespec tmp;

  clock_gettime(CLOCK_REALTIME, &tmp);
  return tmp.tv_sec * 1000000000L + tmp.tv_nsec;
}
#else /* WIN32 */

#include "c.h"
#include <sys/time.h>
#include <sysinfoapi.h>

/* FILETIME of Jan 1 1970 00:00:00, the PostgreSQL epoch */
static const unsigned __int64 epoch = UINT64CONST(116444736000000000);

/*
 * FILETIME represents the number of 100-nanosecond intervals since
 * January 1, 1601 (UTC).
 */
#define FILETIME_UNITS_TO_NS UINT64CONST(100)

/*
 * timezone information is stored outside the kernel so tzp isn't used anymore.
 *
 * Note: this function is not for Win32 high precision timing purposes. See
 * elapsed_time().
 */
static uint64 get_real_time_ns() {
  FILETIME file_time;
  ULARGE_INTEGER ularge;

  GetSystemTimePreciseAsFileTime(&file_time);
  ularge.LowPart = file_time.dwLowDateTime;
  ularge.HighPart = file_time.dwHighDateTime;

  return (ularge.QuadPart - epoch) * FILETIME_UNITS_TO_NS;
}
#endif

PG_FUNCTION_INFO_V1(uuidv7);

/*
 * Generate UUID version 7 per RFC 9562.
 *
 * Monotonicity (regarding generation on given backend) is ensured with method
 * "Replace Leftmost Random Bits with Increased Clock Precision (Method 3)"
 * We use 12 bits in "rand_a" bits to store 1/4096 fractions of millisecond.
 * Usage of pg_testtime indicates that such precision is avaiable on most
 * systems. If timestamp is not advancing between two consecutive UUID
 * generations, previous timestamp is incremented and used instead of current
 * timestamp.
 */
Datum uuidv7(PG_FUNCTION_ARGS) {
  static uint64 previous_ns = 0;

  pg_uuid_t *uuid = palloc(UUID_LEN);
  uint64 ns;
  uint64 unix_ts_ms;
  uint16 incresed_clock_precision;

/* minimum amount of ns that guarantees step of incresed_clock_precision */
#define SUB_MILLISECOND_STEP (1000000 / 4096 + 1)
  ns = get_real_time_ns();
  if (previous_ns + SUB_MILLISECOND_STEP >= ns)
    ns = previous_ns + SUB_MILLISECOND_STEP;
  previous_ns = ns;

  if (PG_NARGS() > 0) {
    Interval *span;
    TimestampTz ts = (TimestampTz)(ns / 1000) -
                     (POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY * USECS_PER_SEC;
    span = PG_GETARG_INTERVAL_P(0);
    ts = DatumGetTimestampTz(DirectFunctionCall2(timestamptz_pl_interval, TimestampTzGetDatum(ts),
                                                 IntervalPGetDatum(span)));
    ns = (ts + (POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY * USECS_PER_SEC) * 1000 +
         ns % 1000;
  }

  unix_ts_ms = ns / 1000000;

  /* Fill in time part */
  uuid->data[0] = (unsigned char)(unix_ts_ms >> 40);
  uuid->data[1] = (unsigned char)(unix_ts_ms >> 32);
  uuid->data[2] = (unsigned char)(unix_ts_ms >> 24);
  uuid->data[3] = (unsigned char)(unix_ts_ms >> 16);
  uuid->data[4] = (unsigned char)(unix_ts_ms >> 8);
  uuid->data[5] = (unsigned char)unix_ts_ms;

  /* sub-millisecond timestamp fraction (12 bits) */
  incresed_clock_precision = ((ns % 1000000) * 4096) / 1000000;

  uuid->data[6] = (unsigned char)(incresed_clock_precision >> 8);
  uuid->data[7] = (unsigned char)(incresed_clock_precision);

  /* fill everything after the increased clock precision with random bytes */
  if (!pg_strong_random(&uuid->data[8], UUID_LEN - 8))
    ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("could not generate random values")));

  /*
   * Set magic numbers for a "version 7" (pseudorandom) UUID, see
   * https://www.rfc-editor.org/rfc/rfc9562#name-version-field
   */
  /* set version field, top four bits are 0, 1, 1, 1 */
  uuid->data[6] = (uuid->data[6] & 0x0f) | 0x70;
  /* set variant field, top two bits are 1, 0 */
  uuid->data[8] = (uuid->data[8] & 0x3f) | 0x80;

  PG_RETURN_UUID_P(uuid);
}
