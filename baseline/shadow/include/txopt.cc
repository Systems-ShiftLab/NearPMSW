#include "txopt.h"
#include <string.h>

// source: http://stackoverflow.com/questions/1919183/how-to-allocate-and-free-aligned-memory-in-c
void *
aligned_malloc(int size) {
	void *mem = malloc(size+64+sizeof(void*));
	void **ptr = (void**)((uintptr_t)((uint64_t)mem+64+uint64_t(sizeof(void*))) & ~(64-1));
	ptr[-1] = mem;
	return ptr;
}

// source: http://stackoverflow.com/questions/1640258/need-a-fast-random-generator-for-c
static unsigned long x=123456789, y=362436069, z=521288629;

unsigned long xorshf96() {          //period 2^96-1
	unsigned long t;
	x ^= x << 16;
	x ^= x >> 5;
	x ^= x << 1;
    t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;
    return z;
}


//volatile  void s_fence();
// Flush the selected addresses
//volatile void metadata_cache_flush(uint64_t addr, unsigned size);
//volatile void cache_flush(uint64_t addr, unsigned size);
//volatile void flush_caches(uint64_t addr, unsigned size);
// Flush the one cacheline
//volatile inline void metadata_flush(uint64_t addr);
//volatile inline void cache_flush(uint64_t addr);
// Flush the whole caches
//volatile inline void metadata_flush();
//volatile inline void cache_flush();

//volatile void TX_OPT(uint64_t addr, unsigned size);


// Deduplication and Compression are transaparent
/*
class Dedup {
	public:

};


class Compress {
	public:
}
*/


uint64_t CounterAtomic::currAtomicAddr = COUNTER_ATOMIC_VADDR;
//uint64_t CounterAtomic::currCacheFlushAddr = CACHE_FLUSH_VADDR;
//uint64_t CounterAtomic::currCounterCacheFlushAddr = COUNTER_CACHE_FLUSH_VADDR;


void*
CounterAtomic::counter_atomic_malloc(unsigned _size) {
	return (void*)getNextAtomicAddr(_size);	
}


volatile void
metadata_cache_flush(void* addr, unsigned size) {
	int num_cache_line = size / CACHE_LINE_SIZE;
	if ((uint64_t)addr % CACHE_LINE_SIZE)
		num_cache_line++;

	for (int i = 0; i < num_cache_line; ++i)
		*((volatile uint64_t*)METADATA_CACHE_FLUSH_VADDR) = (uint64_t)addr + i * CACHE_LINE_SIZE;
}

volatile void
cache_flush(void* addr, unsigned size) {
	int num_cache_line = size / CACHE_LINE_SIZE;
	if ((uint64_t)addr % CACHE_LINE_SIZE)
		num_cache_line++;

	for (int i = 0; i < num_cache_line; ++i)
		*((volatile uint64_t*)CACHE_FLUSH_VADDR) = (uint64_t)addr + i * CACHE_LINE_SIZE;
}

volatile void
flush_caches(void* addr, unsigned size) {
	cache_flush(addr, size);
	metadata_cache_flush(addr, size);
}

// OPT with both data and addr ready
volatile void
OPT(void* opt_obj, bool reg, void* pmemaddr, void* data, unsigned size) {
	// fprintf(stderr, "size: %u\n", size);
	opt_packet_t opt_packet;

	opt_packet.opt_obj = opt_obj;
	//opt_packet.seg_id = i;
	//opt_packet.pmemaddr = (void*)((uint64_t)(pmemaddr) + i * CACHE_LINE_SIZE);
	opt_packet.pmemaddr = pmemaddr;
	//opt_packet.data_ptr = (void*)((uint64_t)(data) + i * CACHE_LINE_SIZE);
	//opt_packet.data_val = 0;
	opt_packet.size = size;
	opt_packet.type = (!reg ? FLAG_OPT : FLAG_OPT_REG);
	//opt_packet.type = FLAG_OPT;
	*((opt_packet_t*)TXOPT_VADDR) = opt_packet;
	//*((opt_packet_t*)TXOPT_VADDR) = (opt_packet_t){opt_obj, pmemaddr, size, FLAG_OPT_DATA};
}

// OPT with both data (int) and addr ready
volatile void 
OPT_VAL(void* opt_obj, bool reg, void* pmemaddr, int data_val) {
	opt_packet_t opt_packet;

	opt_packet.opt_obj = opt_obj;
	opt_packet.pmemaddr = pmemaddr;
	//opt_packet.data_ptr = 0;
	//opt_packet.data_val = data_val;
	opt_packet.size = sizeof(int);
	opt_packet.type = (!reg ? FLAG_OPT_VAL : FLAG_OPT_VAL_REG);
	//opt_packet.type = FLAG_OPT;
	*((opt_packet_t*)TXOPT_VADDR) = opt_packet;
}


