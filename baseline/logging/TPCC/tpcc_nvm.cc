/*
 Author: Vaibhav Gogte <vgogte@umich.edu>
         Aasheesh Kolli <akolli@umich.edu>


This file models the TPCC benchmark.
*/

//Korakit
//remove MT stuffs
//#include <pthread.h>
#include <memkind.h>
#include <dlfcn.h>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <string>
#include <fstream>
//#include "txopt.h"


#include <libpmem.h>
#include "tpcc_db.h"
#include "../include/txopt.h"

#define NUM_ORDERS 1000	//10000000
#define NUM_THREADS 1 

#define NUM_WAREHOUSES 1
#define NUM_ITEMS 10000//10000
#define NUM_LOCKS NUM_WAREHOUSES*10 + NUM_WAREHOUSES*NUM_ITEMS
TPCC_DB* tpcc_db[NUM_THREADS];

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


void initialize(int tid, void * backUpLog) {
  tpcc_db[tid] = (TPCC_DB *)malloc(sizeof(TPCC_DB));
  tpcc_db[tid]->backUpInst = (struct backUpLog *)backUpLog;
  new(tpcc_db[tid]) TPCC_DB();
  tpcc_db[tid]->initialize(NUM_WAREHOUSES, NUM_THREADS);
  // fprintf(stderr, "Created tpcc at %p\n", (void *)tpcc_db[tid]);
}


//void new_orders(TxEngine* tx_engine, int tx_engn_type, TPCC_DB* tpcc_db, int thread_id, int num_orders, int num_threads, int num_strands_per_thread, std::atomic<bool>*wait) {
void* new_orders(void* arguments) {

  int thread_id = *((int*)arguments);
  // fprintf(stdout, "New order, thread: %d\n", thread_id);

  for(int i=0; i<NUM_ORDERS/NUM_THREADS; i++) {
    int w_id = 1;
    //There can only be 10 districts, this controls the number of locks in tpcc_db, which is why NUM_LOCKS = warehouse*10
    int d_id = tpcc_db[thread_id]->get_random(thread_id, 1, 10);
    int c_id = tpcc_db[thread_id]->get_random(thread_id, 1, 3000);
    // fprintf(stdout, "thread: %d, line: %d\n", thread_id, __LINE__);

    tpcc_db[thread_id]->new_order_tx(thread_id, w_id, d_id, c_id);
    // fprintf(stdout, "thread: %d, #%d\n", thread_id, i);
  }
  // fprintf(stdout, "thread: %d\n", thread_id);
  // return 0;
}
#define PMEM_MAX_SIZE (1024 * 1024 * 32)
#define GRANULARITY 4096
int main(int argc, char* argv[]) {
 
  //Korakit
  //Remove all timing/stats stuffs
  /*
  std::cout<<"in main"<<std::endl;
  struct timeval tv_start;
  struct timeval tv_end;
  std::ofstream fexec;
  fexec.open("exec.csv",std::ios_base::app);
	*/

  size_t mapped_len;
  int is_pmem;
  void * backUpLogPtr;
  void * backUpLogPtr1;

  if ((backUpLogPtr = pmem_map_file("/mnt/mem/tpcc_db", sizeof(struct backUpLog)*NUM_THREADS,
        PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem)) == NULL) {
        fprintf(stderr, "pmem_map_file failed\n");
        exit(0);
  }
  if ((backUpLogPtr1 = pmem_map_file("/mnt/mem/tpcc_db1", sizeof(struct backUpLog)*NUM_THREADS,
        PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem)) == NULL) {
        fprintf(stderr, "pmem_map_file failed\n");
        exit(0);
  }
	


  for(int i=0;i<NUM_THREADS;i++){
    initialize(i, (backUpLogPtr + i*sizeof(struct backUpLog)));
  }
  // exit(0);
//CounterAtomic::initCounterCache();

  /*
  std::cout<<"num_threads, num_orders = "<< NUM_THREADS <<", "<<NUM_ORDERS <<std::endl;

  std::cout<<"done with initialization"<<std::endl;

  tpcc_db->populate_tables();

  std::cout<<"done with populating tables"<<std::endl;
  */
  pthread_t threads[NUM_THREADS];
  int id[NUM_THREADS];

  //gettimeofday(&tv_start, NULL);
  uint64_t endCycles, startCycles,totalCycles;

  startCycles =  getCycle();
  for(int i=0; i<NUM_THREADS; i++) {
    id[i] = i;
    // fprintf(stderr, "create %d\n", i);
    //Korakit
    //convert to ST version
    //new_orders((void *)(id+i));
    new_orders((void *)&id[i]);
  }
  endCycles = getCycle();
	totalCycles = endCycles - startCycles;
	
	double totTime = ((double)totalCycles)/2000000000;
	printf("tottime %f\n", totTime);
  //Korakit
  //remote MT stuffs
  
  // for(int i=0; i<NUM_THREADS; i++) {
  //   pthread_join(threads[i], NULL);
  // }
  
  //Korakit
  //Remove all timing stuffs
  /*
  gettimeofday(&tv_end, NULL);
  fprintf(stderr, "time elapsed %ld us\n",
          tv_end.tv_usec - tv_start.tv_usec +
              (tv_end.tv_sec - tv_start.tv_sec) * 1000000);

  fexec << "TPCC" << ", " << std::to_string((tv_end.tv_usec - tv_start.tv_usec) + (tv_end.tv_sec - tv_start.tv_sec) * 1000000) << std::endl;

  fexec.close();
  */
  //free(tpcc_db);

  //std::cout<<"done with threads"<<std::endl;

  return 0;

}
