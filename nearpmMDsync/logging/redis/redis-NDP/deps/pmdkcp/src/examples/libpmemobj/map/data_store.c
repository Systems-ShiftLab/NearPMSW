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

/////////////////Page fault handling/////////////////
#include <bits/types/sig_atomic_t.h>
#include <bits/types/sigset_t.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SIGSTKSZ	8192

#define SA_SIGINFO    4		
#define SA_ONSTACK   0x08000000 /* Use signal stack by using `sa_restorer'. */
#define SA_RESTART   0x10000000 /* Restart syscall on signal return.  */
#define SA_NODEFER   0x40000000 /* Don't automatically block the signal when*/


stack_t _sigstk;
int updated_page_count = 0;
int all_updates = 0;
void * checkpoint_start;
void * page[50];
PMEMobjpool *pop;
void * device;

/// @brief Signal handler to trap SEGVs.
static void segvHandle(int signum, siginfo_t * siginfo, void * context) {

	

    void * addr = siginfo->si_addr; // address of access
	uint64_t pageNo = ((uint64_t)addr)/4096;
	unsigned long * pageStart = (unsigned long *)(pageNo*4096);

	
	// Check if this was a SEGV that we are supposed to trap.
    if (siginfo->si_code == SEGV_ACCERR) {
		mprotect(pageStart, 4096, PROT_READ|PROT_WRITE);

		if(all_updates > 20 || updated_page_count == 50){
			for(int i=0;i<updated_page_count;i++){
				memcpy(checkpoint_start + i*4096, page[i],4096);
				pmemobj_persist(pop, checkpoint_start + i*4096,4096);	 
/*				*((uint64_t*)device) = (uint64_t)(checkpoint_start + i*4096);
				*((uint64_t*)(device)+1) = 00;
				*((uint64_t*)(device)+2) = (uint64_t)page[i];
				*((uint64_t*)(device)+3) = ((uint64_t)(((0) << 16)| 6) << 32) | 4096;
				*(((uint32_t*)(device))+255) =   (uint32_t)(((0) << 16)| 6);
//*/
				page[updated_page_count] = 0;
			}
			updated_page_count = 0;
			all_updates = 0;
		}
		all_updates ++;
		//printf("te\n");
		for(int i=0; i<updated_page_count; i++){
			if(page[i] == pageStart)
			return;
		}
		page[updated_page_count] = pageStart;
		//printf("test1 %lx %d %d\n",page[updated_page_count],updated_page_count,all_updates);
		
		
		updated_page_count++;
		//*((int *)checkpoint_start) = 10;
		//test++;
		//printf("test1 %lx %d\n",updated_page_count);
    } else if (siginfo->si_code == SEGV_MAPERR) {
		fprintf (stderr, "%d : map error with addr %p!\n", getpid(), addr);
		abort();
    } else {
		fprintf (stderr, "%d : other access error with addr %p.\n", getpid(), addr);
		abort();
    }
}

static void installSignalHandler(void) {
    // Set up an alternate signal stack.
	printf("page fault handler initialized!!\n");
    _sigstk.ss_sp = mmap(NULL, SIGSTKSZ, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON, -1, 0);
    _sigstk.ss_size = SIGSTKSZ;
    _sigstk.ss_flags = 0;
    sigaltstack(&_sigstk, (stack_t *) 0);

    // Now set up a signal handler for SIGSEGV events.
    struct sigaction siga;
    sigemptyset(&siga.sa_mask);

    // Set the following signals to a set
    sigaddset(&siga.sa_mask, SIGSEGV);
    sigaddset(&siga.sa_mask, SIGALRM);

    sigprocmask(SIG_BLOCK, &siga.sa_mask, NULL);

    // Point to the handler function.
    siga.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART | SA_NODEFER;


    siga.sa_sigaction = segvHandle;
    if (sigaction(SIGSEGV, &siga, NULL) == -1) {
      perror("sigaction(SIGSEGV)");
      exit(-1);
    }

    sigprocmask(SIG_UNBLOCK, &siga.sa_mask, NULL);
	return;
}

