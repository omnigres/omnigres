// clang-format off
/*-------------------------------------------------------------------------
 *
 * dshash.c
 *	  Concurrent hash tables backed by dynamic shared memory areas.
 *
 * This is an open hashing hash table, with a linked list at each table
 * entry.  It supports dynamic resizing, as required to prevent the linked
 * lists from growing too long on average.  Currently, only growing is
 * supported: the hash table never becomes smaller.
 *
 * To deal with concurrency, it has a fixed size set of partitions, each of
 * which is independently locked.  Each bucket maps to a partition; so insert,
 * find and iterate operations normally only acquire one lock.  Therefore,
 * good concurrency is achieved whenever such operations don't collide at the
 * lock partition level.  However, when a resize operation begins, all
 * partition locks must be acquired simultaneously for a brief period.  This
 * is only expected to happen a small number of times until a stable size is
 * found, since growth is geometric.
 *
 * Future versions may support iterators and incremental resizing; for now
 * the implementation is minimalist.
 *
 * Portions Copyright (c) 1996-2023, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/lib/dshash.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#if PG_MAJORVERSION_NUM < 15

#include "dshash.h"

#include "common/hashfn.h"
#include "storage/ipc.h"
#include "storage/lwlock.h"
#include "utils/dsa.h"
#include "utils/memutils.h"

/*
 * The number of partitions for locking purposes.  This is set to match
 * NUM_BUFFER_PARTITIONS for now, on the basis that whatever's good enough for
 * the buffer pool must be good enough for any other purpose.  This could
 * become a runtime parameter in future.
 */
#define DSHASH_NUM_PARTITIONS_LOG2 7
#define DSHASH_NUM_PARTITIONS (1 << DSHASH_NUM_PARTITIONS_LOG2)

/* A magic value used to identify our hash tables. */
#define DSHASH_MAGIC 0x75ff6a20

/*
 * Tracking information for each lock partition.  Initially, each partition
 * corresponds to one bucket, but each time the hash table grows, the buckets
 * covered by each partition split so the number of buckets covered doubles.
 *
 * We might want to add padding here so that each partition is on a different
 * cache line, but doing so would bloat this structure considerably.
 */
typedef struct dshash_partition
{
	LWLock		lock;			/* Protects all buckets in this partition. */
	size_t		count;			/* # of items in this partition's buckets */
} dshash_partition;

/*
 * The head object for a hash table.  This will be stored in dynamic shared
 * memory.
 */
typedef struct dshash_table_control
{
	dshash_table_handle handle;
	uint32		magic;
	dshash_partition partitions[DSHASH_NUM_PARTITIONS];
	int			lwlock_tranche_id;

	/*
	 * The following members are written to only when ALL partitions locks are
	 * held.  They can be read when any one partition lock is held.
	 */

	/* Number of buckets expressed as power of 2 (8 = 256 buckets). */
	size_t		size_log2;		/* log2(number of buckets) */
	dsa_pointer buckets;		/* current bucket array */
} dshash_table_control;

/*
 * Per-backend state for a dynamic hash table.
 */
struct dshash_table
{
	dsa_area   *area;			/* Backing dynamic shared memory area. */
	dshash_parameters params;	/* Parameters. */
	void	   *arg;			/* User-supplied data pointer. */
	dshash_table_control *control;	/* Control object in DSM. */
	dsa_pointer *buckets;		/* Current bucket pointers in DSM. */
	size_t		size_log2;		/* log2(number of buckets) */
};

/* Given a pointer to an item, find the entry (user data) it holds. */
#define ENTRY_FROM_ITEM(item) \
	((char *)(item) + MAXALIGN(sizeof(dshash_table_item)))

/* Given a pointer to an entry, find the item that holds it. */
#define ITEM_FROM_ENTRY(entry)											\
	((dshash_table_item *)((char *)(entry) -							\
							 MAXALIGN(sizeof(dshash_table_item))))

