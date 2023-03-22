// The starting address of the selected counter_atomic writes
#ifndef TXOPT_H
#define TXOPT_H
#define COUNTER_ATOMIC_VADDR (4096UL*1024*1024)
#define NUM_COUNTER_ATOMIC_PAGE 262144

// The starting address of the flush cache instruction
#define CACHE_FLUSH_VADDR (4096UL*1024*1024+4*NUM_COUNTER_ATOMIC_PAGE*1024)

// The starting address of the flush metadata cache instruction
#define METADATA_CACHE_FLUSH_VADDR (4096UL*1024*1024+(4*NUM_COUNTER_ATOMIC_PAGE+4)*1024)

#define STATUS_OUTPUT_VADDR (METADATA_CACHE_FLUSH_VADDR + 1024UL)
#define INIT_METADATA_CACHE_VADDR (STATUS_OUTPUT_VADDR + 1024UL)
#define TXOPT_VADDR (INIT_METADATA_CACHE_VADDR+1024UL)

#define CACHE_LINE_SIZE 64UL

#include <vector>
#include <deque>
#include <cstdlib>
#include <cstdint>
#include <atomic>
#include <stdio.h>
#include <cassert>

enum opt_flag {
	FLAG_OPT,
	FLAG_OPT_VAL,
	FLAG_OPT_ADDR,
	FLAG_OPT_DATA,
	FLAG_OPT_DATA_VAL,
	/* register no execute */
	FLAG_OPT_REG,
	FLAG_OPT_VAL_REG,
	FLAG_OPT_ADDR_REG,
	FLAG_OPT_DATA_REG,
	FLAG_OPT_DATA_VAL_REG,
	/* execute registered OPT */
	FLAG_OPT_START
};

struct opt_t {
	//int pid;
	int obj_id;
};

// Fields in the OPT packet
// Used by both SW and HW
struct opt_packet_t {
	void* opt_obj;
	void* pmemaddr;
	//void* data_ptr;
	//int seg_id;
	//int data_val;
	unsigned size;
	opt_flag type;
};

// OPT with both data and addr ready
volatile void OPT(void* opt_obj, bool reg, void* pmemaddr, void* data, unsigned size);
//#define OPT(opt_obj, pmemaddr, data, size) \
//	*((opt_packet_t*)TXOPT_VADDR) = (opt_packet_t){opt_obj, pmemaddr, size, FLAG_OPT_DATA};
	
// OPT with both data (int) and addr ready
volatile void OPT_VAL(void* opt_obj, bool reg, void* pmemaddr, int data_val);
// OPT with only data ready
volatile void OPT_DATA(void* opt_obj, bool reg, void* data, unsigned size);
// OPT with only addr ready
volatile void OPT_ADDR(void* opt_obj, bool reg, void* pmemaddr, unsigned size);
// OPT with only data (int) ready
volatile void OPT_DATA_VAL(void* opt_obj, bool reg, int data_val);

// Begin OPT operation
volatile void OPT_START(void* opt_obj);

// store barrier
volatile void s_fence();

// flush both metadata cache and data cache
volatile void flush_caches(void* addr, unsigned size);

// flush data cache only
volatile void cache_flush(void* addr, unsigned size);
// flush metadata cache only
volatile void metadata_cache_flush(void* addr, unsigned size);

// malloc that is cache-line aligned
void *aligned_malloc(int size);



class CounterAtomic {
	public:
		static void* counter_atomic_malloc(unsigned _size);
		// size is num of bytes
		
		static volatile void statOutput();
		static volatile void initCounterCache();

		uint64_t getValue();
		uint64_t getPtr();
		
		CounterAtomic();
		CounterAtomic(uint64_t _val);
		CounterAtomic(bool _val);
		CounterAtomic& operator=(uint64_t _val);
		CounterAtomic& operator+(uint64_t _val);
		CounterAtomic& operator++();
		CounterAtomic& operator--();
		CounterAtomic& operator-(uint64_t _val);
		bool operator==(uint64_t _val);
		bool operator!=(uint64_t _val);

	private:
		void init();
		static uint64_t getNextAtomicAddr(unsigned _size);
		static uint64_t getNextCacheFlushAddr(unsigned _size);
		//static uint64_t getNextPersistBarrierAddr(unsigned _size);
		static uint64_t getNextCounterCacheFlushAddr(unsigned _size);
		
		static uint64_t currAtomicAddr;
		static uint64_t currCacheFlushAddr;
		//static uint64_t currPersistentBarrierAddr;
		static uint64_t currCounterCacheFlushAddr;
/*
		static bool hasAllocateCacheFlush;
		static bool hasAllocateCounterCacheFlush;
		static bool hasAllocatePersistBarrier;
*/
		//uint64_t val;
		uint64_t val_addr = 0;
};
#endif
