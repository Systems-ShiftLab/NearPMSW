/*
 Author: Vaibhav Gogte <vgogte@umich.edu>
         Aasheesh Kolli <akolli@umich.edu>

This file defines the various transactions in TATP.
*/

#include "tatp_db.h"
#include <cstdlib> // For rand
#include <iostream>
#include <libpmem.h>
//#include <queue>
//#include <iostream>

#define NUM_RNDM_SEEDS 1280

subscriber_entry * subscriber_table_entry_backup;
uint64_t * subscriber_table_entry_backup_valid;

extern void * device;

void * move_data(void * src, void * dest, int size){
  *((uint64_t*)device) = (uint64_t)(dest);
  *((uint64_t*)(device)+1) = 00;
  *((uint64_t*)(device)+2) = (uint64_t)src;
  *((uint64_t*)(device)+3) = ((uint64_t)(((0) << 16)| 6) << 32) | size;
  *(((uint32_t*)(device))+255) =   (uint32_t)(((0) << 16)| 6);
}

inline void cmd_issue( uint32_t opcode,
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


int getRand() {
  return rand();
}

TATP_DB::TATP_DB(unsigned num_subscribers) {}
//Korakit
//this function is used in setup phase, no need to provide crash consistency
void TATP_DB::initialize(unsigned num_subscribers, int n) {
  total_subscribers = num_subscribers;
  num_threads = n;

  size_t mapped_len;
  int is_pmem;
  void * pmemstart;

  int totsize = num_subscribers*sizeof(subscriber_entry) + 4*num_subscribers*sizeof(access_info_entry) + 4*num_subscribers*sizeof(special_facility_entry)  + 3*4*num_subscribers*sizeof(call_forwarding_entry) + NUM_RNDM_SEEDS*sizeof(unsigned long) + NUM_RNDM_SEEDS*sizeof(unsigned long) + NUM_RNDM_SEEDS*sizeof(unsigned long) + sizeof(subscriber_entry) + sizeof(uint64_t);

  if ((pmemstart = pmem_map_file("/mnt/mem/tatp", totsize,
        PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem)) == NULL) {
        fprintf(stderr, "pmem_map_file failed\n");
        exit(0);
   }


  uint64_t* tmp = (uint64_t*)pmemstart;
	//printf( "%ld\n",PMEM_OBJ_POOL_UNUSED2_SIZE);
	//printf( "%ld %ld %ld\n",sizeof(PMEMobjpool),sizeof(uint16_t),sizeof(void*));
	printf("vaddr %p pmemobjid %lx\n",tmp,0);

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
  

  subscriber_table = (subscriber_entry*) pmemstart;

  // A max of 4 access info entries per subscriber
  access_info_table = (access_info_entry*) (pmemstart + num_subscribers*sizeof(subscriber_entry));

  // A max of 4 access info entries per subscriber
  special_facility_table = (special_facility_entry*) (pmemstart + num_subscribers*sizeof(subscriber_entry) + 4*num_subscribers*sizeof(access_info_entry));

  // A max of 3 call forwarding entries per "special facility entry"
  call_forwarding_table= (call_forwarding_entry*) (pmemstart + num_subscribers*sizeof(subscriber_entry) + 4*num_subscribers*sizeof(access_info_entry) +  4*num_subscribers*sizeof(special_facility_entry));
  //Korakit
  //removed for single thread version
  /*
  lock_ = (pthread_mutex_t *)malloc(n*sizeof(pthread_mutex_t));

  for(int i=0; i<num_threads; i++) {
    pthread_mutex_init(&lock_[i], NULL);
  }
  */
  for(int i=0; i<4*num_subscribers; i++) {
    access_info_table[i].valid = false;
    special_facility_table[i].valid = false;
    for(int j=0; j<3; j++) {
      call_forwarding_table[3*i+j].valid = false;
  //    printf("%d\n",j);
    }
  //   printf("%d %d %d\n", i, 4*num_subscribers, totsize);
  }

  //printf("ab\n");
  //rndm_seeds = new std::atomic<unsigned long>[NUM_RNDM_SEEDS];
  //rndm_seeds = (std::atomic<unsigned long>*) malloc(NUM_RNDM_SEEDS*sizeof(std::atomic<unsigned long>));
  subscriber_rndm_seeds = (unsigned long*) (pmemstart + num_subscribers*sizeof(subscriber_entry) + 4*num_subscribers*sizeof(access_info_entry) +  4*num_subscribers*sizeof(special_facility_entry) + 3*4*num_subscribers*sizeof(call_forwarding_entry));
  vlr_rndm_seeds = (unsigned long*) (pmemstart + num_subscribers*sizeof(subscriber_entry) + 4*num_subscribers*sizeof(access_info_entry) +  4*num_subscribers*sizeof(special_facility_entry) + 3*4*num_subscribers*sizeof(call_forwarding_entry) + NUM_RNDM_SEEDS*sizeof(unsigned long));
  rndm_seeds = (unsigned long*) (pmemstart + num_subscribers*sizeof(subscriber_entry) + 4*num_subscribers*sizeof(access_info_entry) +  4*num_subscribers*sizeof(special_facility_entry) + 3*4*num_subscribers*sizeof(call_forwarding_entry) + NUM_RNDM_SEEDS*sizeof(unsigned long) +  NUM_RNDM_SEEDS*sizeof(unsigned long));

  subscriber_table_entry_backup = (subscriber_entry*) (pmemstart + num_subscribers*sizeof(subscriber_entry) + 4*num_subscribers*sizeof(access_info_entry) +  4*num_subscribers*sizeof(special_facility_entry) + 3*4*num_subscribers*sizeof(call_forwarding_entry) + NUM_RNDM_SEEDS*sizeof(unsigned long) +  NUM_RNDM_SEEDS*sizeof(unsigned long) + NUM_RNDM_SEEDS*sizeof(unsigned long));
  subscriber_table_entry_backup_valid = (uint64_t*)(pmemstart + num_subscribers*sizeof(subscriber_entry) + 4*num_subscribers*sizeof(access_info_entry) +  4*num_subscribers*sizeof(special_facility_entry) + 3*4*num_subscribers*sizeof(call_forwarding_entry) + NUM_RNDM_SEEDS*sizeof(unsigned long) +  NUM_RNDM_SEEDS*sizeof(unsigned long) + NUM_RNDM_SEEDS*sizeof(unsigned long) + sizeof(subscriber_entry) );
  //sgetRand();
  for(int i=0; i<NUM_RNDM_SEEDS; i++) {
    subscriber_rndm_seeds[i] = getRand()%(NUM_RNDM_SEEDS*10)+1;
    vlr_rndm_seeds[i] = getRand()%(NUM_RNDM_SEEDS*10)+1;
    rndm_seeds[i] = getRand()%(NUM_RNDM_SEEDS*10)+1;
    //std::cout<<i<<" "<<rndm_seeds[i]<<std::endl;
  }
}

TATP_DB::~TATP_DB(){

  free(subscriber_rndm_seeds);
  free(vlr_rndm_seeds);
  free(rndm_seeds);
}
//Korakit
//this function is used in setup phase, no need to provide crash consistency
void TATP_DB::populate_tables(unsigned num_subscribers) {
  for(int i=0; i<num_subscribers; i++) {
    fill_subscriber_entry(i);
    int num_ai_types = getRand()%4 + 1; // num_ai_types varies from 1->4
    for(int j=1; j<=num_ai_types; j++) {
      fill_access_info_entry(i,j);
    }
    int num_sf_types = getRand()%4 + 1; // num_sf_types varies from 1->4
    for(int k=1; k<=num_sf_types; k++) {
      fill_special_facility_entry(i,k);
      int num_call_forwards = getRand()%4; // num_call_forwards varies from 0->3
      for(int p=0; p<num_call_forwards; p++) {
        fill_call_forwarding_entry(i,k,8*p);
      }
    }
  }
}
//Korakit
//this function is used in setup phase, no need to provide crash consistency
void TATP_DB::fill_subscriber_entry(unsigned _s_id) {
  subscriber_table[_s_id].s_id = _s_id;
  convert_to_string(_s_id, 15, subscriber_table[_s_id].sub_nbr);

  subscriber_table[_s_id].bit_1 = (short) (getRand()%2);
  subscriber_table[_s_id].bit_2 = (short) (getRand()%2);
  subscriber_table[_s_id].bit_3 = (short) (getRand()%2);
  subscriber_table[_s_id].bit_4 = (short) (getRand()%2);
  subscriber_table[_s_id].bit_5 = (short) (getRand()%2);
  subscriber_table[_s_id].bit_6 = (short) (getRand()%2);
  subscriber_table[_s_id].bit_7 = (short) (getRand()%2);
  subscriber_table[_s_id].bit_8 = (short) (getRand()%2);
  subscriber_table[_s_id].bit_9 = (short) (getRand()%2);
  subscriber_table[_s_id].bit_10 = (short) (getRand()%2);

  subscriber_table[_s_id].hex_1 = (short) (getRand()%16);
  subscriber_table[_s_id].hex_2 = (short) (getRand()%16);
  subscriber_table[_s_id].hex_3 = (short) (getRand()%16);
  subscriber_table[_s_id].hex_4 = (short) (getRand()%16);
  subscriber_table[_s_id].hex_5 = (short) (getRand()%16);
  subscriber_table[_s_id].hex_6 = (short) (getRand()%16);
  subscriber_table[_s_id].hex_7 = (short) (getRand()%16);
  subscriber_table[_s_id].hex_8 = (short) (getRand()%16);
  subscriber_table[_s_id].hex_9 = (short) (getRand()%16);
  subscriber_table[_s_id].hex_10 = (short) (getRand()%16);

  subscriber_table[_s_id].byte2_1 = (short) (getRand()%256);
  subscriber_table[_s_id].byte2_2 = (short) (getRand()%256);
  subscriber_table[_s_id].byte2_3 = (short) (getRand()%256);
  subscriber_table[_s_id].byte2_4 = (short) (getRand()%256);
  subscriber_table[_s_id].byte2_5 = (short) (getRand()%256);
  subscriber_table[_s_id].byte2_6 = (short) (getRand()%256);
  subscriber_table[_s_id].byte2_7 = (short) (getRand()%256);
  subscriber_table[_s_id].byte2_8 = (short) (getRand()%256);
  subscriber_table[_s_id].byte2_9 = (short) (getRand()%256);
  subscriber_table[_s_id].byte2_10 = (short) (getRand()%256);

  subscriber_table[_s_id].msc_location = getRand()%(2^32 - 1) + 1;
  subscriber_table[_s_id].vlr_location = getRand()%(2^32 - 1) + 1;
}
//Korakit
//this function is used in setup phase, no need to provide crash consistency
void TATP_DB::fill_access_info_entry(unsigned _s_id, short _ai_type) {

  int tab_indx = 4*_s_id + _ai_type - 1;

  access_info_table[tab_indx].s_id = _s_id;
  access_info_table[tab_indx].ai_type = _ai_type;

  access_info_table[tab_indx].data_1 = getRand()%256;
  access_info_table[tab_indx].data_2 = getRand()%256;

  make_upper_case_string(access_info_table[tab_indx].data_3, 3);
  make_upper_case_string(access_info_table[tab_indx].data_4, 5);

  access_info_table[tab_indx].valid = true;
}
//Korakit
//this function is used in setup phase, no need to provide crash consistency
void TATP_DB::fill_special_facility_entry(unsigned _s_id, short _sf_type) {

  int tab_indx = 4*_s_id + _sf_type - 1;

  special_facility_table[tab_indx].s_id = _s_id;
  special_facility_table[tab_indx].sf_type = _sf_type;

  special_facility_table[tab_indx].is_active = ((getRand()%100 < 85) ? 1 : 0);

  special_facility_table[tab_indx].error_cntrl = getRand()%256;
  special_facility_table[tab_indx].data_a = getRand()%256;

  make_upper_case_string(special_facility_table[tab_indx].data_b, 5);
  special_facility_table[tab_indx].valid = true;
}
//Korakit
//this function is used in setup phase, no need to provide crash consistency
void TATP_DB::fill_call_forwarding_entry(unsigned _s_id, short _sf_type, short _start_time) {
  if(_start_time == 0)
    return;

  int tab_indx = 12*_s_id + 3*(_sf_type-1) + (_start_time-8)/8;

  call_forwarding_table[tab_indx].s_id = _s_id;
  call_forwarding_table[tab_indx].sf_type = _sf_type;
  call_forwarding_table[tab_indx].start_time = _start_time - 8;

  call_forwarding_table[tab_indx].end_time = (_start_time - 8) + getRand()%8 + 1;

  convert_to_string(getRand()%1000, 15, call_forwarding_table[tab_indx].numberx);
}

void TATP_DB::convert_to_string(unsigned number, int num_digits, char* string_ptr) {
  
  char digits[10] = {'0','1','2','3','4','5','6','7','8','9'};
  int quotient = number;
  int reminder = 0;
  int num_digits_converted=0;
  int divider = 1;
  while((quotient != 0) && (num_digits_converted<num_digits)) {
    divider = 10^(num_digits_converted+1);
    reminder = quotient%divider; quotient = quotient/divider;
    string_ptr[num_digits-1 - num_digits_converted] = digits[reminder];
    num_digits_converted++;
  }
  if(num_digits_converted < num_digits) {
    string_ptr[num_digits-1 - num_digits_converted] = digits[0];
    num_digits_converted++;
  }
  return;
}

void TATP_DB::make_upper_case_string(char* string_ptr, int num_chars) {
  char alphabets[26] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N',
                                'O','P','Q','R','S','T','U','V','W','X','Y','Z'};
  for(int i=0; i<num_chars; i++) {
    string_ptr[i] = alphabets[getRand()%26];
  }
  return;
}

