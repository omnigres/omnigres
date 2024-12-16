/**
 * \file uuidv7.c
 *
 * This file is derived from the `78c5e141e9c139fc2ff36a220334e4aa25e1b0eb` commit to Postgres
 *
 * Discussion:
 * https://postgr.es/m/CAAhFRxitJv%3DyoGnXUgeLB_O%2BM7J2BJAmb5jqAT9gZ3bij3uLDA%40mail.gmail.com
 *
 * Should the new version of this patch appear, we can merged it. Once merged in all supported
 * versions of Postgres, we can remove it.
 *
 */
/*-------------------------------------------------------------------------
 *
 * uuid.c
 *	  Functions for the built-in type "uuid".
 *
 * Copyright (c) 2007-2024, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/uuid.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <time.h> /* for clock_gettime() */

#include "common/hashfn.h"
#include "lib/hyperloglog.h"
#include "libpq/pqformat.h"
#include "port/pg_bswap.h"
#include "utils/fmgrprotos.h"
#include "utils/guc.h"
#include "utils/sortsupport.h"
#include "utils/timestamp.h"
#include "utils/uuid.h"

PG_FUNCTION_INFO_V1(uuidv7_);
PG_FUNCTION_INFO_V1(uuidv7_interval_);
PG_FUNCTION_INFO_V1(uuid_extract_timestamp_);
PG_FUNCTION_INFO_V1(uuid_extract_version_);

/* helper macros */
#define NS_PER_S INT64CONST(1000000000)
#define NS_PER_MS INT64CONST(1000000)
#define NS_PER_US INT64CONST(1000)

/*
 * UUID version 7 uses 12 bits in "rand_a" to store  1/4096 (or 2^12) fractions of
 * sub-millisecond. While most Unix-like platforms provide nanosecond-precision
 * timestamps, some systems only offer microsecond precision, limiting us to 10
 * bits of sub-millisecond information. For example, on macOS, real time is
 * truncated to microseconds. Additionally, MSVC uses the ported version of
 * gettimeofday() that returns microsecond precision.
 *
 * On systems with only 10 bits of sub-millisecond precision, we still use
 * 1/4096 parts of a millisecond, but fill lower 2 bits with random numbers
 * (see generate_uuidv7() for details).
 *
 * SUBMS_MINIMAL_STEP_NS defines the minimum number of nanoseconds that guarantees
 * an increase in the UUID's clock precision.
 */
#if defined(__darwin__) || defined(_MSC_VER)
#define SUBMS_MINIMAL_STEP_BITS 10
#else
#define SUBMS_MINIMAL_STEP_BITS 12
#endif
#define SUBMS_BITS 12
#define SUBMS_MINIMAL_STEP_NS ((NS_PER_MS / (1 << SUBMS_MINIMAL_STEP_BITS)) + 1)

/* sortsupport for uuid */
typedef struct {
  int64 input_count; /* number of non-null values seen */
  bool estimating;   /* true if estimating cardinality */

  hyperLogLogState abbr_card; /* cardinality estimator */
} uuid_sortsupport_state;

static inline void uuid_set_version(pg_uuid_t *uuid, unsigned char version);
static inline int64 get_real_time_ns_ascending();

/*
 * Set the given UUID version and the variant bits
 */
static inline void uuid_set_version(pg_uuid_t *uuid, unsigned char version) {
  /* set version field, top four bits */
  uuid->data[6] = (uuid->data[6] & 0x0f) | (version << 4);

  /* set variant field, top two bits are 1, 0 */
  uuid->data[8] = (uuid->data[8] & 0x3f) | 0x80;
}

/*
 * Get the current timestamp with nanosecond precision for UUID generation.
 * The returned timestamp is ensured to be at least SUBMS_MINIMAL_STEP greater
 * than the previous returned timestamp (on this backend).
 */
