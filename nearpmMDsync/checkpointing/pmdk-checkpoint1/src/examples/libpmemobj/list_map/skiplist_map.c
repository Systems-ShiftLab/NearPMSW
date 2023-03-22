// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016, Intel Corporation */

/*
 * skiplist_map.c -- Skiplist implementation
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "skiplist_map.h"

#define SKIPLIST_LEVELS_NUM 4
#define NULL_NODE TOID_NULL(struct skiplist_map_node)
#include <x86intrin.h> 

static inline uint64_t getCycle(){
	uint32_t cycles_high, cycles_low, pid;
	asm volatile ("RDTSCP\n\t" // rdtscp into eax and edx
					"mov %%edx, %0\n\t"
					"mov %%eax, %1\n\t"
					"mov %%ecx, %2\n\t"
					:"=r" (cycles_high), "=r" (cycles_low), "=r" (pid) //store in vars
					:// no input
					:"%eax", "%edx", "%ecx" // clobbered by rdtscp
				   );
  
	return((uint64_t)cycles_high << 32) | cycles_low;
}
struct skiplist_map_entry {
	uint64_t key;
	PMEMoid value;
};

struct skiplist_map_node {
	TOID(struct skiplist_map_node) next[SKIPLIST_LEVELS_NUM];
	struct skiplist_map_entry entry;
};

/*
 * skiplist_map_create -- allocates a new skiplist instance
 */
int
skiplist_map_create(PMEMobjpool *pop, TOID(struct skiplist_map_node) *map,
	void *arg)
{
	int ret = 0;

	TX_BEGIN(pop) {
		pmemobj_tx_add_range_direct(map, sizeof(*map));
		*map = TX_ZNEW(struct skiplist_map_node);
	} TX_ONABORT {
		ret = 1;
	} TX_END
	return ret;
}

/*
 * skiplist_map_clear -- removes all elements from the map
 */
int
skiplist_map_clear(PMEMobjpool *pop, TOID(struct skiplist_map_node) map)
{
	while (!TOID_EQUALS(D_RO(map)->next[0], NULL_NODE)) {
		TOID(struct skiplist_map_node) next = D_RO(map)->next[0];
		skiplist_map_remove_free(pop, map, D_RO(next)->entry.key);
	}
	return 0;
}

/*
 * skiplist_map_destroy -- cleanups and frees skiplist instance
 */
int
skiplist_map_destroy(PMEMobjpool *pop, TOID(struct skiplist_map_node) *map)
{
	int ret = 0;

	TX_BEGIN(pop) {
		skiplist_map_clear(pop, *map);
		pmemobj_tx_add_range_direct(map, sizeof(*map));
		TX_FREE(*map);
		*map = TOID_NULL(struct skiplist_map_node);
	} TX_ONABORT {
		ret = 1;
	} TX_END
	return ret;
}

/*
 * skiplist_map_insert_new -- allocates a new object and inserts it into
 * the list
 */
int
skiplist_map_insert_new(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	uint64_t key, size_t size, unsigned type_num,
	void (*constructor)(PMEMobjpool *pop, void *ptr, void *arg),
	void *arg)
{
	int ret = 0;

	TX_BEGIN(pop) {
		PMEMoid n = pmemobj_tx_alloc(size, type_num);
		constructor(pop, pmemobj_direct(n), arg);
		skiplist_map_insert(pop, map, key, n);
	} TX_ONABORT {
		ret = 1;
	} TX_END

	return ret;
}

/*
 * skiplist_map_insert_node -- (internal) adds new node in selected place
 */
static void
skiplist_map_insert_node(TOID(struct skiplist_map_node) new_node,
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM])
{
	unsigned current_level = 0;
	do {
		TX_ADD_FIELD(path[current_level], next[current_level]);
		D_RW(new_node)->next[current_level] =
			D_RO(path[current_level])->next[current_level];
		D_RW(path[current_level])->next[current_level] = new_node;
	} while (++current_level < SKIPLIST_LEVELS_NUM && rand() % 2 == 0);
}

/*
 * skiplist_map_map_find -- (internal) returns path to searched node, or if
 * node doesn't exist, it will return path to place where key should be.
 */
static void
skiplist_map_find(uint64_t key, TOID(struct skiplist_map_node) map,
	TOID(struct skiplist_map_node) *path)
{
	int current_level;
	TOID(struct skiplist_map_node) active = map;
	for (current_level = SKIPLIST_LEVELS_NUM - 1;
			current_level >= 0; current_level--) {
		for (TOID(struct skiplist_map_node) next =
				D_RO(active)->next[current_level];
				!TOID_EQUALS(next, NULL_NODE) &&
				D_RO(next)->entry.key < key;
				next = D_RO(active)->next[current_level]) {
			active = next;
		}

		path[current_level] = active;
	}
}

/*
 * skiplist_map_insert -- inserts a new key-value pair into the map
 */
#ifdef GET_NDP_BREAKDOWN
uint64_t ulogCycles;
uint64_t waitCycles;
uint64_t resetCycles;
#endif