/* How many resize operations (bucket splits) have there been? */
#define NUM_SPLITS(size_log2)					\
	(size_log2 - DSHASH_NUM_PARTITIONS_LOG2)

/* How many buckets are there in a given size? */
#define NUM_BUCKETS(size_log2)		\
	(((size_t) 1) << (size_log2))

/* How many buckets are there in each partition at a given size? */
#define BUCKETS_PER_PARTITION(size_log2)		\
	(((size_t) 1) << NUM_SPLITS(size_log2))

/* Max entries before we need to grow.  Half + quarter = 75% load factor. */
#define MAX_COUNT_PER_PARTITION(hash_table)				\
	(BUCKETS_PER_PARTITION(hash_table->size_log2) / 2 + \
	 BUCKETS_PER_PARTITION(hash_table->size_log2) / 4)

/* Choose partition based on the highest order bits of the hash. */
#define PARTITION_FOR_HASH(hash)										\
	(hash >> ((sizeof(dshash_hash) * CHAR_BIT) - DSHASH_NUM_PARTITIONS_LOG2))

/*
 * Find the bucket index for a given hash and table size.  Each time the table
 * doubles in size, the appropriate bucket for a given hash value doubles and
 * possibly adds one, depending on the newly revealed bit, so that all buckets
 * are split.
 */
#define BUCKET_INDEX_FOR_HASH_AND_SIZE(hash, size_log2)		\
	(hash >> ((sizeof(dshash_hash) * CHAR_BIT) - (size_log2)))

/* The index of the first bucket in a given partition. */
#define BUCKET_INDEX_FOR_PARTITION(partition, size_log2)	\
	((partition) << NUM_SPLITS(size_log2))

/* Choose partition based on bucket index. */
#define PARTITION_FOR_BUCKET_INDEX(bucket_idx, size_log2)				\
	((bucket_idx) >> NUM_SPLITS(size_log2))

/* The head of the active bucket for a given hash value (lvalue). */
#define BUCKET_FOR_HASH(hash_table, hash)								\
	(hash_table->buckets[												\
		BUCKET_INDEX_FOR_HASH_AND_SIZE(hash,							\
									   hash_table->size_log2)])

static void delete_item(dshash_table *hash_table,
						dshash_table_item *item);
static void resize(dshash_table *hash_table, size_t new_size_log2);
static inline void ensure_valid_bucket_pointers(dshash_table *hash_table);
static inline dshash_table_item *find_in_bucket(dshash_table *hash_table,
												const void *key,
												dsa_pointer item_pointer);
static void insert_item_into_bucket(dshash_table *hash_table,
									dsa_pointer item_pointer,
									dshash_table_item *item,
									dsa_pointer *bucket);
static dshash_table_item *insert_into_bucket(dshash_table *hash_table,
											 const void *key,
											 dsa_pointer *bucket);
static bool delete_key_from_bucket(dshash_table *hash_table,
								   const void *key,
								   dsa_pointer *bucket_head);
static bool delete_item_from_bucket(dshash_table *hash_table,
									dshash_table_item *item,
									dsa_pointer *bucket_head);
static inline dshash_hash hash_key(dshash_table *hash_table, const void *key);
static inline bool equal_keys(dshash_table *hash_table,
							  const void *a, const void *b);

#define PARTITION_LOCK(hash_table, i)			\
	(&(hash_table)->control->partitions[(i)].lock)

#define ASSERT_NO_PARTITION_LOCKS_HELD_BY_ME(hash_table) \
	Assert(!LWLockAnyHeldByMe(&(hash_table)->control->partitions[0].lock, \
		   DSHASH_NUM_PARTITIONS, sizeof(dshash_partition)))

/*
 * Sequentially scan through dshash table and return all the elements one by
 * one, return NULL when all elements have been returned.
 *
 * dshash_seq_term needs to be called when a scan finished.  The caller may
 * delete returned elements midst of a scan by using dshash_delete_current()
 * if exclusive = true.
 */
