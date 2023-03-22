#!/usr/bin/env bash
cd nearpmMDsync/checkpointing/pmdk-checkpoint1
make -j EXTRA_CFLAGS="-Wno-error" 
sudo make install
cd src/examples/libpmemobj/map/
sudo ./run.sh btree > btree
sudo ./run.sh rbtree > rbtree
sudo ./run.sh skiplist > skip
sudo ./run.sh hashmap_tx > hashmp
cd ../../../../../../../
cat nearpmMDsync/checkpointing/pmdk-checkpoint1/src/examples/libpmemobj/map/btree > cpnearpmsyncresult
cat nearpmMDsync/checkpointing/pmdk-checkpoint1/src/examples/libpmemobj/map/rbtree >> cpnearpmsyncresult
cat nearpmMDsync/checkpointing/pmdk-checkpoint1/src/examples/libpmemobj/map/skip >> cpnearpmsyncresult
cat nearpmMDsync/checkpointing/pmdk-checkpoint1/src/examples/libpmemobj/map/hashmp >> cpnearpmsyncresult
cd nearpmMDsync/checkpointing/TATP_CP/
sudo ./run.sh tatp > tatp
cd ../../../
cat nearpmMDsync/checkpointing/TATP_CP/tatp >> cpnearpmsyncresult
cd nearpmMDsync/checkpointing/TPCC_CP/
sudo ./run.sh tpcc > tpcc
cd ../../../
cat nearpmMDsync/checkpointing/TPCC_CP/tpcc >> cpnearpmsyncresult
cd nearpmMDsync/checkpointing/memcached-pmem-checkpointing/
sudo ./run.sh memcached > memcachedr &
cd ../../../
cd baseline/logging/YCSB/
sleep 30
sudo ./runmem.sh  memcached > memcachedr
cd ../../../
cat baseline/logging/YCSB/memcachedr >> cpnearpmsyncresult
sleep 30
cat nearpmMDsync/checkpointing/memcached-pmem-checkpointing/memcachedr >> cpnearpmsyncresult
cd nearpmMDsync/checkpointing/redis-NDP-chekpoint/
sudo ./run.sh redis > redis1 &
cd ../../../
cd baseline/logging/YCSB/
sleep 60
sudo ./runredis.sh  redis > redis1
cd ../../../
cat baseline/logging/YCSB/redis1 >> cpnearpmsyncresult
sleep 30
cat nearpmMDsync/checkpointing/redis-NDP-chekpoint/redis1 >> cpnearpmsyncresult
cd nearpmMDsync/checkpointing/pmemkv-bench-chekpointing/
sudo ./run.sh pmemkv > pmemkv1 &
cd ../../../
sleep 30
cat nearpmMDsync/checkpointing/pmemkv-bench-chekpointing/pmemkv1 >> cpnearpmsyncresult

