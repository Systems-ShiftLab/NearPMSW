#!/usr/bin/env bash
cd nearpmMDsync/logging/pmdk
make -j EXTRA_CFLAGS="-Wno-error"  
sudo make install
cd src/examples/libpmemobj/map/
sudo ./run.sh btree > btree
sudo ./run.sh rbtree > rbtree
sudo ./run.sh skiplist > skip
sudo ./run.sh hashmap_tx > hashmp
cd ../../../../../../../
cat nearpmMDsync/logging/pmdk/src/examples/libpmemobj/map/btree > lognearpmsyncresult
cat nearpmMDsync/logging/pmdk/src/examples/libpmemobj/map/rbtree >> lognearpmsyncresult
cat nearpmMDsync/logging/pmdk/src/examples/libpmemobj/map/skip >> lognearpmsyncresult
cat nearpmMDsync/logging/pmdk/src/examples/libpmemobj/map/hashmp >> lognearpmsyncresult
cd nearpmMDsync/logging/TATP_NDP/
sudo ./run.sh tatp > tatp
cd ../../../
cat nearpmMDsync/logging/TATP_NDP/tatp >> lognearpmsyncresult
cd nearpmMDsync/logging/TPCC_NDP/
sudo ./run.sh tpcc > tpcc
cd ../../../
cat nearpmMDsync/logging/TPCC_NDP/tpcc >> lognearpmsyncresult
cd nearpmMDsync/logging/memcached-pmem-NDP/
sudo ./run.sh memcached > memcachedr &
cd ../../../
cd baseline/logging/YCSB/
sleep 30
sudo ./runmem.sh  memcached > memcachedr
cd ../../../
cat baseline/logging/YCSB/memcachedr >> lognearpmsyncresult
sleep 30
cat nearpmMDsync/logging/memcached-pmem-NDP/memcachedr >> lognearpmsyncresult
cd nearpmMDsync/logging/redis/redis-NDP/
sudo ./run.sh redis > redis1 &
cd ../../../../
cd baseline/logging/YCSB/
sleep 60
sudo ./runredis.sh  redis > redis1
cd ../../../
cat baseline/logging/YCSB/redis1 >> lognearpmsyncresult
sleep 30
cat nearpmMDsync/logging/redis/redis-NDP/redis1 >> lognearpmsyncresult
cd nearpmMDsync/logging/pmemkv-bench/
sudo ./run.sh pmemkv > pmemkv1 &
cd ../../../
sleep 30
cat nearpmMDsync/logging/pmemkv-bench/pmemkv1 >> lognearpmsyncresult







