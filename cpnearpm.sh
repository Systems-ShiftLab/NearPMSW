#!/usr/bin/env bash
cd nearpm/checkpointing/pmdk-checkpoint1
make -j EXTRA_CFLAGS="-Wno-error" 
sudo make install
cd src/examples/libpmemobj/map/
sudo ./run.sh btree > btree
sudo ./run.sh rbtree > rbtree
sudo ./run.sh skiplist > skip
sudo ./run.sh hashmap_tx > hashmp
cd ../../../../../../../
cat nearpm/checkpointing/pmdk-checkpoint1/src/examples/libpmemobj/map/btree > cpnearpmresult
cat nearpm/checkpointing/pmdk-checkpoint1/src/examples/libpmemobj/map/rbtree >> cpnearpmresult
cat nearpm/checkpointing/pmdk-checkpoint1/src/examples/libpmemobj/map/skip >> cpnearpmresult
cat nearpm/checkpointing/pmdk-checkpoint1/src/examples/libpmemobj/map/hashmp >> cpnearpmresult
cd nearpm/checkpointing/TATP_CP/
sudo ./run.sh tatp > tatp
cd ../../../
cat nearpm/checkpointing/TATP_CP/tatp >> cpnearpmresult
cd nearpm/checkpointing/TPCC_CP/
sudo ./run.sh tpcc > tpcc
cd ../../../
cat nearpm/checkpointing/TPCC_CP/tpcc >> cpnearpmresult
cd nearpm/checkpointing/memcached-pmem-checkpointing/
sudo ./run.sh memcached > memcachedr &
cd ../../../
cd baseline/logging/YCSB/
sleep 30
sudo ./runmem.sh  memcached > memcachedr
cd ../../../
cat baseline/logging/YCSB/memcachedr >> cpnearpmresult
sleep 30
cat nearpm/checkpointing/memcached-pmem-checkpointing/memcachedr >> cpnearpmresult
cd nearpm/checkpointing/redis-NDP-chekpoint/
sudo ./run.sh redis > redis1 &
cd ../../../
cd baseline/logging/YCSB/
sleep 60
sudo ./runredis.sh  redis > redis1
cd ../../../
cat baseline/logging/YCSB/redis1 >> cpnearpmresult
sleep 30
cat nearpm/checkpointing/redis-NDP-chekpoint/redis1 >> cpnearpmresult
cd nearpm/checkpointing/pmemkv-bench-chekpointing/
sudo ./run.sh pmemkv > pmemkv1 &
cd ../../../
sleep 30
cat nearpm/checkpointing/pmemkv-bench-chekpointing/pmemkv1 >> cpnearpmresult