void
dshash_seq_init(dshash_seq_status *status, dshash_table *hash_table,
				bool exclusive)
{
	status->hash_table = hash_table;
	status->curbucket = 0;
	status->nbuckets = 0;
	status->curitem = NULL;
	status->pnextitem = InvalidDsaPointer;
	status->curpartition = -1;
	status->exclusive = exclusive;
}

/*
 * Returns the next element.
 *
 * Returned elements are locked and the caller may not release the lock. It is
 * released by future calls to dshash_seq_next() or dshash_seq_term().
 */
void *
dshash_seq_next(dshash_seq_status *status)
{
	dsa_pointer next_item_pointer;

	/*
	 * Not yet holding any partition locks. Need to determine the size of the
	 * hash table, it could have been resized since we were looking last.
	 * Since we iterate in partition order, we can start by unconditionally
	 * lock partition 0.
	 *
	 * Once we hold the lock, no resizing can happen until the scan ends. So
	 * we don't need to repeatedly call ensure_valid_bucket_pointers().
	 */
	if (status->curpartition == -1)
	{
		Assert(status->curbucket == 0);
		ASSERT_NO_PARTITION_LOCKS_HELD_BY_ME(status->hash_table);

		status->curpartition = 0;

		LWLockAcquire(PARTITION_LOCK(status->hash_table,
									 status->curpartition),
					  status->exclusive ? LW_EXCLUSIVE : LW_SHARED);

		ensure_valid_bucket_pointers(status->hash_table);

		status->nbuckets =
			NUM_BUCKETS(status->hash_table->control->size_log2);
		next_item_pointer = status->hash_table->buckets[status->curbucket];
	}
	else
		next_item_pointer = status->pnextitem;

	Assert(LWLockHeldByMeInMode(PARTITION_LOCK(status->hash_table,
											   status->curpartition),
								status->exclusive ? LW_EXCLUSIVE : LW_SHARED));

	/* Move to the next bucket if we finished the current bucket */
	while (!DsaPointerIsValid(next_item_pointer))
	{
		int			next_partition;

		if (++status->curbucket >= status->nbuckets)
		{
			/* all buckets have been scanned. finish. */
			return NULL;
		}

		/* Check if move to the next partition */
		next_partition =
			PARTITION_FOR_BUCKET_INDEX(status->curbucket,
									   status->hash_table->size_log2);

		if (status->curpartition != next_partition)
		{
			/*
			 * Move to the next partition. Lock the next partition then
			 * release the current, not in the reverse order to avoid
			 * concurrent resizing.  Avoid dead lock by taking lock in the
			 * same order with resize().
			 */
			LWLockAcquire(PARTITION_LOCK(status->hash_table,
										 next_partition),
						  status->exclusive ? LW_EXCLUSIVE : LW_SHARED);
			LWLockRelease(PARTITION_LOCK(status->hash_table,
										 status->curpartition));
			status->curpartition = next_partition;
		}

		next_item_pointer = status->hash_table->buckets[status->curbucket];
	}

	status->curitem =
		dsa_get_address(status->hash_table->area, next_item_pointer);

	/*
	 * The caller may delete the item. Store the next item in case of
	 * deletion.
	 */
	status->pnextitem = status->curitem->next;

	return ENTRY_FROM_ITEM(status->curitem);
}

/*
 * Terminates the seqscan and release all locks.
 *
 * Needs to be called after finishing or when exiting a seqscan.
 */
void
dshash_seq_term(dshash_seq_status *status)
{
	if (status->curpartition >= 0)
		LWLockRelease(PARTITION_LOCK(status->hash_table, status->curpartition));
}

/*
 * Remove the current entry of the seq scan.
 */