int
skiplist_map_insert(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	uint64_t key, PMEMoid value)
{
	int ret = 0;
	#ifdef GET_NDP_BREAKDOWN
	ulogCycles = 0;
	waitCycles = 0;
	#endif
	#ifdef GET_NDP_PERFORMENCE
	uint64_t btreetxCycles = 0; 
	uint64_t endCycles, startCycles;
	for(int i=0;i<RUN_COUNT;i++){
	#endif
	TOID(struct skiplist_map_node) new_node;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];
	#ifdef GET_NDP_PERFORMENCE
		startCycles = getCycle();
	#endif
	TX_BEGIN(pop) {
		new_node = TX_ZNEW(struct skiplist_map_node);
		D_RW(new_node)->entry.key = key;
		D_RW(new_node)->entry.value = value;

		skiplist_map_find(key, map, path);
		skiplist_map_insert_node(new_node, path);
	} TX_ONABORT {
		ret = 1;
	} TX_END
	#ifdef GET_NDP_PERFORMENCE
	endCycles = getCycle();
	btreetxCycles += endCycles - startCycles;
	}
	double totTime = ((double)btreetxCycles)/2000000000;
	printf("ctree TX/s = %f\nctree tx total time = %f\n",RUN_COUNT/totTime,totTime);
	#endif
	#ifdef GET_NDP_BREAKDOWN
	printf("ctree tx cmd issue total time = %f\n", (((double)ulogCycles)/2000000000));
	printf("ctree tx total wait time = %f\n", (((double)waitCycles)/2000000000));
	#endif
	return ret;
}

/*
 * skiplist_map_remove_free -- removes and frees an object from the list
 */
int
skiplist_map_remove_free(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	uint64_t key)
{
	int ret = 0;

	TX_BEGIN(pop) {
		PMEMoid val = skiplist_map_remove(pop, map, key);
		pmemobj_tx_free(val);
	} TX_ONABORT {
		ret = 1;
	} TX_END

	return ret;
}

/*
 * skiplist_map_remove_node -- (internal) removes selected node
 */
static void
skiplist_map_remove_node(
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM])
{
	TOID(struct skiplist_map_node) to_remove = D_RO(path[0])->next[0];
	int i;
	for (i = 0; i < SKIPLIST_LEVELS_NUM; i++) {
		if (TOID_EQUALS(D_RO(path[i])->next[i], to_remove)) {
			TX_ADD_FIELD(path[i], next[i]);
			D_RW(path[i])->next[i] = D_RO(to_remove)->next[i];
		}
	}
}

/*
 * skiplist_map_remove -- removes key-value pair from the map
 */
PMEMoid
skiplist_map_remove(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	uint64_t key)
{
	PMEMoid ret = OID_NULL;
	#ifdef GET_NDP_BREAKDOWN
	ulogCycles = 0;
	waitCycles = 0;
	#endif
	#ifdef GET_NDP_PERFORMENCE
	uint64_t btreetxCycles = 0; 
	uint64_t endCycles, startCycles;
	for(int i=0;i<RUN_COUNT;i++){
	#endif
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM];
	TOID(struct skiplist_map_node) to_remove;
	#ifdef GET_NDP_PERFORMENCE
		startCycles = getCycle();
	#endif
	TX_BEGIN(pop) {
		skiplist_map_find(key, map, path);
		to_remove = D_RO(path[0])->next[0];
		if (!TOID_EQUALS(to_remove, NULL_NODE) &&
			D_RO(to_remove)->entry.key == key) {
			ret = D_RO(to_remove)->entry.value;
			skiplist_map_remove_node(path);
		}
	} TX_ONABORT {
		ret = OID_NULL;
	} TX_END
	#ifdef GET_NDP_PERFORMENCE
	endCycles = getCycle();
	btreetxCycles += endCycles - startCycles;
	}
	double totTime = ((double)btreetxCycles)/2000000000;
	printf("ctree TX/s = %f\nctree tx total time = %f\n",RUN_COUNT/totTime,totTime);
	#endif
	#ifdef GET_NDP_BREAKDOWN
	printf("ctree tx cmd issue total time = %f\n", (((double)ulogCycles)/2000000000));
	printf("ctree tx total wait time = %f\n", (((double)waitCycles)/2000000000));
	#endif
	return ret;
}

/*
 * skiplist_map_get -- searches for a value of the key
 */
PMEMoid
skiplist_map_get(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	uint64_t key)
{
	PMEMoid ret = OID_NULL;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM], found;
	skiplist_map_find(key, map, path);
	found = D_RO(path[0])->next[0];
	if (!TOID_EQUALS(found, NULL_NODE) &&
		D_RO(found)->entry.key == key) {
		ret = D_RO(found)->entry.value;
	}
	return ret;
}

/*
 * skiplist_map_lookup -- searches if a key exists
 */
int
skiplist_map_lookup(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	uint64_t key)
{
	int ret = 0;
	TOID(struct skiplist_map_node) path[SKIPLIST_LEVELS_NUM], found;

	skiplist_map_find(key, map, path);
	found = D_RO(path[0])->next[0];
	if (!TOID_EQUALS(found, NULL_NODE) &&
		D_RO(found)->entry.key == key) {
		ret = 1;
	}
	return ret;
}

/*
 * skiplist_map_foreach -- calls function for each node on a list
 */
int
skiplist_map_foreach(PMEMobjpool *pop, TOID(struct skiplist_map_node) map,
	int (*cb)(uint64_t key, PMEMoid value, void *arg), void *arg)
{
	TOID(struct skiplist_map_node) next = map;
	while (!TOID_EQUALS(D_RO(next)->next[0], NULL_NODE)) {
		next = D_RO(next)->next[0];
		cb(D_RO(next)->entry.key, D_RO(next)->entry.value, arg);
	}
	return 0;
}

/*
 * skiplist_map_is_empty -- checks whether the list map is empty
 */
int
skiplist_map_is_empty(PMEMobjpool *pop, TOID(struct skiplist_map_node) map)
{
	return TOID_IS_NULL(D_RO(map)->next[0]);
}

/*
 * skiplist_map_check -- check if given persistent object is a skiplist
 */
int
skiplist_map_check(PMEMobjpool *pop, TOID(struct skiplist_map_node) map)
{
	return TOID_IS_NULL(map) || !TOID_VALID(map);
}