void TATP_DB::update_subscriber_data(int threadId) {
  unsigned rndm_s_id = getRand() % total_subscribers;
  short rndm_sf_type = getRand() % 4 + 1;
  unsigned special_facility_tab_indx = 4*rndm_s_id + rndm_sf_type -1;

  if(special_facility_table[special_facility_tab_indx].valid) {

//FIXME: There is a potential data race here, do not use this function yet
  //Korakit
  //removed for single thread version
  //pthread_mutex_lock(&lock_[rndm_s_id]);
    subscriber_table[rndm_s_id].bit_1 = getRand()%2;
    special_facility_table[special_facility_tab_indx].data_a = getRand()%256;
  //Korakit
  //removed for single thread version
  //pthread_mutex_unlock(&lock_[rndm_s_id]);

  }
  return;
}

//subscriber_entry subscriber_table_entry_backup;
//uint64_t subscriber_table_entry_backup_valid;
#define CPTIME
uint64_t totTimeulog = 0;
void TATP_DB::update_location(int threadId, int num_ops) {
  long rndm_s_id;
  rndm_s_id = get_random_s_id(threadId)-1;
  rndm_s_id /=total_subscribers;
  //Korakit
  //removed for single thread version
  //pthread_mutex_lock(&lock_[rndm_s_id]);
      //create backup
      //*subscriber_table_entry_backup = subscriber_table[rndm_s_id];
      //move_data(&subscriber_table_entry_backup, &subscriber_table[rndm_s_id],sizeof(subscriber_table_entry_backup));
#ifdef CPTIME
    uint64_t endCycles, startCycles,totalCycles;
	
    startCycles = getCycle();
#endif
      cmd_issue( 2, 1, 0, 0, (uint64_t)(&subscriber_table[rndm_s_id]), sizeof(subscriber_table_entry_backup), device); 
#ifdef CPTIME
        endCycles = getCycle();
        totalCycles = endCycles - startCycles;
       
        totTimeulog += (totalCycles);
		  printf("ulog %ld\n",totTimeulog);
#endif   
      //s_fence();

      //*subscriber_table_entry_backup_valid = 1;
      //s_fence();
      
      subscriber_table[rndm_s_id].vlr_location = get_random_vlr(threadId);
      //flush_caches(&subscriber_table[rndm_s_id], sizeof(subscriber_table[rndm_s_id]));

      //*subscriber_table_entry_backup_valid = 0;
      //s_fence();


 


  //Korakit
  //removed for single thread version
  //pthread_mutex_unlock(&lock_[rndm_s_id]);

  return;
}

