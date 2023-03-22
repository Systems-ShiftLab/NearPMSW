// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2015-2018, Intel Corporation */

/*
 * data_store.c -- tree_map example usage
 */

#include <ex_common.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "map.h"
#include "map_ctree.h"
#include "map_btree.h"
#include "map_rbtree.h"
#include "map_hashmap_atomic.h"
#include "map_hashmap_tx.h"
#include "map_hashmap_rp.h"
#include "map_skiplist.h"

POBJ_LAYOUT_BEGIN(data_store);
	POBJ_LAYOUT_ROOT(data_store, struct store_root);
	POBJ_LAYOUT_TOID(data_store, struct store_item);
POBJ_LAYOUT_END(data_store);


#define size 1000000
unsigned num_op = 20000;
unsigned g_seed = 1312515;

struct element{
	uint64_t ele_val[60];
};

struct store_item {
	//struct element val[size+2];
	uint64_t val[size+2];
};

struct store_root{
	TOID(struct store_item) arr;
};

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

//inline unsigned fastrand() {
//	g_seed = (179423891 * g_seed + 2038073749); 
//	return (g_seed >> 8) & 0x7FFFFFFF;
//} 



int main(int argc, const char *argv[]) {

	const char *path = argv[1];

	PMEMobjpool *pop;

	if (file_exists(path) != 0) {
		if ((pop = pmemobj_create(path, POBJ_LAYOUT_NAME(data_store),
			(sizeof(struct element)*(size+2)), 0666)) == NULL) {
			perror("failed to create pool\n");
			return 1;
		}
	} else {
		if ((pop = pmemobj_open(path,
				POBJ_LAYOUT_NAME(data_store))) == NULL) {
			perror("failed to open pool\n");
			return 1;
		}
	}

	TOID(struct store_root) root = POBJ_ROOT(pop, struct store_root);
	struct store_root *rootp = D_RW(root);

	TX_BEGIN(pop) {	
		TX_ADD(root);
		TOID(struct store_item) arr = TX_NEW(struct store_item);
//		for(int i=0; i<size; i++)
//			D_RW(arr)->val[i] = i;
		D_RW(root)->arr = arr;
	} TX_END

	uint64_t endCycles, startCycles,totalCycles, tmp;
	unsigned src, dest;
	//struct element tmp;
		
	startCycles = getCycle();
	for (int i = 0; i < num_op; i++) {
		TX_BEGIN(pop) {
			g_seed = (179423891 * g_seed + 2038073749);
			src =  ((g_seed >> 8) & 0x7FFFFFFF) % size;
			g_seed = (179423891 * g_seed + 2038073749);
			dest = ((g_seed >> 8) & 0x7FFFFFFF) % size;	
			TX_ADD_DIRECT(&(D_RW(rootp->arr)->val[src]));
			TX_ADD_DIRECT(&(D_RW(rootp->arr)->val[dest]));
			tmp = D_RW(rootp->arr)->val[src];
			D_RW(rootp->arr)->val[src] = D_RW(rootp->arr)->val[dest];
			D_RW(rootp->arr)->val[dest] = tmp;
		} TX_ONABORT {
			perror("transaction aborted y\n");
		} TX_END
	}
	endCycles = getCycle();
	totalCycles = endCycles - startCycles;
	
	double totTime = ((double)totalCycles)/2000000000;
	printf("swap TX/s = %f\ninsert total tx total time = %f\n", num_op/totTime, totTime);//RUN_COUNT/totTime, totTime);

	pmemobj_close(pop);

	return 0;
}
