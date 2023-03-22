/*
 Author: Vaibhav Gogte <vgogte@umich.edu>
         Aasheesh Kolli <akolli@umich.edu>


This file defines the various functions of the tpcc database
*/


#include <cstdlib>
#include <iostream>
#include <queue>
#include <cstring> // For memcpy
#include <algorithm> // for sort
#include "tpcc_db.h"

#include <libpmem.h>
//#define NEW_ORDER_LOCK 10;
#define TPCC_DEBUG 0
//#define NUM_ITEMS 1000
#define NUM_ITEMS 10000
#define NUM_RNDM_SEEDS 1280


extern void * device;

void * move_data(void * src, void * dest, int size){
  *((uint64_t*)device) = (uint64_t)(dest);
  *((uint64_t*)(device)+1) = 00;
  *((uint64_t*)(device)+2) = (uint64_t)src;
  *((uint64_t*)(device)+3) = ((uint64_t)(((0) << 16)| 6) << 32) | size;
  *(((uint32_t*)(device))+255) =   (uint32_t)(((0) << 16)| 6);
}

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


TPCC_DB::TPCC_DB() {
  uint64_t district_back_valid = 0UL;
  uint64_t fill_new_order_entry_back_valid = 0UL;
  uint64_t update_order_entry_back_valid = 0UL;
  uint64_t update_stock_entry_num_valid = 0UL;
  uint64_t log_valid = 0UL;
  g_seed = 1312515;
}


unsigned TPCC_DB::fastrand() {
	g_seed = (179423891 * g_seed + 2038073749); 
	return (g_seed >> 8) & 0x7FFFFFFF;
}