static inline int64 get_real_time_ns_ascending() {
  static int64 previous_ns = 0;
  int64 ns;

  /* Get the current real timestamp */

#ifdef _MSC_VER
  struct timeval tmp;

  gettimeofday(&tmp, NULL);
  ns = tmp.tv_sec * NS_PER_S + tmp.tv_usec * NS_PER_US;
#else
  struct timespec tmp;

  /*
   * We don't use gettimeofday(), instead use clock_gettime() with
   * CLOCK_REALTIME where available in order to get a high-precision
   * (nanoseconds) real timestamp.
   *
   * Note while a timestamp returned by clock_gettime() with CLOCK_REALTIME
   * is nanosecond-precision on most Unix-like platforms, on some platforms
   * such as macOS it's restricted to microsecond-precision.
   */
  clock_gettime(CLOCK_REALTIME, &tmp);
  ns = tmp.tv_sec * NS_PER_S + tmp.tv_nsec;
#endif

  /* Guarantee the minimal step advancement of the timestamp */
  if (previous_ns + SUBMS_MINIMAL_STEP_NS >= ns)
    ns = previous_ns + SUBMS_MINIMAL_STEP_NS;
  previous_ns = ns;

  return ns;
}

/*
 * Generate UUID version 7 per RFC 9562, with the given timestamp.
 *
 * UUID version 7 consists of a Unix timestamp in milliseconds (48 bits) and
 * 74 random bits, excluding the required version and variant bits. To ensure
 * monotonicity in scenarios of high-frequency UUID generation, we employ the
 * method "Replace Leftmost Random Bits with Increased Clock Precision (Method 3)",
 * described in the RFC. This method utilizes 12 bits from the "rand_a" bits
 * to store a 1/4096 (or 2^12) fraction of sub-millisecond precision.
 *
 * ns is a number of nanoseconds since start of the UNIX epoch. This value is
 * used for time-dependent bits of UUID.
 */
static pg_uuid_t *generate_uuidv7(int64 ns) {
  pg_uuid_t *uuid = palloc(UUID_LEN);
  int64 unix_ts_ms;
  int32 increased_clock_precision;

  unix_ts_ms = ns / NS_PER_MS;

  /* Fill in time part */
  uuid->data[0] = (unsigned char)(unix_ts_ms >> 40);
  uuid->data[1] = (unsigned char)(unix_ts_ms >> 32);
  uuid->data[2] = (unsigned char)(unix_ts_ms >> 24);
  uuid->data[3] = (unsigned char)(unix_ts_ms >> 16);
  uuid->data[4] = (unsigned char)(unix_ts_ms >> 8);
  uuid->data[5] = (unsigned char)unix_ts_ms;

  /*
   * sub-millisecond timestamp fraction (SUBMS_BITS bits, not
   * SUBMS_MINIMAL_STEP_BITS)
   */
  increased_clock_precision = ((ns % NS_PER_MS) * (1 << SUBMS_BITS)) / NS_PER_MS;

  /* Fill the increased clock precision to "rand_a" bits */
  uuid->data[6] = (unsigned char)(increased_clock_precision >> 8);
  uuid->data[7] = (unsigned char)(increased_clock_precision);

  /* fill everything after the increased clock precision with random bytes */
  if (!pg_strong_random(&uuid->data[8], UUID_LEN - 8))
    ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR), errmsg("could not generate random values")));

#if SUBMS_MINIMAL_STEP_BITS == 10

  /*
   * On systems that have only 10 bits of sub-ms precision,  2 least
   * significant are dependent on other time-specific bits, and they do not
   * contribute to uniqueness. To make these bit random we mix in two bits
   * from CSPRNG. SUBMS_MINIMAL_STEP is chosen so that we still guarantee
   * monotonicity despite altering these bits.
   */
  uuid->data[7] = uuid->data[7] ^ (uuid->data[8] >> 6);
#endif

  /*
   * Set magic numbers for a "version 7" (pseudorandom) UUID and variant,
   * see https://www.rfc-editor.org/rfc/rfc9562#name-version-field
   */
  uuid_set_version(uuid, 7);

  return uuid;
}

/*
 * Generate UUID version 7 with the current timestamp.
 */
Datum uuidv7_(PG_FUNCTION_ARGS) {
  pg_uuid_t *uuid = generate_uuidv7(get_real_time_ns_ascending());

  PG_RETURN_UUID_P(uuid);
}

