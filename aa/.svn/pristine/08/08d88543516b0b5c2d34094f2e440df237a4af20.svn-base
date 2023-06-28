#include "qcommon.h"

// Least-recently-used cache algorithm implementation.

#define LRU_CACHE_MAX_SIZE 256

typedef struct
{
	// used_list is circular and double-linked, free_list is neither
	struct
	{
		int prevs[LRU_CACHE_MAX_SIZE], nexts[LRU_CACHE_MAX_SIZE], head, tail;
	} used_list;
	struct 
	{
		int prevs[LRU_CACHE_MAX_SIZE], tail;
	} free_list;
	size_t keys[LRU_CACHE_MAX_SIZE];
	int size;
} lru_cache_t;

typedef enum
{
	lru_keep, lru_dontkeep
} lru_insertion_type_t;

typedef struct
{
	qboolean present;
	int slot;
} lru_result_t;

lru_result_t lru_lookup (lru_cache_t *cache, lru_insertion_type_t insertion, size_t key, int slot)
{
	lru_result_t result;

	assert (key);

	// first-time lookup, slot is unknown, so search for it manually
	if (slot < 0)
	{
		slot = cache->used_list.head;
		while (cache->keys[slot] != key && slot != cache->used_list.tail)
			slot = cache->used_list.nexts[slot];
	}

	result.present = cache->keys[slot] == key;
	if (!result.present) // not in cache
	{
		if (cache->free_list.tail != -1) // don't have to evict anything
		{
			slot = cache->free_list.tail;
			cache->free_list.tail = cache->free_list.prevs[slot];
		}
		else // evict least recently used
		{
			slot = cache->used_list.tail;
		}
		cache->keys[slot] = key;
	}

	if (insertion == lru_keep) // move to head of list
	{
		if (cache->used_list.tail == cache->used_list.head && !cache->keys[cache->used_list.head])
		{
			cache->used_list.head = cache->used_list.tail =
			cache->used_list.nexts[slot] = cache->used_list.prevs[slot] = slot;
		}
		else
		{
			int old_prev, old_next;

			old_prev = cache->used_list.prevs[slot];
			old_next = cache->used_list.nexts[slot];

			cache->used_list.nexts[old_prev] = old_next;
			cache->used_list.prevs[old_next] = old_prev;

			cache->used_list.tail = cache->used_list.prevs[slot] = cache->used_list.prevs[cache->used_list.head];
			cache->used_list.nexts[slot] = cache->used_list.head;
			cache->used_list.prevs[cache->used_list.head] = slot;
			cache->used_list.nexts[cache->used_list.tail] = slot;
			cache->used_list.head = slot;
		}
	}

	result.slot = slot;
	return result;
}

void lru_initialize (lru_cache_t *cache, int size)
{
	int i;

	memset (cache, 0, sizeof (*cache));

	assert (size <= LRU_CACHE_MAX_SIZE);
	cache->size = size;

	for (i = 0; i < size; i++)
		cache->free_list.prevs[i] = i - 1;
	cache->free_list.tail = size - 1;
}



#if 0 // unit tests-- re-run these if you ever modify the cache code.
static void print_cache_state (lru_cache_t *cache)
{
	int i;

	printf ("free slots: ");
	i = cache->free_list.tail;
	while (i != -1)
	{
		printf ("[%03d] = %lu  ", i, cache->keys[i]);
		i = cache->free_list.prevs[i];
	}
	printf ("\n");

	printf ("used slots: ");
	i = cache->used_list.head;
	do {
		if (i == cache->used_list.head)
			putchar ('h');
		else
			putchar ('_');
		if (i == cache->used_list.tail)
			putchar ('t');
		else
			putchar ('_');
		printf ("[%03d] = %lu  ", i, cache->keys[i]);
		i = cache->used_list.nexts[i];
	} while (i != cache->used_list.head);
	printf ("\n");
}

static void check_cache_state (lru_cache_t *cache)
{
	int count, i;

	count = 0;
	i = cache->free_list.tail;

	while (i != -1)
	{
		i = cache->free_list.prevs[i];
		count++;
		assert (count <= LRU_CACHE_MAX_SIZE);
	}

	if (cache->used_list.tail != cache->used_list.head || cache->keys[cache->used_list.head])
	{
		i = cache->used_list.head;
		do {
			i = cache->used_list.nexts[i];
			count++;
			assert (count <= LRU_CACHE_MAX_SIZE);
		} while (i != cache->used_list.head);
	}

	assert (count == LRU_CACHE_MAX_SIZE);
}

