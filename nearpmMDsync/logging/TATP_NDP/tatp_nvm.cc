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

//Korakit
//might need to change parameters
#define NUM_SUBSCRIBERS 100000	//100000
#define NUM_OPS_PER_CS 2 
#define NUM_OPS 30000	//10000000
#define NUM_THREADS 1 

TATP_DB* my_tatp_db;
//#include "../DCT/rdtsc.h"

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

#include <sys/mman.h>
#include <fcntl.h>
void * device;
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

int main(int argc, char* argv[]) {

  //printf("in main\n");

  //struct timeval tv_start;
  //struct timeval tv_end;
  //std::ofstream fexec;
  //fexec.open("exec.csv",std::ios_base::app);
  // Korakit: move to the init
  // LIU
  
  device = open_device("/sys/devices/pci0000:00/0000:00:00.2/iommu/ivhd0/devices/0000:0a:00.0/resource0");


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
	printf("tottime %f\n", totTime);

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