/*
 * Similar to uuidv7() but with the timestamp adjusted by the given interval.
 */
Datum uuidv7_interval_(PG_FUNCTION_ARGS) {
  Interval *shift = PG_GETARG_INTERVAL_P(0);
  TimestampTz ts;
  pg_uuid_t *uuid;
  int64 ns = get_real_time_ns_ascending();

  /*
   * Shift the current timestamp by the given interval. To calculate time
   * shift correctly, we convert the UNIX epoch to TimestampTz and use
   * timestamptz_pl_interval(). Since this calculation is done with
   * microsecond precision, we carry nanoseconds from original ns value to
   * shifted ns value.
   */

  ts = (TimestampTz)(ns / NS_PER_US) -
       (POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY * USECS_PER_SEC;

  /* Compute time shift */
  ts = DatumGetTimestampTz(DirectFunctionCall2(timestamptz_pl_interval, TimestampTzGetDatum(ts),
                                               IntervalPGetDatum(shift)));

  /*
   * Convert a TimestampTz value back to an UNIX epoch and back nanoseconds.
   */
  ns = (ts + (POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY * USECS_PER_SEC) * NS_PER_US +
       ns % NS_PER_US;

  /* Generate an UUIDv7 */
  uuid = generate_uuidv7(ns);

  PG_RETURN_UUID_P(uuid);
}

/*
 * Start of a Gregorian epoch == date2j(1582,10,15)
 * We cast it to 64-bit because it's used in overflow-prone computations
 */
#define GREGORIAN_EPOCH_JDATE INT64CONST(2299161)

/*
 * Extract timestamp from UUID.
 *
 * Returns null if not RFC 9562 variant or not a version that has a timestamp.
 */
Datum uuid_extract_timestamp_(PG_FUNCTION_ARGS) {
  pg_uuid_t *uuid = PG_GETARG_UUID_P(0);
  int version;
  uint64 tms;
  TimestampTz ts;

  /* check if RFC 9562 variant */
  if ((uuid->data[8] & 0xc0) != 0x80)
    PG_RETURN_NULL();

  version = uuid->data[6] >> 4;

  if (version == 1) {
    tms = ((uint64)uuid->data[0] << 24) + ((uint64)uuid->data[1] << 16) +
          ((uint64)uuid->data[2] << 8) + ((uint64)uuid->data[3]) + ((uint64)uuid->data[4] << 40) +
          ((uint64)uuid->data[5] << 32) + (((uint64)uuid->data[6] & 0xf) << 56) +
          ((uint64)uuid->data[7] << 48);

    /* convert 100-ns intervals to us, then adjust */
    ts = (TimestampTz)(tms / 10) -
         ((uint64)POSTGRES_EPOCH_JDATE - GREGORIAN_EPOCH_JDATE) * SECS_PER_DAY * USECS_PER_SEC;
    PG_RETURN_TIMESTAMPTZ(ts);
  }

  if (version == 7) {
    tms = (uuid->data[5]) + (((uint64)uuid->data[4]) << 8) + (((uint64)uuid->data[3]) << 16) +
          (((uint64)uuid->data[2]) << 24) + (((uint64)uuid->data[1]) << 32) +
          (((uint64)uuid->data[0]) << 40);

    /* convert ms to us, then adjust */
    ts = (TimestampTz)(tms * NS_PER_US) -
         (POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY * USECS_PER_SEC;

    PG_RETURN_TIMESTAMPTZ(ts);
  }

  /* not a timestamp-containing UUID version */
  PG_RETURN_NULL();
}

/*
 * Extract UUID version.
 *
 * Returns null if not RFC 9562 variant.
 */
Datum uuid_extract_version_(PG_FUNCTION_ARGS) {
  pg_uuid_t *uuid = PG_GETARG_UUID_P(0);
  uint16 version;

  /* check if RFC 9562 variant */
  if ((uuid->data[8] & 0xc0) != 0x80)
    PG_RETURN_NULL();

  version = uuid->data[6] >> 4;

  PG_RETURN_UINT16(version);
}