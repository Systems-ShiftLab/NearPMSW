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
#include <libpmem.h>
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

#define GRANULARITY 4096
/// @brief Signal handler to trap SEGVs.
//static void segvHandle(int signum, siginfo_t * siginfo, void * context) {


void cmd_issue( uint32_t opcode,
		uint32_t TXID,
		uint32_t TID,
		uint32_t OID,
		uint64_t data_addr,
		uint32_t data_size,
		void * ptr){

	

	//command with thread id encoded as first 8 bits of each word
	uint32_t issue_cmd[7];
	issue_cmd[0] = (TID<<24)|(opcode<<16)|(TXID<<8)|TID;
	issue_cmd[1] = (TID<<24)|(OID<<16)|(data_addr>>48);
	issue_cmd[2] = (TID<<24)|((data_addr & 0x0000FFFFFFFFFFFF)>>24);
	issue_cmd[3] = (TID<<24)|(data_addr & 0x0000000000FFFFFF);
	issue_cmd[4] = (TID<<24)|(data_size<<8);
	issue_cmd[5] = (TID<<24)|(0X00FFFFFF>>16);
	issue_cmd[6] = (TID<<24)|((0X00FFFFFF & 0x0000FFFF)<<8);

	for(int i=0;i<7;i++){
	//	printf("%08x\n",issue_cmd[i]);
		*((u_int32_t *) ptr) = issue_cmd[i];
	}


}
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

/// @brief Signal handler to trap SEGVs.
static void segvHandle(int signum, siginfo_t * siginfo, void * context) {
//#define CPTIME
#ifdef CPTIME
    uint64_t endCycles, startCycles,totalCycles;
	
    startCycles = getCycle();
#endif
	

    void * addr = siginfo->si_addr; // address of access
	uint64_t pageNo = ((uint64_t)addr)/4096;
	unsigned long * pageStart = (unsigned long *)(pageNo*4096);

	
	// Check if this was a SEGV that we are supposed to trap.
    if (siginfo->si_code == SEGV_ACCERR) {
		mprotect(pageStart, 4096, PROT_READ|PROT_WRITE);
		if(all_updates >=5 || updated_page_count == 50){
			for(int i=0;i<updated_page_count;i++){
				//memcpy(checkpoint_start + 4096, pageStart, 4096);
				//pmem_persist(checkpoint_start + 4096, 4096);
				cmd_issue(2,0,0,0, (uint64_t)pageStart,4096,device);
/*				*((uint64_t*)device) = (uint64_t)(checkpoint_start + 4096);
				*((uint64_t*)(device)+1) = 00;
				*((uint64_t*)(device)+2) = (uint64_t)0.320580;
				*((uint64_t*)(device)+3) = ((uint64_t)(((0) << 16)| 6) << 32) | 4096;
				*(((uint32_t*)(device))+255) =   (uint32_t)(((0) << 16)| 6);
*/
				page[updated_page_count] = 0;
			}
			updated_page_count = 0;
			all_updates = 0;
		}
		all_updates ++;
		
		for(int i=0; i<updated_page_count; i++){
			if(page[i] == pageStart){
#ifdef CPTIME
            endCycles = getCycle();
            totalCycles = endCycles - startCycles;
       
            double totTime = ((double)totalCycles)/2000000000;
            printf("timecp = %f\n", totTime);
#endif 
			return;}
		}
		page[updated_page_count] = pageStart;
		//printf("test1 %lx %d %d\n",page[updated_page_count],updated_page_count,all_updates);

		updated_page_count++;
#ifdef CPTIME
    endCycles = getCycle();
    totalCycles = endCycles - startCycles;
       
    double totTime = ((double)totalCycles)/2000000000;
    printf("timecp = %f\n", totTime);
#endif
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

#define size 10000
unsigned num_op = 20000;
unsigned g_seed = 1312515;



struct store_item {
	uint64_t val[size];
};

struct store_root{
	TOID(struct store_item)  arr;
};




void installSignalHandler (void) __attribute__ ((constructor));



int main(int argc, const char *argv[]) {
	size_t mapped_len1;
    int is_pmem1;
    if ((checkpoint_start = pmem_map_file("/mnt/mem/checkpoint", GRANULARITY*50,
        PMEM_FILE_CREATE, 0666, &mapped_len1, &is_pmem1)) == NULL) {
        fprintf(stderr, "pmem_map_file failed\n");
        exit(0);
    }

	device = open_device("/sys/devices/pci0000:00/0000:00:00.2/iommu/ivhd0/devices/0000:0a:00.0/resource0");

	const char *path = argv[1];

	PMEMobjpool *pop;

	if (file_exists(path) != 0) {
		if ((pop = pmemobj_create(path, POBJ_LAYOUT_NAME(data_store),
			(sizeof(uint64_t)*(size)), 0666)) == NULL) {
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

	if (!rootp){
		printf("NULL root!!\n");
		exit(0);
	}
		
	TX_BEGIN(pop) {	
//		TX_ADD(root);
		TOID(struct store_item) arr = TX_NEW(struct store_item);
//		for(int i=0; i<size; i++)
//			D_RW(arr)->val[i] = i;
		rootp->arr = arr;
	} TX_END

	uint64_t endCycles, startCycles,totalCycles, tmp;
	unsigned src, dest;
		
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