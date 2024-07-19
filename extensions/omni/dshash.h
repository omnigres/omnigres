// clang-format off
/*-------------------------------------------------------------------------
 *
 * dshash.h
 *	  Concurrent hash tables backed by dynamic shared memory areas.
 *
 * Portions Copyright (c) 1996-2023, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/include/lib/dshash.h
 *
 *-------------------------------------------------------------------------
 */

#if PG_MAJORVERSION_NUM < 15

#include "utils/dsa.h"

#include <lib/dshash.h>

/*
 * An item in the hash table.  This wraps the user's entry object in an
 * envelop that holds a pointer back to the bucket and a pointer to the next
 * item in the bucket.
 */
struct dshash_table_item
{
	/* The next item in the same bucket. */
	dsa_pointer next;
	/* The hashed key, to avoid having to recompute it. */
	dshash_hash hash;
	/* The user's entry object follows here.  See ENTRY_FROM_ITEM(item). */
};


/*
 * Sequential scan state. The detail is exposed to let users know the storage
 * size but it should be considered as an opaque type by callers.
 */
typedef struct dshash_seq_status
{
	dshash_table *hash_table;	/* dshash table working on */
	int			curbucket;		/* bucket number we are at */
	int			nbuckets;		/* total number of buckets in the dshash */
	dshash_table_item *curitem; /* item we are currently at */
	dsa_pointer pnextitem;		/* dsa-pointer to the next item */
	int			curpartition;	/* partition number we are at */
	bool		exclusive;		/* locking mode */
} dshash_seq_status;

/* seq scan support */
extern void dshash_seq_init(dshash_seq_status *status, dshash_table *hash_table,
							bool exclusive);
extern void *dshash_seq_next(dshash_seq_status *status);
extern void dshash_seq_term(dshash_seq_status *status);
extern void dshash_delete_current(dshash_seq_status *status);

// clang-format on
#endif