// OPT with only data ready
volatile void
OPT_DATA(void* opt_obj, bool reg, void* data, unsigned size) {
	opt_packet_t opt_packet;
	
	opt_packet.opt_obj = opt_obj;
	opt_packet.pmemaddr = 0;
	//opt_packet.data_ptr = (void*)((uint64_t)(data) + i * CACHE_LINE_SIZE);
	//opt_packet.data_val = 0;
	opt_packet.size = size;
	opt_packet.type = (!reg ? FLAG_OPT_DATA : FLAG_OPT_DATA_REG);
	//opt_packet.type = FLAG_OPT;
	*((opt_packet_t*)TXOPT_VADDR) = opt_packet;
}

// OPT with only addr ready
volatile void
OPT_ADDR(void* opt_obj, bool reg, void* pmemaddr, unsigned size) {
	opt_packet_t opt_packet;
	
	opt_packet.opt_obj = opt_obj;
	opt_packet.pmemaddr = pmemaddr;
	//opt_packet.data_ptr = 0;
	//opt_packet.data_val = 0;
	opt_packet.size = size;
	opt_packet.type = (!reg ? FLAG_OPT_ADDR : FLAG_OPT_ADDR_REG);
	//opt_packet.type = FLAG_OPT;
	*((opt_packet_t*)TXOPT_VADDR) = opt_packet;
}

// OPT with only data (int) ready
volatile void
OPT_DATA_VAL(void* opt_obj, bool reg, int data_val) {
	opt_packet_t opt_packet;

	opt_packet.opt_obj = opt_obj;
	opt_packet.pmemaddr = 0;
	//opt_packet.data_ptr = 0;
	//opt_packet.data_val = data_val;
	opt_packet.size = sizeof(int);
	opt_packet.type = (!reg ? FLAG_OPT_DATA_VAL : FLAG_OPT_DATA_VAL_REG);
	//opt_packet.type = FLAG_OPT;
	*((opt_packet_t*)TXOPT_VADDR) = opt_packet;
}

volatile void 
OPT_START(void* opt_obj) {
	opt_packet_t opt_packet;
	opt_packet.opt_obj = opt_obj;
	opt_packet.type = FLAG_OPT_START;
}

volatile void
s_fence() {
	std::atomic_thread_fence(std::memory_order_acq_rel);
}

CounterAtomic::CounterAtomic() {
	val_addr = getNextAtomicAddr(CACHE_LINE_SIZE);
}

CounterAtomic::CounterAtomic(uint64_t _val) {
	val_addr = getNextAtomicAddr(CACHE_LINE_SIZE);
	*((volatile uint64_t*)val_addr) = _val;
}

CounterAtomic::CounterAtomic(bool _val) {
	*((volatile uint64_t*)val_addr) = uint64_t(_val);
	val_addr = getNextAtomicAddr(CACHE_LINE_SIZE);
}

uint64_t
CounterAtomic::getValue() {
	return *((volatile uint64_t*)val_addr);
}

uint64_t
CounterAtomic::getPtr() {
	return val_addr;
}

CounterAtomic&
CounterAtomic::operator=(uint64_t _val) {
	*((volatile uint64_t*)val_addr) = _val;
	return *this;
}

CounterAtomic& 
CounterAtomic::operator+(uint64_t _val) {
	*((volatile uint64_t*)val_addr) += _val;
	return *this;
}

CounterAtomic& 
CounterAtomic::operator++() {
	uint64_t val = *((volatile uint64_t*)val_addr);
	val++;
	*((volatile uint64_t*)val_addr) = val;
	return *this;
}

CounterAtomic& 
CounterAtomic::operator--() {
	uint64_t val = *((volatile uint64_t*)val_addr);
	val--;
	*((volatile uint64_t*)val_addr) = val;
	return *this;
}

CounterAtomic& 
CounterAtomic::operator-(uint64_t _val) {
	*((volatile uint64_t*)val_addr) -= _val;
	return *this;
}

bool 
CounterAtomic::operator==(uint64_t _val) {
	return *((volatile uint64_t*)val_addr) == _val;
}

bool 
CounterAtomic::operator!=(uint64_t _val) {
	return *((volatile uint64_t*)val_addr) != _val;
}

uint64_t 
CounterAtomic::getNextAtomicAddr(unsigned _size) {
	if (currAtomicAddr + _size >= COUNTER_ATOMIC_VADDR + NUM_COUNTER_ATOMIC_PAGE*4*1024) {
		printf("@@not enough counter atomic space, current addr=%lu, size=%u\n", currAtomicAddr, _size);
		exit(0);
	}
	currAtomicAddr += _size;
	return (currAtomicAddr - _size);
}

volatile void
CounterAtomic::statOutput() {
	*((volatile uint64_t*) (STATUS_OUTPUT_VADDR))= 0;
}

volatile void
CounterAtomic::initCounterCache() {
	*((volatile uint64_t*) (INIT_METADATA_CACHE_VADDR))= 0;
}

