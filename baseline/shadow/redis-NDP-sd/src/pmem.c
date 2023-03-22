/*
 * Copyright (c) 2017, Andreas Bluemle <andreas dot bluemle at itxperts dot de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef USE_PMDK
#include "server.h"
#include "obj.h"
#include "libpmemobj.h"
#include "util.h"


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
int updated_page_count1 = 0;
int all_updates1 = 0;
void * checkpoint_start = NULL;
bool init = true;
void * page[50];
PMEMobjpool *pop;
void * device;

static void* open_device1(const char* pathname)
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
#define CPTIME

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
		//printf("%d %08x\n",TID, issue_cmd[i]);
		*((u_int32_t *) ptr) = issue_cmd[i];
	}


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
#endif
static  void makecheckpoint( void * ptr) {

#ifdef CPTIME
    uint64_t endCycles, startCycles,totalCycles;
	
    startCycles = getCycle();
#endif
    if(init){
        size_t mapped_len1;
    int is_pmem1;
    if ((checkpoint_start = pmem_map_file("/mnt/mem/checkpoint", 4096*50,
        PMEM_FILE_CREATE, 0666, &mapped_len1, &is_pmem1)) == NULL) {
        fprintf(stderr, "pmem_map_file failed\n");
        exit(0);
    }
    device = open_device1("/sys/devices/pci0000:00/0000:00:00.2/iommu/ivhd0/devices/0000:0a:00.0/resource0");
    init = false;
    }
    uint64_t pageNo = ((uint64_t)ptr)/4096;
	unsigned long * pageStart = (unsigned long *)(pageNo*4096);
    uint64_t cp_count = 0;
//    memcpy(checkpoint_start , pageStart,4096);
//    pmem_persist( checkpoint_start ,4096);	 
    if(all_updates1 >  0 || updated_page_count1 == 40){
			for(int i=0;i<updated_page_count1;i++){
                cp_count++;
				memcpy(checkpoint_start + i*4096, page[i],4096);
				pmem_persist( checkpoint_start + i*4096,4096);	 
                //cmd_issue(2,0,0,0, (uint64_t)(checkpoint_start + i*4096),4096,device);

				page[updated_page_count1] = 0;
			}
            printf("pagecnt %ld\n",cp_count);
			updated_page_count1 = 0;
            all_updates1 = 0;
		}
    all_updates1++;
    for(int i=0; i<updated_page_count1; i++){
		if(page[i] == pageStart){
#ifdef CPTIME
            endCycles = getCycle();
            totalCycles = endCycles - startCycles;
       
            double totTime = ((double)totalCycles)/2000000000;
            printf("cp %f\n", totTime);
            printf("cycles = %ld\n", totalCycles);
#endif     
            return;
        }
    }

    page[updated_page_count1] = pageStart;
    updated_page_count1++;
#ifdef CPTIME
            endCycles = getCycle();
            totalCycles = endCycles - startCycles;
       
            double totTime = ((double)totalCycles)/2000000000;
            printf("cp %f\n", totTime);
            printf("cycles = %ld\n", totalCycles);
#endif 
}





///////////////////////////////////////////////////////////////
int
pmemReconstruct(void)
{
    TOID(struct redis_pmem_root) root;
    TOID(struct key_val_pair_PM) kv_PM_oid;
    struct key_val_pair_PM *kv_PM;
    dict *d;
    void *key;
    void *val;
    void *pmem_base_addr;

    root = server.pm_rootoid;
    pmem_base_addr = (void *)server.pm_pool->addr;
    d = server.db[0].dict;
    dictExpand(d, D_RO(root)->num_dict_entries);
    for (kv_PM_oid = D_RO(root)->pe_first; TOID_IS_NULL(kv_PM_oid) == 0; kv_PM_oid = D_RO(kv_PM_oid)->pmem_list_next){
		kv_PM = (key_val_pair_PM *)(kv_PM_oid.oid.off + (uint64_t)pmem_base_addr);
		key = (void *)(kv_PM->key_oid.off + (uint64_t)pmem_base_addr);
		val = (void *)(kv_PM->val_oid.off + (uint64_t)pmem_base_addr);

        (void)dictAddReconstructedPM(d, key, val);
    }
    return C_OK;
}

void pmemKVpairSet(void *key, void *val)
{
    PMEMoid *kv_PM_oid;
    PMEMoid val_oid;
    struct key_val_pair_PM *kv_PM_p;

    kv_PM_oid = sdsPMEMoidBackReference((sds)key);
    kv_PM_p = (struct key_val_pair_PM *)pmemobj_direct(*kv_PM_oid);

    val_oid.pool_uuid_lo = server.pool_uuid_lo;
    val_oid.off = (uint64_t)val - (uint64_t)server.pm_pool->addr;

    //setpage(&(kv_PM_p->val_oid));
    //printf("B");
    struct key_val_pair_PM a;
    kv_PM_p = &a;
    kv_PM_p->val_oid = val_oid;
    makecheckpoint(&(kv_PM_p));
    TX_ADD_FIELD_DIRECT(kv_PM_p, val_oid);
   // printf("A");
    
    return;
}

PMEMoid
pmemAddToPmemList(void *key, void *val)
{
    PMEMoid key_oid;
    PMEMoid val_oid;
    PMEMoid kv_PM;
    struct key_val_pair_PM *kv_PM_p;
    TOID(struct key_val_pair_PM) typed_kv_PM;
    struct redis_pmem_root *root;

    key_oid.pool_uuid_lo = server.pool_uuid_lo;
    key_oid.off = (uint64_t)key - (uint64_t)server.pm_pool->addr;

    val_oid.pool_uuid_lo = server.pool_uuid_lo;
    val_oid.off = (uint64_t)val - (uint64_t)server.pm_pool->addr;

    kv_PM = pmemobj_tx_zalloc(sizeof(struct key_val_pair_PM), pm_type_key_val_pair_PM);
    kv_PM_p = (struct key_val_pair_PM *)pmemobj_direct(kv_PM);
    kv_PM_p->key_oid = key_oid;
    kv_PM_p->val_oid = val_oid;
    typed_kv_PM.oid = kv_PM;

    root = pmemobj_direct(server.pm_rootoid.oid);

    kv_PM_p->pmem_list_next = root->pe_first;
    if(!TOID_IS_NULL(root->pe_first)) {
        struct key_val_pair_PM *head = D_RW(root->pe_first);
        TX_ADD_FIELD_DIRECT(head,pmem_list_prev);
        makecheckpoint(&(head->pmem_list_prev));
                //serverLog(LL_NOTICE,"ulog\n");
                //printf("ulog\n");

    	head->pmem_list_prev = typed_kv_PM;
    }

    TX_ADD_DIRECT(root);
    makecheckpoint(root);
            //serverLog(LL_NOTICE,"ulog\n");
            //printf("ulog\n");

    root->pe_first = typed_kv_PM;
    root->num_dict_entries++;

    return kv_PM;
}

void
pmemRemoveFromPmemList(PMEMoid kv_PM_oid)
{
    TOID(struct key_val_pair_PM) typed_kv_PM;
    struct redis_pmem_root *root;

    root = pmemobj_direct(server.pm_rootoid.oid);

    typed_kv_PM.oid = kv_PM_oid;

    if(TOID_EQUALS(root->pe_first, typed_kv_PM)) {
    	TOID(struct key_val_pair_PM) typed_kv_PM_next = D_RO(typed_kv_PM)->pmem_list_next;
    	if(!TOID_IS_NULL(typed_kv_PM_next)){
    		struct key_val_pair_PM *next = D_RW(typed_kv_PM_next);
    		TX_ADD_FIELD_DIRECT(next,pmem_list_prev);
            makecheckpoint(&(next->pmem_list_prev));
                    //serverLog(LL_NOTICE,"ulog\n");
                    //printf("ulog\n");

    		next->pmem_list_prev.oid = OID_NULL;
    	}
    	TX_FREE(root->pe_first);
    	TX_ADD_DIRECT(root);
        makecheckpoint(root);
                //serverLog(LL_NOTICE,"ulog\n");
                //printf("ulog\n");

    	root->pe_first = typed_kv_PM_next;
        root->num_dict_entries--;
        return;
    }
    else {
    	TOID(struct key_val_pair_PM) typed_kv_PM_prev = D_RO(typed_kv_PM)->pmem_list_prev;
    	TOID(struct key_val_pair_PM) typed_kv_PM_next = D_RO(typed_kv_PM)->pmem_list_next;
    	if(!TOID_IS_NULL(typed_kv_PM_prev)){
    		struct key_val_pair_PM *prev = D_RW(typed_kv_PM_prev);
    		TX_ADD_FIELD_DIRECT(prev,pmem_list_next);
            makecheckpoint(&(prev->pmem_list_next));
                    //serverLog(LL_NOTICE,"ulog\n");
                    //printf("ulog\n");

    		prev->pmem_list_next = typed_kv_PM_next;
    	}
    	if(!TOID_IS_NULL(typed_kv_PM_next)){
    		struct key_val_pair_PM *next = D_RW(typed_kv_PM_next);
    		TX_ADD_FIELD_DIRECT(next,pmem_list_prev);
            makecheckpoint(&(next->pmem_list_prev));
                    //serverLog(LL_NOTICE,"ulog\n");
                    //printf("ulog\n");

    		next->pmem_list_prev = typed_kv_PM_prev;
    	}
    	TX_FREE(typed_kv_PM);
    	TX_ADD_FIELD_DIRECT(root,num_dict_entries);
        makecheckpoint(&(root->num_dict_entries));
                //serverLog(LL_NOTICE,"ulog\n");
                //printf("ulog\n");

        root->num_dict_entries--;
        return;
    }
}
#endif