void TPCC_DB::initialize(int _num_warehouses, int numThreads) {
  num_warehouses = _num_warehouses;
  int num_districts = 10*num_warehouses;
  int num_customers = 3000*num_districts;
  int num_stocks = NUM_ITEMS*num_warehouses;

  for(int i=0; i<3000; i++) {
    random_3000[i] = i;
  }
  for(int i=0; i<3000; i++) {
    int rand_loc = fastrand()%3000;
    int temp = random_3000[i];
    random_3000[i] = random_3000[rand_loc];
    random_3000[rand_loc] = temp;
  }
  /*
  perTxLocks = new queue_t[numThreads];
  for(int i=0; i<numThreads; i++) {
    perTxLocks[i].push(0);
    perTxLocks[i].pop();
  }
  */
  /*
  locks = new pthread_mutex_t[numLocks];
  for (int i = 0; i < numLocks; i++) {
      pthread_mutex_init(&locks[i],NULL);
  }
  */
  //Korakit
  //info removed
//  std::cout<<"Allocating tables"<<std::endl;
  
  int num_items = NUM_ITEMS;
  int num_histories = num_customers;
  int num_orders = 3000*num_districts;
  int num_order_lines = 15*num_orders; // Max possible, average is 10*num_orders
  int num_new_orders = 900*num_districts;

  size_t mapped_len;
  int is_pmem;
  void * pmemstart;

  int totsize = num_warehouses*sizeof(warehouse_entry) + num_districts*sizeof(district_entry) + num_customers*sizeof(customer_entry)
    + num_stocks*sizeof(stock_entry) + num_items*sizeof(item_entry) + num_histories*sizeof(history_entry) + num_orders*sizeof(order_entry)
    + num_new_orders*sizeof(new_order_entry) + num_order_lines*sizeof(order_line_entry);

  if ((pmemstart = pmem_map_file("/mnt/mem/tpcc", totsize,
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

  warehouse = (warehouse_entry*) pmemstart;//malloc(num_warehouses*sizeof(warehouse_entry));
  district = (district_entry*) (pmemstart + num_warehouses*sizeof(warehouse_entry));//malloc(num_districts*sizeof(district_entry));
  customer = (customer_entry*) (pmemstart + num_warehouses*sizeof(warehouse_entry) + num_districts*sizeof(district_entry));//malloc(num_customers*sizeof(customer_entry));
  stock = (stock_entry*) (pmemstart + num_warehouses*sizeof(warehouse_entry) + num_districts*sizeof(district_entry) + num_stocks*sizeof(stock_entry));//malloc(num_stocks*sizeof(stock_entry)); 

  item = (item_entry*) (pmemstart + num_warehouses*sizeof(warehouse_entry) + num_districts*sizeof(district_entry) + num_stocks*sizeof(stock_entry) + num_items*sizeof(item_entry) );//malloc(num_items*sizeof(item_entry));

  

  history = (history_entry*) (pmemstart + num_warehouses*sizeof(warehouse_entry) + num_districts*sizeof(district_entry) + num_stocks*sizeof(stock_entry) + num_items*sizeof(item_entry) + num_histories*sizeof(history_entry));//malloc(num_histories*sizeof(history_entry));

  order = (order_entry*) (pmemstart + num_warehouses*sizeof(warehouse_entry) + num_districts*sizeof(district_entry) + num_stocks*sizeof(stock_entry) + num_items*sizeof(item_entry) + num_histories*sizeof(history_entry) + num_orders*sizeof(order_entry));//malloc(num_orders*sizeof(order_entry));
  new_order = (new_order_entry*) (pmemstart + num_warehouses*sizeof(warehouse_entry) + num_districts*sizeof(district_entry) + num_stocks*sizeof(stock_entry) + num_items*sizeof(item_entry) + num_histories*sizeof(history_entry) + num_orders*sizeof(order_entry) + num_new_orders*sizeof(new_order_entry));//malloc(num_new_orders*sizeof(new_order_entry));
  order_line = (order_line_entry*) (pmemstart + num_warehouses*sizeof(warehouse_entry) + num_districts*sizeof(district_entry) + num_stocks*sizeof(stock_entry) + num_items*sizeof(item_entry) + num_histories*sizeof(history_entry) + num_orders*sizeof(order_entry) + num_new_orders*sizeof(new_order_entry) + num_order_lines*sizeof(order_line_entry));//malloc(num_order_lines*sizeof(order_line_entry));

  rndm_seeds = new unsigned long[NUM_RNDM_SEEDS];
  for(int i=0; i<NUM_RNDM_SEEDS; i++) {
    srand(i);
    rndm_seeds[i] = rand_local(1,NUM_RNDM_SEEDS*10);
  }

//Korakit
//info removed
/*
  std::cout<<"finished allocating tables"<<std::endl;

  std::cout<<"warehouse_entry: "<<sizeof(warehouse_entry)<<std::endl;
  std::cout<<"district_entry: "<<sizeof(district_entry)<<std::endl;
  std::cout<<"customer_entry: "<<sizeof(customer_entry)<<std::endl;
  std::cout<<"stock_entry: "<<sizeof(stock_entry)<<std::endl;
  std::cout<<"item_entry: "<<sizeof(item_entry)<<std::endl;
  std::cout<<"history_entry: "<<sizeof(history_entry)<<std::endl;
  std::cout<<"order_entry: "<<sizeof(order_entry)<<std::endl;
  std::cout<<"new_order_entry: "<<sizeof(new_order_entry)<<std::endl;
  std::cout<<"order_line_entry: "<<sizeof(order_line_entry)<<std::endl;
*/
}





TPCC_DB::~TPCC_DB(){
  free(warehouse);
  free(district);
  free(customer);
  free(stock);
  free(item);
  free(history);
  free(order);
  free(new_order);
  free(order_line);
}

void TPCC_DB::populate_tables() {
  //std::cout<<"populating item table"<<std::endl;
  for(int i=0; i<NUM_ITEMS; i++) {
    fill_item_entry(i+1);
  }
  //std::cout<<"finished populating item table"<<std::endl;

  for(int i=0; i<num_warehouses; i++) {
    fill_warehouse_entry(i+1);
    for(int j=0; j<NUM_ITEMS; j++) {
      fill_stock_entry(i+1, j+1);
    }
    //std::cout<<"finished populating stock table"<<std::endl;
    for(int j=0; j<10; j++) {
      fill_district_entry(i+1, j+1);
      for(int k=0; k<3000; k++) {
        fill_customer_entry(i+1, j+1, k+1);
        fill_history_entry(i+1, j+1, k+1);
        fill_order_entry(i+1, j+1, k+1);
      }
      for(int k=2100; k<3000; k++) {
        fill_new_order_entry(i+1, j+1, k+1, 0);
      }
    }
  }
}

//Korakit
//remove MT stuff
/*
void TPCC_DB::acquire_locks(int threadId, queue_t &requestedLocks) {
  // Acquire locks in order.
  int i = -1;
  while(!requestedLocks.empty()) {
    i = requestedLocks.front();
    perTxLocks[threadId].push(i);
    requestedLocks.pop();
    pthread_mutex_lock(&locks[i]);
  }
}

void TPCC_DB::release_locks(int threadId) {

  // Release locks in order
  int i = -1;
  while(!perTxLocks[threadId].empty()) {
    i = perTxLocks[threadId].front();
    perTxLocks[threadId].pop();
    pthread_mutex_unlock(&locks[i]);
  }
}
*/


void TPCC_DB::fill_item_entry(int _i_id) {
  int indx = (_i_id-1);
  item[indx].i_id = _i_id;
  item[indx].i_im_id = rand_local(1,NUM_ITEMS);
  random_a_string(14,24,item[indx].i_name);
  item[indx].i_price = rand_local(1,100)*(1.0);
  random_a_original_string(26,50,10,item[indx].i_data);
}

void TPCC_DB::fill_warehouse_entry(int _w_id) {
  int indx = (_w_id-1);
  warehouse[indx].w_id = _w_id;
  random_a_string(6,10,warehouse[indx].w_name);
  random_a_string(10,20,warehouse[indx].w_street_1);
  random_a_string(10,20,warehouse[indx].w_street_2);
  random_a_string(10,20,warehouse[indx].w_city);
  random_a_string(2,2,warehouse[indx].w_state);
  random_zip(warehouse[indx].w_zip);
  warehouse[indx].w_tax = (rand_local(0,20))/100.0;
  warehouse[indx].w_ytd = 300000.0;
}

void TPCC_DB::fill_stock_entry(int _s_w_id, int _s_i_id) {
  //std::cout<<"entered fill stock entry: "<<_s_w_id<<", "<<_s_i_id<<std::endl;
  int indx = (_s_w_id-1)*NUM_ITEMS + (_s_i_id-1);
  stock[indx].s_i_id = _s_i_id;
  //std::cout<<"1"<<std::endl;
  stock[indx].s_w_id = _s_w_id;
  //std::cout<<"1"<<std::endl;
  stock[indx].s_quantity = rand_local(10,100);
  //std::cout<<"1"<<std::endl;
  random_a_string(24,24,stock[indx].s_dist_01);
  //std::cout<<"1"<<std::endl;
  random_a_string(24,24,stock[indx].s_dist_02);
  //std::cout<<"1"<<std::endl;
  random_a_string(24,24,stock[indx].s_dist_03);
  //std::cout<<"1"<<std::endl;
  random_a_string(24,24,stock[indx].s_dist_04);
  //std::cout<<"1"<<std::endl;
  random_a_string(24,24,stock[indx].s_dist_05);
  //std::cout<<"1"<<std::endl;
  random_a_string(24,24,stock[indx].s_dist_06);
  //std::cout<<"1"<<std::endl;
  random_a_string(24,24,stock[indx].s_dist_07);
  //std::cout<<"1"<<std::endl;
  random_a_string(24,24,stock[indx].s_dist_08);
  //std::cout<<"1"<<std::endl;
  random_a_string(24,24,stock[indx].s_dist_09);
  //std::cout<<"1"<<std::endl;
  random_a_string(24,24,stock[indx].s_dist_10);
  //std::cout<<"1"<<std::endl;
  stock[indx].s_ytd = 0.0;
  //std::cout<<"1"<<std::endl;
  stock[indx].s_order_cnt = 0.0;
  //std::cout<<"1"<<std::endl;
  stock[indx].s_remote_cnt = 0.0;
  //std::cout<<"1"<<std::endl;
  random_a_original_string(26,50,10,stock[indx].s_data);
  //std::cout<<"exiting fill stock entry: "<<_s_w_id<<", "<<_s_i_id<<std::endl;
}

void TPCC_DB::fill_district_entry(int _d_w_id, int _d_id) {
  int indx = (_d_w_id-1)*10 + (_d_id-1);
  district[indx].d_id = _d_id;
  district[indx].d_w_id = _d_w_id;
  random_a_string(6,10,district[indx].d_name);
  random_a_string(10,20,district[indx].d_street_1);
  random_a_string(10,20,district[indx].d_street_2);
  random_a_string(10,20,district[indx].d_city);
  random_a_string(2,2,district[indx].d_state);
  random_zip(district[indx].d_zip);
  district[indx].d_tax = (rand_local(0,20))/100.0;
  district[indx].d_ytd = 30000.0;
  district[indx].d_next_o_id = 3001;
}

void TPCC_DB::fill_customer_entry(int _c_w_id, int _c_d_id, int _c_id) {
  int indx = (_c_w_id-1)*10*3000 + (_c_d_id-1)*3000 + (_c_id-1);
  customer[indx].c_id = _c_id;
  customer[indx].c_d_id = _c_d_id;
  customer[indx].c_w_id = _c_w_id;
  random_a_string(16,16,customer[indx].c_last); // FIXME: check tpcc manual for exact setting
  customer[indx].c_middle[0] = 'O';
  customer[indx].c_middle[1] = 'E';
  random_a_string(8,16,customer[indx].c_first);
  random_a_string(10,20,customer[indx].c_street_1);
  random_a_string(10,20,customer[indx].c_street_2);
  random_a_string(10,20,customer[indx].c_city);
  random_a_string(2,2,customer[indx].c_state);
  random_zip(customer[indx].c_zip);
  random_n_string(16,16, customer[indx].c_phone);
  fill_time(customer[indx].c_since);
  if(fastrand()%10 < 1) {
    customer[indx].c_credit[0] = 'G';
    customer[indx].c_credit[1] = 'C';
  }
  else {
    customer[indx].c_credit[0] = 'B';
    customer[indx].c_credit[1] = 'C';
  }
  customer[indx].c_credit_lim = 50000.0;
  customer[indx].c_discount = (rand_local(0,50))/100.0;
  customer[indx].c_balance = -10.0;
  customer[indx].c_ytd_payment = 10.0;
  customer[indx].c_payment_cnt = 1.0;
  customer[indx].c_delivery_cnt = 0.0;
  random_a_string(300,500,customer[indx].c_data);
}

void TPCC_DB::fill_history_entry(int _h_c_w_id, int _h_c_d_id, int _h_c_id) {
  int indx = (_h_c_w_id-1)*10*3000 + (_h_c_d_id-1)*3000 + (_h_c_id-1);
  history[indx].h_c_id = _h_c_id;
  history[indx].h_c_d_id = _h_c_d_id;
  history[indx].h_c_w_id = _h_c_w_id;
  fill_time(history[indx].h_date);
  history[indx].h_amount = 10.0;
  random_a_string(12,24,history[indx].h_data);
}

void TPCC_DB::fill_order_entry(int _o_w_id, int _o_d_id, int _o_id) {
  int indx = (_o_w_id-1)*10*3000 + (_o_d_id-1)*3000 + (_o_id-1);
  order[indx].o_id = _o_id;
  order[indx].o_c_id = random_3000[_o_id];
  order[indx].o_d_id = _o_d_id;
  order[indx].o_w_id = _o_w_id;
  fill_time(order[indx].o_entry_d);
  if(_o_id<2101)
    order[indx].o_carrier_id = fastrand()%10 + 1;
  else
    order[indx].o_carrier_id = 0;
  order[indx].o_ol_cnt = rand_local(5,15);
  order[indx].o_all_local = 1.0;
  for(int i=0; i<order[indx].o_ol_cnt; i++) {
    fill_order_line_entry(_o_w_id, _o_d_id, _o_id, i, order[indx].o_entry_d);
  }
}

void TPCC_DB::fill_order_line_entry(int _ol_w_id, int _ol_d_id, int _ol_o_id, int _o_ol_cnt, long long _o_entry_d) {
  int indx = (_ol_w_id-1)*10*3000*15 + (_ol_d_id-1)*3000*15 + (_ol_o_id-1)*15 + _o_ol_cnt;
  order_line[indx].ol_o_id = _ol_o_id;
  order_line[indx].ol_d_id = _ol_d_id;
  order_line[indx].ol_w_id = _ol_w_id;
  order_line[indx].ol_number = _o_ol_cnt;
  order_line[indx].ol_i_id = rand_local(1,NUM_ITEMS);
  order_line[indx].ol_supply_w_id = _ol_w_id;
  if(_ol_o_id < 2101) {
    order_line[indx].ol_delivery_d = _o_entry_d;
    order_line[indx].ol_amount = 0.0;
  }
  else {
    order_line[indx].ol_delivery_d = 0;
    order_line[indx].ol_amount = rand_local(1,999999)/100.0;
  }
  order_line[indx].ol_quantity = 5.0;
  random_a_string(24,24,order_line[indx].ol_dist_info);
}


#define CPTIME
#ifdef CPTIME
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
uint64_t totTimeMeta = 0;
uint64_t totTimeulog = 0;
#endif
void TPCC_DB::fill_new_order_entry(int _no_w_id, int _no_d_id, int _no_o_id, int threadId) {
  int indx = (_no_w_id-1)*10*900 + (_no_d_id-1)*900 + (_no_o_id-2101) % 900;
  // OPT_ADDR((void*)(7), threadId, &new_order[indx], sizeof(new_order_entry));
  // if(TPCC_DEBUG)
  //   std::cout<<"w_id, d_id, o_id, indx: "<<_no_w_id<<", "<<_no_d_id<<", "
  //                                               <<_no_o_id<<", "<<indx<<std::endl;
  //Korakit
  //do backup
  //backUpInst->fill_new_order_entry_indx = indx;
  new_order[indx].indx = indx;
  //backUpInst->new_order_entry_back = new_order[indx];
  //move_data(&backUpInst->new_order_entry_back, &new_order[indx], sizeof(backUpInst->new_order_entry_back));
#ifdef CPTIME
    uint64_t endCycles, startCycles,totalCycles;
	
    startCycles = getCycle();
#endif
  cmd_issue( 2, 1, 0, 0, (uint64_t)(&new_order[indx]), sizeof(backUpInst->new_order_entry_back), device); 
#ifdef CPTIME
        endCycles = getCycle();
        totalCycles = endCycles - startCycles;
       
        totTimeulog += (totalCycles);
		  printf("ulog %ld\n",totTimeulog);
#endif  

  //pmem_persist((void*)&backUpInst->new_order_entry_back, (unsigned)sizeof(backUpInst->new_order_entry_back));
  //s_fence();
  //backUpInst->fill_new_order_entry_back_valid=1;
  //s_fence();
  //just flush the cache
  new_order[indx].no_o_id = _no_o_id;
  new_order[indx].no_d_id = _no_d_id;
  new_order[indx].no_w_id = _no_w_id;
  pmem_persist((void*)&new_order[indx], (unsigned)sizeof(new_order[indx]));
}

int TPCC_DB::rand_local(int min, int max) {
  return (min + (fastrand()%(max-min+1)));
}

void TPCC_DB::random_a_string(int min, int max, char* string_ptr) {
  //std::cout<<"entered random a string"<<std::endl;
  char alphabets[26] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N',
                                'O','P','Q','R','S','T','U','V','W','X','Y','Z'};
  //std::cout<<"2"<<std::endl;
  int string_length = min + (fastrand()%(max-min+1));
  //std::cout<<"2"<<std::endl;
  for(int i=0; i<string_length; i++) {
    string_ptr[max-1-i] = alphabets[fastrand()%26];
    //std::cout<<"f3"<<std::endl;
  }
  //std::cout<<"2"<<std::endl;
  for(int i=0; i<max-string_length; i++) {
    string_ptr[max-1-i] = ' ';
    //std::cout<<"f4"<<std::endl;
  }
  //std::cout<<"exiting random a string"<<std::endl;
}
void TPCC_DB::random_a_original_string(int min, int max, int probability, char* string_ptr) {
  //FIXME: use probability and add ORIGINAL
  random_a_string(min, max,string_ptr);
}

void TPCC_DB::random_zip(char* string_ptr) {
  random_a_string(4,4,string_ptr);
  for(int i=4; i<9; i++) {
    string_ptr[i] = '1';
  }
}

void TPCC_DB::random_n_string(int min, int max, char* string_ptr) {
  char digits[10] = {'0','1','2','3','4','5','6','7','8','9'};
  int string_length = min + (fastrand()%(max-min+1));
  for(int i=0; i<string_length; i++) {
    string_ptr[max-1-i] = digits[fastrand()%10];
  }
  for(int i=0; i<max-string_length; i++) {
    string_ptr[max-1-i] = ' ';
  }
}

void TPCC_DB::fill_time(long long &time_slot) {
  //FIXME: put correct time
  time_slot = 12112342433241;
}

void TPCC_DB::copy_district_info(district_entry &dest, district_entry &source) {
  std::memcpy(&dest, &source, sizeof(district_entry));
}

void TPCC_DB::copy_customer_info(customer_entry &dest, customer_entry &source) {
  std::memcpy(&dest, &source, sizeof(customer_entry));
}

void TPCC_DB::copy_new_order_info(new_order_entry &dest, new_order_entry &source) {
  std::memcpy(&dest, &source, sizeof(new_order_entry));
}

void TPCC_DB::copy_order_info(order_entry &dest, order_entry &source) {
  std::memcpy(&dest, &source, sizeof(order_entry));
}

void TPCC_DB::copy_stock_info(stock_entry &dest, stock_entry &source) {
  std::memcpy(&dest, &source, sizeof(stock_entry));
}

void TPCC_DB::copy_order_line_info(order_line_entry &dest, order_line_entry &source) {
  std::memcpy(&dest, &source, sizeof(order_line_entry));
}

void TPCC_DB::update_order_entry(int _w_id, short _d_id, int _o_id, int _c_id, int _ol_cnt, int threadId) {
  int indx = (_w_id-1)*10*3000 + (_d_id-1)*3000 + (_o_id-1)%3000;
  // OPT((void*)(8), threadId, &backUpInst->order_entry_back, &order[indx], sizeof(order_entry));
  // OPT_ADDR((void*)(9), threadId, &order[indx], sizeof(order_entry));
  
  // Korakit
  // create backup
  // fprintf(stdout, "thread=%d, line=%d\n", threadId, __LINE__);
  //backUpInst->update_order_entry_indx = indx;
  order[indx].indx = indx;
  //backUpInst->order_entry_back = order[indx];
  pmem_persist((void*)&backUpInst->update_order_entry_indx, (unsigned)sizeof(backUpInst->update_order_entry_indx));
  //move_data(&backUpInst->order_entry_back, &order[indx],(unsigned)sizeof(backUpInst->order_entry_back));
#ifdef CPTIME
    uint64_t endCycles, startCycles,totalCycles;
	
    startCycles = getCycle();
#endif
  cmd_issue( 2, 1, 0, 0, (uint64_t)(&order[indx]), sizeof(backUpInst->order_entry_back), device); 

#ifdef CPTIME
        endCycles = getCycle();
        totalCycles = endCycles - startCycles;
       
        totTimeulog += (totalCycles);
		  printf("ulog %ld\n",totTimeulog);
#endif  
  //pmem_persist((void*)&backUpInst->order_entry_back, (unsigned)sizeof(backUpInst->order_entry_back));
  //s_fence();
  // fprintf(stdout, "thread=%d, line=%d\n", threadId, __LINE__);
  //backUpInst->update_order_entry_back_valid = 1;
  //s_fence();

  order[indx].o_id = _o_id;
  order[indx].o_carrier_id = 0;
  order[indx].o_all_local = 1;
  order[indx].o_ol_cnt = _ol_cnt;
  order[indx].o_c_id = _c_id;

  fill_time(order[indx].o_entry_d);

  pmem_persist((void*)&order[indx], (unsigned)sizeof(order[indx]));
  s_fence();
}

void TPCC_DB::update_stock_entry(int threadId, int _w_id, int _i_id, int _d_id, float &amount, int itr) {
  int indx = (_w_id-1)*NUM_ITEMS + _i_id-1;
  //int ol_quantity = get_random(threadId, 1, 10);
  int ol_quantity = 7;
  // OPT_ADDR((void*)(0x20), threadId, &stock[indx], sizeof(stock_entry));
  // fprintf(stdout, "thread=%d, line=%d\n", threadId, __LINE__);
  //backUpInst->update_stock_entry_indx[itr] = indx;
  stock[indx].indx = indx;
  //backUpInst->stock_entry_back[itr] = stock[indx];

  //backUpInst->update_stock_entry_num_valid = itr+1;
  //pmem_persist((void*)&backUpInst->update_stock_entry_indx[itr], (unsigned)sizeof(backUpInst->update_stock_entry_indx[itr]));
  //move_data(&backUpInst->stock_entry_back[itr], &stock[indx], (unsigned)sizeof(backUpInst->stock_entry_back[itr]));
#ifdef CPTIME
    uint64_t endCycles, startCycles,totalCycles;
	
    startCycles = getCycle();
#endif
  cmd_issue( 2, 1, 0, 0, (uint64_t)(&stock[indx]), sizeof(backUpInst->stock_entry_back[itr]), device); 

#ifdef CPTIME
        endCycles = getCycle();
        totalCycles = endCycles - startCycles;
       
        totTimeulog += (totalCycles);
		  printf("ulog %ld\n",totTimeulog);
#endif  
  //pmem_persist((void*)&backUpInst->stock_entry_back[itr], (unsigned)sizeof(backUpInst->stock_entry_back[itr]));
  //s_fence();
  // fprintf(stdout, "%d\n", __LINE__);
  if(stock[indx].s_quantity - ol_quantity > 10) {
    stock[indx].s_quantity -= ol_quantity;
  }
  else {
    stock[indx].s_quantity -= ol_quantity;
    stock[indx].s_quantity += 91;
  }
  
  stock[indx].s_ytd += ol_quantity;
  stock[indx].s_order_cnt += 1;
  //flush_caches((void*)&stock[indx], (unsigned)sizeof(stock[indx]));
  //s_fence();
  // fprintf(stdout, "%d\n", __LINE__);
  
  //Korakit
  //volatile
  amount += ol_quantity * item[_i_id-1].i_price;
}

void TPCC_DB::new_order_tx(int threadId, int w_id, int d_id, int c_id) {
  
  // OPT_VAL((void*)(1), threadId, (void*)backUpInst->district_back_valid.getPtr(), 0);
  // OPT_VAL((void*)(2), threadId, (void*)backUpInst->fill_new_order_entry_back_valid.getPtr(), 0);
  // OPT_VAL((void*)(3), threadId, (void*)backUpInst->update_order_entry_back_valid.getPtr(), 0);
  // OPT_VAL((void*)(4), threadId, (void*)backUpInst->update_stock_entry_num_valid.getPtr(), 0);
  
  int w_indx = (w_id-1);
  int d_indx = (w_id-1)*10 + (d_id-1);
  int c_indx = (w_id-1)*10*3000 + (d_id-1)*3000 + (c_id-1);
  // OPT((void*)(5), threadId, &backUpInst->district_back, &district[d_indx], sizeof(backUpInst->district_back));
  // OPT_ADDR((void*)(6), threadId, &backUpInst->new_order_entry_back, sizeof(backUpInst->new_order_entry_back));
  /*
  queue_t reqLocks;
  reqLocks.push(d_indx); // Lock for district
  */
  /*
  if(TPCC_DEBUG)
    std::cout<<"**NOTx** district lock id: "<<d_indx<<std::endl; 
  */
//  fprintf(stdout, "%d\n", __LINE__);
  int ol_cnt = get_random(threadId, 5, 15);
  int item_ids[ol_cnt];
  for(int i=0; i<ol_cnt; i++) {
    int new_item_id;
    bool match;
    do {
      match = false;
      new_item_id = get_random(threadId, 1, NUM_ITEMS);
      for(int j=0; j<i; j++) {
        if(new_item_id == item_ids[j]) {
          match = true;
          break;
        }
      }
    } while (match);
    item_ids[i] = new_item_id;
  }

  // fprintf(stdout, "%d\n", __LINE__);
  std::sort(item_ids, item_ids+ol_cnt);
  // fprintf(stdout, "%d\n", __LINE__);
  /*
  if(TPCC_DEBUG)
    std::cout<<"**NOTx** ol_cnt: "<<ol_cnt<<std::endl; 
  */
  for(int i=0; i<ol_cnt; i++) {
    int item_lock_id = num_warehouses*10 + (w_id-1)*NUM_ITEMS + item_ids[i] - 1;
	/*
    reqLocks.push(item_lock_id); // Lock for each item in stock table
    */
    /*
    if(TPCC_DEBUG)
      std::cout<<"**NOTx** item lock id: "<<item_lock_id<<" thread id: "<<threadId<<std::endl; 
    */
  }
  //Korakit
  //remove MT stuff
  //acquire_locks(threadId, reqLocks);
  /*
  if(TPCC_DEBUG)
    std::cout<<"**NOTx** finished start tx: "<<std::endl; 
  */

  float w_tax = warehouse[w_indx].w_tax;
  
  float d_tax = district[d_indx].d_tax;
  int d_o_id = district[d_indx].d_next_o_id;
  int no_indx = (w_id-1)*10*900 + (d_id-1)*900 + (d_o_id-2101) % 900;

  int o_indx = (w_id-1)*10*3000 + (d_id-1)*3000 + (d_o_id-1)%3000;

//Korakit
//real stuff here
// okay we gonna try really simple stuff first
// let's force all writes when the transaction completes
//  flush_caches(uint64_t addr, unsigned size);
//  s_fence();
  // fprintf(stdout, "%d\n", __LINE__);
  //prepare backup log
  backUpInst->district_back_valid = 0;
  backUpInst->fill_new_order_entry_back_valid = 0;
  backUpInst->update_order_entry_back_valid = 0;
  backUpInst->update_stock_entry_num_valid = 0;
  s_fence();
  // OPT_VAL((void*)(0x41), threadId, (void*)backUpInst->district_back_valid.getPtr(), 1);
  // OPT_VAL((void*)(0x42), threadId, (void*)backUpInst->fill_new_order_entry_back_valid.getPtr(), 1);
  // OPT_VAL((void*)(0x43), threadId, (void*)backUpInst->update_order_entry_back_valid.getPtr(), 1);

  backUpInst->log_valid = 1;
  pmem_persist((void*)&backUpInst->log_valid, (unsigned)sizeof(backUpInst->log_valid));
  s_fence();
  for(int i=0; i<ol_cnt; i++) {
    // OPT_ADDR((void*)(0x100UL+i), threadId, &backUpInst->stock_entry_back[i], sizeof(stock_entry));
  }
  //do backup
  //fprintf(stdout, "%d\n", __LINE__);
  //backUpInst->district_back = district[d_indx];
  //pmem_persist(&backUpInst->district_back, sizeof(backUpInst->district_back));
  //move_data(&backUpInst->district_back, &district[d_indx],sizeof(backUpInst->district_back));
#ifdef CPTIME
    uint64_t endCycles, startCycles,totalCycles;
	
    startCycles = getCycle();
#endif
  cmd_issue( 2, 1, 0, 0, (uint64_t)(&district[d_indx]), sizeof(backUpInst->district_back), device);

#ifdef CPTIME
        endCycles = getCycle();
        totalCycles = endCycles - startCycles;
       
        totTimeulog += (totalCycles);
		  printf("ulog %ld\n",totTimeulog);
#endif  
  district[d_indx].d_next_o_id++;
  //flush district[d_indx].d_next_o_id++;
  //pmem_persist((void*)&district[d_indx].d_next_o_id, (unsigned)sizeof(district[d_indx].d_next_o_id));
  //s_fence();
  // fprintf(stdout, "%d\n", __LINE__);
  fill_new_order_entry(w_id,d_id,d_o_id, threadId);
  
  // fprintf(stdout, "%d\n", __LINE__);
  update_order_entry(w_id, d_id, d_o_id, c_id, ol_cnt, threadId);
  // fprintf(stdout, "%d\n", __LINE__);
  
  float total_amount = 0.0;
  for(int i=0; i<ol_cnt; i++) {
    update_stock_entry(threadId, w_id, item_ids[i], d_id, total_amount, i);
  }
  // fprintf(stdout, "%d\n", __LINE__);
  //invalidate log entries
  backUpInst->log_valid = 0;
  pmem_persist((void*)&backUpInst->log_valid, (unsigned)sizeof(backUpInst->log_valid));
  s_fence();
  // fprintf(stdout, "%d\n", __LINE__);
/////////////////
  
//Korakit
//debug removed
/*
  if(TPCC_DEBUG)
    std::cout<<"d_id, d_o_id, ol_cnt, total_amount: "<<d_id<<", "<<d_o_id<<", "<<
                                                ol_cnt<<", "<<total_amount<<std::endl;
*/
//Korakit
//remove MT stuffs
  //release_locks(threadId);
  return;
}


unsigned long TPCC_DB::get_random(int thread_id) {
  unsigned long tmp;
  tmp = rndm_seeds[thread_id*10] = (rndm_seeds[thread_id*10] * 16807) % 2147483647;
  //return rand()%(2^32-1);
  return tmp;
}

unsigned long TPCC_DB::get_random(int thread_id, int min, int max) {
  unsigned long tmp;
  //return min+(rand()%(max-min+1));
  tmp = rndm_seeds[thread_id*10] = (rndm_seeds[thread_id*10] * 16807) % 2147483647;
  return min+(tmp%(max-min+1));
  //return tmp
}
//Korakit
//debug removed
/*
void TPCC_DB::printStackPointer(int* sp, int thread_id) {
  std::cout<<"Stack Heap: "<<sp<<std::endl;
}
*/
