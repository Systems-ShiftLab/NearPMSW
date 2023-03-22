#!/usr/bin/env bash
cd baseline/checkpointing/pmdk-checkpoint1
make -j EXTRA_CFLAGS="-Wno-error" 
sudo make install
cd src/examples/libpmemobj/map/
sudo ./run.sh btree > btree
sudo ./run.sh rbtree > rbtree
sudo ./run.sh skiplist > skip
sudo ./run.sh hashmap_tx > hashmp
cd ../../../../../../../
cat baseline/checkpointing/pmdk-checkpoint1/src/examples/libpmemobj/map/btree > cpbaseresult
cat baseline/checkpointing/pmdk-checkpoint1/src/examples/libpmemobj/map/rbtree >> cpbaseresult
cat baseline/checkpointing/pmdk-checkpoint1/src/examples/libpmemobj/map/skip >> cpbaseresult
cat baseline/checkpointing/pmdk-checkpoint1/src/examples/libpmemobj/map/hashmp >> cpbaseresult
cd baseline/checkpointing/TATP_CP/
sudo ./run.sh tatp > tatp
cd ../../../
cat baseline/checkpointing/TATP_CP/tatp >> cpbaseresult
cd baseline/checkpointing/TPCC_CP/
sudo ./run.sh tpcc > tpcc
cd ../../../
cat baseline/checkpointing/TPCC_CP/tpcc >> cpbaseresult
cd baseline/checkpointing/memcached-pmem-checkpointing/
sudo ./run.sh memcached > memcachedr &
cd ../../../
cd baseline/logging/YCSB/
sleep 30 
sudo ./runmem.sh  memcached > memcachedr
cd ../../../
cat baseline/logging/YCSB/memcachedr >> cpbaseresult
sleep 30
cat baseline/checkpointing/memcached-pmem-checkpointing/memcachedr >> cpbaseresult
cd baseline/checkpointing/redis-NDP-chekpoint/
sudo ./run.sh redis > redis1 &
cd ../../../
cd baseline/logging/YCSB/
sleep 60
sudo ./runredis.sh  redis > redis1
cd ../../../
cat baseline/logging/YCSB/redis1 >> cpbaseresult
sleep 30
cat baseline/checkpointing/redis-NDP-chekpoint/redis1 >> cpbaseresult
cd baseline/checkpointing/pmemkv-bench-chekpointing/
sudo ./run.sh pmemkv > pmemkv1 &
cd ../../../
sleep 30
cat baseline/checkpointing/pmemkv-bench-chekpointing/pmemkv1 >> cpbaseresult