void
dshash_delete_current(dshash_seq_status *status)
{
	dshash_table *hash_table = status->hash_table;
	dshash_table_item *item = status->curitem;
	size_t		partition PG_USED_FOR_ASSERTS_ONLY;

	partition = PARTITION_FOR_HASH(item->hash);

	Assert(status->exclusive);
	Assert(hash_table->control->magic == DSHASH_MAGIC);
	Assert(LWLockHeldByMeInMode(PARTITION_LOCK(hash_table, partition),
								LW_EXCLUSIVE));

	delete_item(hash_table, item);
}


/*
 * Delete a locked item to which we have a pointer.
 */
static void
delete_item(dshash_table *hash_table, dshash_table_item *item)
{
	size_t		hash = item->hash;
	size_t		partition = PARTITION_FOR_HASH(hash);

	Assert(LWLockHeldByMe(PARTITION_LOCK(hash_table, partition)));

	if (delete_item_from_bucket(hash_table, item,
								&BUCKET_FOR_HASH(hash_table, hash)))
	{
		Assert(hash_table->control->partitions[partition].count > 0);
		--hash_table->control->partitions[partition].count;
	}
	else
	{
		Assert(false);
	}
}

/*
 * Grow the hash table if necessary to the requested number of buckets.  The
 * requested size must be double some previously observed size.
 *
 * Must be called without any partition lock held.
 */
static void
resize(dshash_table *hash_table, size_t new_size_log2)
{
	dsa_pointer old_buckets;
	dsa_pointer new_buckets_shared;
	dsa_pointer *new_buckets;
	size_t		size;
	size_t		new_size = ((size_t) 1) << new_size_log2;
	size_t		i;

	/*
	 * Acquire the locks for all lock partitions.  This is expensive, but we
	 * shouldn't have to do it many times.
	 */
	for (i = 0; i < DSHASH_NUM_PARTITIONS; ++i)
	{
		Assert(!LWLockHeldByMe(PARTITION_LOCK(hash_table, i)));

		LWLockAcquire(PARTITION_LOCK(hash_table, i), LW_EXCLUSIVE);
		if (i == 0 && hash_table->control->size_log2 >= new_size_log2)
		{
			/*
			 * Another backend has already increased the size; we can avoid
			 * obtaining all the locks and return early.
			 */
			LWLockRelease(PARTITION_LOCK(hash_table, 0));
			return;
		}
	}

	Assert(new_size_log2 == hash_table->control->size_log2 + 1);

	/* Allocate the space for the new table. */
	new_buckets_shared = dsa_allocate0(hash_table->area,
									   sizeof(dsa_pointer) * new_size);
	new_buckets = dsa_get_address(hash_table->area, new_buckets_shared);

	/*
	 * We've allocated the new bucket array; all that remains to do now is to
	 * reinsert all items, which amounts to adjusting all the pointers.
	 */
	size = ((size_t) 1) << hash_table->control->size_log2;
	for (i = 0; i < size; ++i)
	{
		dsa_pointer item_pointer = hash_table->buckets[i];

		while (DsaPointerIsValid(item_pointer))
		{
			dshash_table_item *item;
			dsa_pointer next_item_pointer;

			item = dsa_get_address(hash_table->area, item_pointer);
			next_item_pointer = item->next;
			insert_item_into_bucket(hash_table, item_pointer, item,
									&new_buckets[BUCKET_INDEX_FOR_HASH_AND_SIZE(item->hash,
																				new_size_log2)]);
			item_pointer = next_item_pointer;
		}
	}

	/* Swap the hash table into place and free the old one. */
	old_buckets = hash_table->control->buckets;
	hash_table->control->buckets = new_buckets_shared;
	hash_table->control->size_log2 = new_size_log2;
	hash_table->buckets = new_buckets;
	dsa_free(hash_table->area, old_buckets);

	/* Release all the locks. */
	for (i = 0; i < DSHASH_NUM_PARTITIONS; ++i)
		LWLockRelease(PARTITION_LOCK(hash_table, i));
}

