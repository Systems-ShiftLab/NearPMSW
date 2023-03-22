/*
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * queue.c -- array based queue example
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libpmem.h>
#include <libpmemobj.h>
#include <time.h>
#include <x86intrin.h> 

#include <bits/types/sig_atomic_t.h>
#include <bits/types/sigset_t.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>

POBJ_LAYOUT_BEGIN(queue);
POBJ_LAYOUT_ROOT(queue, struct root);
POBJ_LAYOUT_TOID(queue, struct entry);
POBJ_LAYOUT_TOID(queue, struct queue);
POBJ_LAYOUT_END(queue);
/////////////////Page fault handling/////////////////
#define SIGSTKSZ	8192

#define SA_SIGINFO    4		
#define SA_ONSTACK   0x08000000 /* Use signal stack by using `sa_restorer'. */
#define SA_RESTART   0x10000000 /* Restart syscall on signal return.  */
#define SA_NODEFER   0x40000000 /* Don't automatically block the signal when*/



/*typedef struct
  {
    void *ss_sp;
    int ss_flags;
    size_t ss_size;
  } stack_t;
*/

stack_t _sigstk;
 
/// @brief Signal handler to trap SEGVs.
static void segvHandle(int signum, siginfo_t * siginfo, void * context) {
    void * addr = siginfo->si_addr; // address of access
	uint64_t pageNo = ((uint64_t)addr)/4096;
	unsigned long * pageStart = (unsigned long *)(pageNo*4096);
	// Check if this was a SEGV that we are supposed to trap.
    if (siginfo->si_code == SEGV_ACCERR) {
      mprotect(pageStart, 4096, PROT_READ|PROT_WRITE);
	  //printf("test1\n");
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
	return 0;
}

static void resetpage(void * addr){
	uint64_t pageNo = ((uint64_t)addr)/4096;
	unsigned long * pageStart = (unsigned long *)(pageNo*4096);
	mprotect(pageStart, 4096, PROT_READ);
	return;
}

/////////////////////////////////////////////////////
uint64_t page_pointer_store[64];
int size_page_store = 0;
int update_count = 0;
int global_update_count = 0;
void * checkpointptr;

static inline void insert_page(void * ptr){
	update_count++;
	uint64_t page = (uint64_t)ptr & 0xFFFFFFFFFFFFC000; 
	for(int i=0; i<size_page_store; i++){
		if(page_pointer_store[i] == page)
			return;
	}
	page_pointer_store[size_page_store] = page;
	size_page_store++;
}

static inline void checkpoint(PMEMobjpool *pop){
	for(int i=0; i<size_page_store; i++){
		memcpy(checkpointptr + i*4096,
			 (void *)(page_pointer_store[i]), 4096);

		pmemobj_persist(pop, checkpointptr + i*4096,4096);	 
	//	pmemobj_drain(pop);
	}
	//pmemobj_drain(pop);

	size_page_store = 0;

}

////////////////////////////////////////////////////
static inline void cpuid(){

    asm volatile ("CPUID"

            :

            :

            : "%rax", "%rbx", "%rcx", "%rdx");

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

static inline uint64_t getCycle2(){
	cpuid();
	return getCycle();
}

struct entry { /* queue entry that contains arbitrary data */
	size_t len; /* length of the data buffer */
	char data[];
};

struct queue { /* array-based queue container */
	size_t front; /* position of the first entry */
	size_t back; /* position of the last entry */

	size_t capacity; /* size of the entries array */
	//TOID(struct entry) entries[];
	struct entry entries[];
};

struct root {
	//TOID(struct queue) queue;
	struct queue queue;
};

/*
 * queue_constructor -- constructor of the queue container
 */
static int
queue_constructor(PMEMobjpool *pop, void *ptr, void *arg)
{
	struct queue *q = ptr;
	size_t *capacity = arg;
	q->front = 0;
	q->back = 0;
	q->capacity = *capacity;

	/* atomic API requires that objects are persisted in the constructor */
	pmemobj_persist(pop, q, sizeof(*q));

	return 0;
}

/*
 * queue_new -- allocates a new queue container using the atomic API
 */
static int
queue_new(PMEMobjpool *pop, TOID(struct queue) *q, size_t nentries)
{
	return POBJ_ALLOC(pop,
		q,
		struct queue,
		sizeof(struct queue) + sizeof(TOID(struct entry)) * nentries,
		queue_constructor,
		&nentries);
}

/*
 * queue_nentries -- returns the number of entries
 */
static size_t
queue_nentries(struct queue *queue)
{
	return queue->back - queue->front;
}

/*
 * queue_enqueue -- allocates and inserts a new entry into the queue
 */
static int
queue_enqueue(PMEMobjpool *pop, struct queue *queue,
	const char *data, size_t len, double * tottime)
{
	//printf("inserting %ld %ld %ld %ld %ld\n", queue->capacity, queue_nentries(queue), queue->back, queue->front, sizeof(struct entry));
	if (queue->capacity - queue_nentries(queue) == 0)
		return -1; /* at capacity */

	/* back is never decreased, need to calculate the real position */
	size_t pos = queue->back % queue->capacity;

	int ret = 0;

	//printf("inserting %zu: %s\n", pos, data);
	clock_t start, end;
	start = clock();
	uint64_t startCycles = getCycle();
	
	insert_page(&(queue->back));
	insert_page(&(queue->entries[pos]));
//	if(update_count >= 1999)
//	{
//		checkpoint(pop);
//		update_count = 0;
//	}

	//TX_BEGIN(pop) {
		/* let's first reserve the space at the end of the queue */
		//TX_ADD_DIRECT(&queue->back);
		//TX_ADD_DIRECT(&queue->entries[pos]);
		queue->back += 1;

		/* now we can safely allocate and initialize the new entry */
		//TOID(struct entry) entry = TX_ALLOC(struct entry,
		//	sizeof(struct entry) + len);
		//D_RW(entry)->len = len;
		//PMEMoid root = pmemobj_root(pop, sizeof(struct entry));
		
		//uint64_t* tmp = pmemobj_direct(root);
		struct entry *entry = (struct entry *)(queue + sizeof(struct queue) + sizeof(struct entry) * 10000 + sizeof(struct entry)*global_update_count); //pmemobj_direct(root);
		//printf(" %d %ld\n",size_page_store, sizeof(struct entry));
		//memcpy(D_RW(entry)->data, data, len);
		memcpy(entry->data, data, len);


		/* and then snapshot the queue entry that we will modify */
		
		queue->entries[pos] = *entry;

		
		pmemobj_persist(pop,&(queue->entries[pos]),sizeof(struct entry));
		pmemobj_persist(pop,queue,sizeof(struct queue));

		if(update_count >= 1)
	{
		resetpage(&(queue->entries[pos]));
		resetpage(queue);
		update_count = 0;
	}
		
		//pmemobj_drain(pop);
		//pmemobj_persist(pop,entry,sizeof(struct entry));
		//pmemobj_drain(pop);
		//pmem_persist(queue->entries[pos],sizeof(struct entry));
	//} TX_ONABORT { /* don't forget about error handling! ;) */
	//	ret = -1;
	//} TX_END
	uint64_t endCycles = getCycle();
	//printf("tx cycles: %ld\n",endCycles - startCycles);
	end = clock();
    *tottime += ((double) (end - start)) / CLOCKS_PER_SEC;
	global_update_count++;
	return ret;
}

/*
 * queue_dequeue - removes and frees the first element from the queue
 */
static int
queue_dequeue(PMEMobjpool *pop, struct queue *queue,  double * tottime)
{
	if (queue_nentries(queue) == 0)
		return -1; /* no entries to remove */

	/* front is also never decreased */
	size_t pos = queue->front % queue->capacity;

	int ret = 0;

	printf("removing %zu: %s\n", pos, queue->entries[pos].data);
	clock_t start, end;
	start = clock();
	uint64_t startCycles = getCycle();
	TX_BEGIN(pop) {
		/* move the queue forward */
		TX_ADD_DIRECT(&queue->front);
		queue->front += 1;
		/* and since this entry is now unreachable, free it */
		//TX_FREE(queue->entries[pos]);
		/* notice that we do not change the PMEMoid itself */
	} TX_ONABORT {
		ret = -1;
	} TX_END
	uint64_t endCycles = getCycle();
	printf("tx cycles: %ld\n",endCycles - startCycles);
	end = clock();
    *tottime += ((double) (end - start)) / CLOCKS_PER_SEC;

	return ret;
}

/*
 * queue_show -- prints all queue entries
 */
static void
queue_show(PMEMobjpool *pop, struct queue *queue)
{
	size_t nentries = queue_nentries(queue);
	printf("Entries %zu/%zu\n", nentries, queue->capacity);
	for (size_t i = queue->front; i < queue->back; ++i) {
		size_t pos = i % queue->capacity;
		printf("%zu: %s\n", pos, queue->entries[pos].data);
	}
}

/* available queue operations */
enum queue_op {
	UNKNOWN_QUEUE_OP,
	QUEUE_NEW,
	QUEUE_ENQUEUE,
	QUEUE_DEQUEUE,
	QUEUE_SHOW,

	MAX_QUEUE_OP,
};

/* queue operations strings */
static const char *ops_str[MAX_QUEUE_OP] =
	{"", "new", "enqueue", "dequeue", "show"};

/*
 * parse_queue_op -- parses the operation string and returns matching queue_op
 */
static enum queue_op
queue_op_parse(const char *str)
{
	for (int i = 0; i < MAX_QUEUE_OP; ++i)
		if (strcmp(str, ops_str[i]) == 0)
			return (enum queue_op)i;

	return UNKNOWN_QUEUE_OP;
}

/*
 * fail -- helper function to exit the application in the event of an error
 */
static void __attribute__((noreturn)) /* this function terminates */
fail(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

void pf(void){
	printf("test1\n");

}

void installSignalHandler (void) __attribute__ ((constructor));
//void pf (void) __attribute__ ((constructor));

main(int argc, char *argv[])
{
	enum queue_op op;
	if (argc < 3 || (op = queue_op_parse(argv[2])) == UNKNOWN_QUEUE_OP)
		fail("usage: file-name [new <n>|show|enqueue <data>|dequeue]");

	PMEMobjpool *pop = pmemobj_open(argv[1], POBJ_LAYOUT_NAME(queue));
	if (pop == NULL)
		fail("failed to open the pool");

/*	TOID(struct root) root = POBJ_ROOT(pop, struct root);
	struct root *rootp = D_RW(root);
	
	size_t capacity;
	double totaltime = 0;
	switch (op) {
		case QUEUE_NEW:
			if (argc != 4)
				fail("missing size of the queue");


			char *end;
			errno = 0;
			capacity = strtoull(argv[3], &end, 0);
			if (errno == ERANGE || *end != '\0')
				fail("invalid size of the queue");
			printf("size %ld\n",capacity);
			if (queue_new(pop, &rootp->queue, capacity) != 0)
				fail("failed to create a new queue");
		break;
		case QUEUE_ENQUEUE:
			if (argc != 4)
				fail("missing new entry data");

			if (D_RW(rootp->queue) == NULL)
				fail("queue must exist");
*/
			double totaltime = 0;
			PMEMoid root = pmemobj_root(pop, sizeof(struct queue) + sizeof(struct entry) * 10000 + sizeof(struct entry) * 10000 + 4096*10);
			
			struct queue *qu = pmemobj_direct(root);
			checkpointptr =  (void *)(qu + sizeof(struct queue) + sizeof(struct entry) * 10000 + sizeof(struct entry) * 10000);

			printf("root1 %ld\n",(uint64_t)qu);
			qu->front = 0;
			qu->back = 0;
			qu->capacity = 10000;

	
			pmemobj_persist(pop, qu, sizeof(*qu));

			totaltime = 0;
			for(int i=0;i<10000;i++){
			if (queue_enqueue(pop, qu,
				"hello", 6,&totaltime) != 0)
				fail("failed to insert new entry");
			}
			printf("TX/s = %f %f\n",10000/totaltime, totaltime);
		//	printf("TX avg cycles = %ld \n",totaltime/10000);
/*		break;
		case QUEUE_DEQUEUE:
			if (D_RW(rootp->queue) == NULL)
				fail("queue must exist");
			totaltime = 0;
			for(int i=0;i<10000;i++){
			if (queue_dequeue(pop, D_RW(rootp->queue),&totaltime) != 0)
				fail("failed to remove entry");
			}
			printf("TX/s = %f %f\n",10000/totaltime, totaltime);
		//	printf("TX avg cycles = %ld \n",totaltime/10000);
		break;
		case QUEUE_SHOW:
			if (D_RW(rootp->queue) == NULL)
				fail("queue must exist");

			queue_show(pop, D_RW(rootp->queue));
		break;
		default:
			assert(0); // unreachable 
		break;
	}
*/
	pmemobj_close(pop);

	return 0;
}