static void setpage(void * addr){
	uint64_t pageNo = ((uint64_t)addr)/4096;
	unsigned long * pageStart = (unsigned long *)(pageNo*4096);
	mprotect(pageStart, 4096, PROT_READ);
	return;
}

static void resetpage(void * addr){
	uint64_t pageNo = ((uint64_t)addr)/4096;
	unsigned long * pageStart = (unsigned long *)(pageNo*4096);
	mprotect(pageStart, 4096, PROT_READ|PROT_WRITE);
	return;
}

void* open_device(const char* pathname)
{
	//int fd = os_open("/sys/devices/pci0000:00/0000:00:00.2/iommu/ivhd0/devices/0000:0a:00.0/resource0",O_RDWR|O_SYNC);
	int fd = open(pathname,O_RDWR|O_SYNC);
	if(fd == -1)
	{
		printf("Couldnt opene file!!\n");
		exit(0);
	}
	void * ptr = mmap(0,4096,PROT_READ|PROT_WRITE, MAP_SHARED,fd,0);
	if(ptr == (void *)-1)
	{
		printf("Could not map memory!!\n");
		exit(0);
	}
	printf("opened device without error!!\n");
	return ptr;
}
///////////////////////////////////////////////////////////////

#define MAX_INSERTS 100000
int use_ndp_redo = 0;
static uint64_t nkeys;
static uint64_t keys[MAX_INSERTS];

//int page_skip_counter = 0;
TOID_DECLARE(struct page_checkpoint, 0);
struct page_checkpoint{
	char page[50][4096];
};

struct store_item {
	uint64_t item_data;
};