void TATP_DB::insert_call_forwarding(int threadId) {
  return;
}

void TATP_DB::delete_call_forwarding(int threadId) {
  return;
}

void TATP_DB::print_results() {
  //std::cout<<"TxType:0 successful txs = "<<txCounts[0][0]<<std::endl;
  //std::cout<<"TxType:0 failed txs = "<<txCounts[0][1]<<std::endl;
  //std::cout<<"TxType:1 successful txs = "<<txCounts[1][0]<<std::endl;
  //std::cout<<"TxType:1 failed txs = "<<txCounts[1][1]<<std::endl;
  //std::cout<<"TxType:2 successful txs = "<<txCounts[2][0]<<std::endl;
  //std::cout<<"TxType:2 failed txs = "<<txCounts[2][1]<<std::endl;
  //std::cout<<"TxType:3 successful txs = "<<txCounts[3][0]<<std::endl;
  //std::cout<<"TxType:3 failed txs = "<<txCounts[3][1]<<std::endl;
}

unsigned long TATP_DB::get_random(int thread_id) {
  //return (getRand()%65536 | min + getRand()%(max - min + 1)) % (max - min + 1) + min;
  unsigned long tmp;
  tmp = rndm_seeds[thread_id*10] = (rndm_seeds[thread_id*10] * 16807) % 2147483647;
  return tmp;
}

unsigned long TATP_DB::get_random(int thread_id, int min, int max) {
  //return (getRand()%65536 | min + getRand()%(max - min + 1)) % (max - min + 1) + min;
  unsigned long tmp;
  tmp = rndm_seeds[thread_id*10] = (rndm_seeds[thread_id*10] * 16807) % 2147483647;
  return (min+tmp%(max-min+1));
}

unsigned long TATP_DB::get_random_s_id(int thread_id) {
  unsigned long tmp;
  tmp = subscriber_rndm_seeds[thread_id*10] = (subscriber_rndm_seeds[thread_id*10] * 16807) % 2147483647;
  return (1 + tmp%(total_subscribers));
}

unsigned long TATP_DB::get_random_vlr(int thread_id) {
  unsigned long tmp;
  tmp = vlr_rndm_seeds[thread_id*10] = (vlr_rndm_seeds[thread_id*10] * 16807)%2147483647;
  return (1 + tmp%(2^32));
}