/*
 * Make sure that our backend-local bucket pointers are up to date.  The
 * caller must have locked one lock partition, which prevents resize() from
 * running concurrently.
 */
static inline void
ensure_valid_bucket_pointers(dshash_table *hash_table)
{
	if (hash_table->size_log2 != hash_table->control->size_log2)
	{
		hash_table->buckets = dsa_get_address(hash_table->area,
											  hash_table->control->buckets);
		hash_table->size_log2 = hash_table->control->size_log2;
	}
}

/*
 * Scan a locked bucket for a match, using the provided compare function.
 */
static inline dshash_table_item *
find_in_bucket(dshash_table *hash_table, const void *key,
			   dsa_pointer item_pointer)
{
	while (DsaPointerIsValid(item_pointer))
	{
		dshash_table_item *item;

		item = dsa_get_address(hash_table->area, item_pointer);
		if (equal_keys(hash_table, key, ENTRY_FROM_ITEM(item)))
			return item;
		item_pointer = item->next;
	}
	return NULL;
}

/*
 * Insert an already-allocated item into a bucket.
 */
static void
insert_item_into_bucket(dshash_table *hash_table,
						dsa_pointer item_pointer,
						dshash_table_item *item,
						dsa_pointer *bucket)
{
	Assert(item == dsa_get_address(hash_table->area, item_pointer));

	item->next = *bucket;
	*bucket = item_pointer;
}

/*
 * Allocate space for an entry with the given key and insert it into the
 * provided bucket.
 */
static dshash_table_item *
insert_into_bucket(dshash_table *hash_table,
				   const void *key,
				   dsa_pointer *bucket)
{
	dsa_pointer item_pointer;
	dshash_table_item *item;

	item_pointer = dsa_allocate(hash_table->area,
								hash_table->params.entry_size +
								MAXALIGN(sizeof(dshash_table_item)));
	item = dsa_get_address(hash_table->area, item_pointer);
	memcpy(ENTRY_FROM_ITEM(item), key, hash_table->params.key_size);
	insert_item_into_bucket(hash_table, item_pointer, item, bucket);
	return item;
}

/*
 * Search a bucket for a matching key and delete it.
 */
static bool
delete_key_from_bucket(dshash_table *hash_table,
					   const void *key,
					   dsa_pointer *bucket_head)
{
	while (DsaPointerIsValid(*bucket_head))
	{
		dshash_table_item *item;

		item = dsa_get_address(hash_table->area, *bucket_head);

		if (equal_keys(hash_table, key, ENTRY_FROM_ITEM(item)))
		{
			dsa_pointer next;

			next = item->next;
			dsa_free(hash_table->area, *bucket_head);
			*bucket_head = next;

			return true;
		}
		bucket_head = &item->next;
	}
	return false;
}

/*
 * Delete the specified item from the bucket.
 */
static bool
delete_item_from_bucket(dshash_table *hash_table,
						dshash_table_item *item,
						dsa_pointer *bucket_head)
{
	while (DsaPointerIsValid(*bucket_head))
	{
		dshash_table_item *bucket_item;

		bucket_item = dsa_get_address(hash_table->area, *bucket_head);

		if (bucket_item == item)
		{
			dsa_pointer next;

			next = item->next;
			dsa_free(hash_table->area, *bucket_head);
			*bucket_head = next;
			return true;
		}
		bucket_head = &bucket_item->next;
	}
	return false;
}

/*
 * Compute the hash value for a key.
 */
static inline dshash_hash
hash_key(dshash_table *hash_table, const void *key)
{
	return hash_table->params.hash_function(key,
											hash_table->params.key_size,
											hash_table->arg);
}

/*
 * Check whether two keys compare equal.
 */
static inline bool
equal_keys(dshash_table *hash_table, const void *a, const void *b)
{
	return hash_table->params.compare_function(a, b,
											   hash_table->params.key_size,
											   hash_table->arg) == 0;
}
#endif
// clang-format on