struct store_root {
	TOID(struct map) map;
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

/*
 * new_store_item -- transactionally creates and initializes new item
 */
static TOID(struct store_item)
new_store_item(void)
{
	TOID(struct store_item) item = TX_NEW(struct store_item);
	D_RW(item)->item_data = rand();

	return item;
}

/*
 * get_keys -- inserts the keys of the items by key order (sorted, descending)
 */
static int
get_keys(uint64_t key, PMEMoid value, void *arg)
{
	keys[nkeys++] = key;

	return 0;
}

/*
 * dec_keys -- decrements the keys count for every item
 */
static int
dec_keys(uint64_t key, PMEMoid value, void *arg)
{
	nkeys--;
	return 0;
}

/*
 * parse_map_type -- parse type of map
 */
static const struct map_ops *
parse_map_type(const char *type)
{
	if (strcmp(type, "ctree") == 0)
		return MAP_CTREE;
	else if (strcmp(type, "btree") == 0)
		return MAP_BTREE;
	else if (strcmp(type, "rbtree") == 0)
		return MAP_RBTREE;
	else if (strcmp(type, "hashmap_atomic") == 0)
		return MAP_HASHMAP_ATOMIC;
	else if (strcmp(type, "hashmap_tx") == 0)
		return MAP_HASHMAP_TX;
	else if (strcmp(type, "hashmap_rp") == 0)
		return MAP_HASHMAP_RP;
	else if (strcmp(type, "skiplist") == 0)
		return MAP_SKIPLIST;
	return NULL;

}

void installSignalHandler (void) __attribute__ ((constructor));

int main(int argc, const char *argv[]) {
	if (argc < 3) {
		printf("usage: %s "
			"<ctree|btree|rbtree|hashmap_atomic|hashmap_rp|"
			"hashmap_tx|skiplist> file-name [nops]\n", argv[0]);
		return 1;
	}

	const char *type = argv[1];
	const char *path = argv[2];
	const struct map_ops *map_ops = parse_map_type(type);
	if (!map_ops) {
		fprintf(stderr, "invalid container type -- '%s'\n", type);
		return 1;
	}

	int nops = MAX_INSERTS;

	if (argc > 3) {
		nops = atoi(argv[3]);
		if (nops <= 0 || nops > MAX_INSERTS) {
			fprintf(stderr, "number of operations must be "
				"in range 1..%d\n", MAX_INSERTS);
			return 1;
		}
	}

	
	//PMEMobjpool *pop;
	srand((unsigned)time(NULL));

	if (file_exists(path) != 0) {
		if ((pop = pmemobj_create(path, POBJ_LAYOUT_NAME(data_store),
			(1024*1024*512), 0666)) == NULL) {
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
	
	device = open_device("/sys/devices/pci0000:00/0000:00:00.2/iommu/ivhd0/devices/0000:0a:00.0/resource0");

	uint64_t * tmp;
	tmp = (uint64_t *)device;
	*tmp = 0xdeadbeefdeadbeef;
	pmem_persist(tmp,64);
	
	*tmp = (uint64_t)tmp;
	pmem_persist(tmp,64);
	uint32_t   tid;
	tid = 0;
	tid = tid & 0x3f;
	tid = (tid<< 4)| 0;
	//printf("%d %d\n",tid, pop->run_id);
	*tmp = tid;
	pmem_persist(tmp,64);
	//TOID(struct store_root) root = (TOID(struct store_root))pmemobj_root(pop, 1024*1024*512);
			
	//struct queue *qu = pmemobj_direct(root);
	//checkpoint_start =  (void *)(qu + );

	TOID(struct store_root) root = POBJ_ROOT(pop, struct store_root);
	//checkpoint_start = D_RW(root) + (1024*1024*256);

	TX_BEGIN(pop) {
		checkpoint_start = D_RW(TX_NEW(struct page_checkpoint))->page;
	} TX_END

	struct map_ctx *mapc = map_ctx_init(map_ops, pop);
	if (!mapc) {
		perror("cannot allocate map context\n");
		return 1;
	}
	/* delete the map if it exists */
	if (!map_check(mapc, D_RW(root)->map))
		map_destroy(mapc, &D_RW(root)->map);

	/* insert random items in a transaction */
	int aborted = 0;

	uint64_t endCycles, startCycles,totalCycles;

	
	TX_BEGIN(pop) {
		map_create(mapc, &D_RW(root)->map, NULL);
	} TX_END

	startCycles = getCycle();
	for (int i = 0; i < nops; ++i) {
	TX_BEGIN(pop) {			
			/* new_store_item is transactional! */
			int keyused = rand();
			map_insert(mapc, D_RW(root)->map, keyused,
					new_store_item().oid);

	} TX_ONABORT {
		perror("transaction aborted y\n");
		map_ctx_free(mapc);
		aborted = 1;
	} TX_END
	//updated_page_count = 0;
	}
	endCycles = getCycle();
	totalCycles = endCycles - startCycles;
	
	double totTime = ((double)totalCycles)/2000000000;
	printf("insert btree TX/s = %f\ninsert total tx total time = %f\n", nops/totTime, totTime);//RUN_COUNT/totTime, totTime);

	if (aborted)
		return -1;

	/* count the items */
	map_foreach(mapc, D_RW(root)->map, get_keys, NULL);

	/* remove the items without outer transaction */
	totalCycles = 0;
	for (uint64_t i = 0; i < nkeys; ++i) {
		startCycles = getCycle();
		PMEMoid item = map_remove(mapc, D_RW(root)->map, keys[i]);
		endCycles = getCycle();
		totalCycles += endCycles - startCycles;

		assert(!OID_IS_NULL(item));
		assert(OID_INSTANCEOF(item, struct store_item));
	}
	totTime = ((double)totalCycles)/2000000000;
	printf("remove btree TX/s = %f\nremove total tx total time = %f\n", nops/totTime, totTime);
	uint64_t old_nkeys = nkeys;

	/* tree should be empty */
	map_foreach(mapc, D_RW(root)->map, dec_keys, NULL);
	assert(old_nkeys == nkeys);

	map_ctx_free(mapc);
	pmemobj_close(pop);

	return 0;
}