int main (int argc, char *argv[])
{
	int i, found;

	lru_cache_t cache;
	int slots[LRU_CACHE_MAX_SIZE];

	lru_initialize (&cache, LRU_CACHE_MAX_SIZE);
	check_cache_state (&cache);

	for (i = 1; i <= LRU_CACHE_MAX_SIZE; i++)
	{
		lru_result_t result = lru_lookup (&cache, lru_keep, (size_t)i, -1);
		check_cache_state (&cache);
		slots[i-1] = result.slot;
		assert (!result.present);
	}

	for (i = 1; i <= LRU_CACHE_MAX_SIZE; i++)
	{
		lru_result_t result = lru_lookup (&cache, lru_keep, (size_t)i, -1);
		check_cache_state (&cache);
		assert (slots[i - 1] == result.slot);
		assert (result.present);
	}

	for (i = 1; i <= LRU_CACHE_MAX_SIZE; i++)
	{
		lru_result_t result = lru_lookup (&cache, lru_keep, (size_t)(i + LRU_CACHE_MAX_SIZE + 1), -1);
		check_cache_state (&cache);
		slots[i-1] = result.slot;
		assert (!result.present);
	}

	for (i = 1; i <= LRU_CACHE_MAX_SIZE; i++)
	{
		lru_result_t result = lru_lookup (&cache, lru_keep, (size_t)(i + LRU_CACHE_MAX_SIZE + 1), -1);
		check_cache_state (&cache);
		assert (slots[i - 1] == result.slot);
		assert (result.present);
	}

	for (i = 1; i <= LRU_CACHE_MAX_SIZE/2; i++)
	{
		lru_result_t result = lru_lookup (&cache, lru_keep, (size_t)(i + LRU_CACHE_MAX_SIZE + 1), -1);
		check_cache_state (&cache);
		assert (slots[i - 1] == result.slot);
		assert (result.present);
	}

	for (i = LRU_CACHE_MAX_SIZE/2 + 1; i <= LRU_CACHE_MAX_SIZE; i++)
	{
		lru_result_t result = lru_lookup (&cache, lru_keep, (size_t)i, -1);
		check_cache_state (&cache);
		slots[i-1] = result.slot;
		assert (!result.present);
	}

	for (i = 1; i <= LRU_CACHE_MAX_SIZE/2; i++)
	{
		lru_result_t result = lru_lookup (&cache, lru_keep, (size_t)(i + LRU_CACHE_MAX_SIZE + 1), -1);
		assert (slots[i - 1] == result.slot);
		assert (result.present);
	}

	for (i = LRU_CACHE_MAX_SIZE/2 + 1; i <= LRU_CACHE_MAX_SIZE; i++)
	{
		lru_result_t result = lru_lookup (&cache, lru_keep, (size_t)(i + LRU_CACHE_MAX_SIZE + 1), -1);
		check_cache_state (&cache);
		slots[i-1] = result.slot;
		assert (!result.present);
	}

	for (i = LRU_CACHE_MAX_SIZE/2 + 1; i <= LRU_CACHE_MAX_SIZE; i++)
	{
		lru_result_t result = lru_lookup (&cache, lru_keep, (size_t)i, -1);
		check_cache_state (&cache);
		slots[i-1] = result.slot;
		assert (!result.present);
	}

	for (i = 1; i <= LRU_CACHE_MAX_SIZE; i++)
	{
		lru_result_t result = lru_lookup (&cache, lru_dontkeep, (size_t)(i + 2 * LRU_CACHE_MAX_SIZE + 1), -1);
		check_cache_state (&cache);
		slots[i-1] = result.slot;
		assert (!result.present);
	}

	{
		lru_result_t result = lru_lookup (&cache, lru_dontkeep, (size_t)(LRU_CACHE_MAX_SIZE + 2 * LRU_CACHE_MAX_SIZE + 1), -1);
		check_cache_state (&cache);
		assert (result.present);
	}

	found = 0;
	for (i = LRU_CACHE_MAX_SIZE; i > LRU_CACHE_MAX_SIZE/2; i--)
	{
		lru_result_t result = lru_lookup (&cache, lru_keep, (size_t)(i + LRU_CACHE_MAX_SIZE + 1), -1);
		check_cache_state (&cache);
		slots[i-1] = result.slot;
		found += result.present;
	}
	assert (found == LRU_CACHE_MAX_SIZE/2 - 1);

	{
		lru_result_t result = lru_lookup (&cache, lru_dontkeep, (size_t)(LRU_CACHE_MAX_SIZE + 2 * LRU_CACHE_MAX_SIZE + 1), -1);
		check_cache_state (&cache);
		assert (!result.present);
	}
}
#endif
