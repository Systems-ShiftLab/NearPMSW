/*
 Author: Vaibhav Gogte <vgogte@umich.edu>
         Aasheesh Kolli <akolli@umich.edu>


This file is the TATP benchmark, performs various transactions as per the specifications.
*/

#include "tatp_db.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <cstdint>
#include <assert.h>
#include <sys/time.h>
#include <string>
#include <fstream>
#include <libpmem.h>
#include <cstring>
//Korakit
//might need to change parameters
#define NUM_SUBSCRIBERS 100000	//100000
#define NUM_OPS_PER_CS 2 
#define NUM_OPS 30000	//10000000
#define NUM_THREADS 1 

TATP_DB* my_tatp_db;
//#include "../DCT/rdtsc.h"

void init_db() {
  unsigned num_subscribers = NUM_SUBSCRIBERS;
  my_tatp_db = (TATP_DB *)malloc(sizeof(TATP_DB));
  my_tatp_db->initialize(num_subscribers,NUM_THREADS);
  fprintf(stderr, "Created tatp db at %p\n", (void *)my_tatp_db);
}


void* update_locations(void* args) {
  int thread_id = *((int*)args);
  for(int i=0; i<NUM_OPS/NUM_THREADS; i++) {
    my_tatp_db->update_location(thread_id,NUM_OPS_PER_CS);
  }
  return 0;
}

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
void * device;
double totTimeCP = 0;

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
#define GRANULARITY 2048
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

static void segvHandle(int signum, siginfo_t * siginfo, void * context) {
#define CPTIME
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
				memcpy(checkpoint_start + 4096, pageStart, 4096);
				pmem_persist(checkpoint_start + 4096, 4096);
        		//cmd_issue(2,0,0,0, (uint64_t)pageStart,4096,device);
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
            printf("cp %f\n", totTime);
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
            printf("cp %f\n", totTime);
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

///////////////////////////////////////////////////////////////////
void installSignalHandler (void) __attribute__ ((constructor));

int main(int argc, char* argv[]) {

  //printf("in main\n");

  //struct timeval tv_start;
  //struct timeval tv_end;
  //std::ofstream fexec;
  //fexec.open("exec.csv",std::ios_base::app);
  // Korakit: move to the init
  // LIU
  
  device = open_device("/sys/devices/pci0000:00/0000:00:00.2/iommu/ivhd0/devices/0000:0a:00.0/resource0");

  uint64_t* tmp =  (uint64_t*)device;     

	*tmp = 0xdeadbeefdeadbeef;
	pmem_persist(tmp,64);
	
	*tmp = (uint64_t)tmp;
	pmem_persist(tmp,64);
	uint32_t   tid;
	tid = 0;//gettid();
	tid = tid & 0x3f;
	tid = (tid<< 4)| 0;//pop->run_id;
	//printf("%d %d\n",tid, pop->run_id);
	*tmp = tid;
	pmem_persist(tmp,64);       


  init_db();

  // LIU: remove output
  //std::cout<<"done with initialization"<<std::endl;

  my_tatp_db->populate_tables(NUM_SUBSCRIBERS);
  // LIU: remove output
  //std::cout<<"done with populating tables"<<std::endl;

  pthread_t threads[NUM_THREADS];
  int id[NUM_THREADS];

  //Korakit
  //exit to count instructions after initialization
  //we use memory trace from the beginning to this to test the compression ratio
  //as update locations(the actual test) only do one update

  // LIU
  // gettimeofday(&tv_start, NULL);

  //CounterAtomic::initCounterCache();
  uint64_t endCycles, startCycles,totalCycles;

  startCycles = getCycle();
  

  for(int i=0; i<NUM_THREADS; i++) {
    id[i] = i;
    update_locations((void*)&id[i]);
  }

  endCycles = getCycle();
	totalCycles = endCycles - startCycles;
	
	double totTime = ((double)totalCycles)/2000000000;
	printf("tottime %f\ndata movement time %f\n", totTime, totTimeCP);

//Korakit
//Not necessary for single threaded version
/*
  for(int i=0; i<NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }
*/

  // LIU: remove the output  
/*
  gettimeofday(&tv_end, NULL);
  fprintf(stderr, "time elapsed %ld us\n",
          tv_end.tv_usec - tv_start.tv_usec +
              (tv_end.tv_sec - tv_start.tv_sec) * 1000000);



  fexec << "TATP" << ", " << std::to_string((tv_end.tv_usec - tv_start.tv_usec) + (tv_end.tv_sec - tv_start.tv_sec) * 1000000) << std::endl;


  fexec.close();
  free(my_tatp_db);

  std::cout<<"done with threads"<<std::endl;
*/

  return 0;
}
