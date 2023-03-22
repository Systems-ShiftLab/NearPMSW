#!/usr/bin/env bash
cd nearpm/logging/pmdk
make -j EXTRA_CFLAGS="-Wno-error"  
sudo make install
cd src/examples/libpmemobj/map/
sudo ./run.sh btree > btree
sudo ./run.sh rbtree > rbtree
sudo ./run.sh skiplist > skip
sudo ./run.sh hashmap_tx > hashmp
cd ../../../../../../../
cat nearpm/logging/pmdk/src/examples/libpmemobj/map/btree > lognearpmresult
cat nearpm/logging/pmdk/src/examples/libpmemobj/map/rbtree >> lognearpmresult
cat nearpm/logging/pmdk/src/examples/libpmemobj/map/skip >> lognearpmresult
cat nearpm/logging/pmdk/src/examples/libpmemobj/map/hashmp >> lognearpmresult
cd nearpm/logging/TATP_NDP/
sudo ./run.sh tatp > tatp
cd ../../../
cat nearpm/logging/TATP_NDP/tatp >> lognearpmresult
cd nearpm/logging/TPCC_NDP/
sudo ./run.sh tpcc > tpcc
cd ../../../
cat nearpm/logging/TPCC_NDP/tpcc >> lognearpmresult
cd nearpm/logging/memcached-pmem-NDP/
sudo ./run.sh memcached > memcachedr &
cd ../../../
cd baseline/logging/YCSB/
sleep 30
sudo ./runmem.sh  memcached > memcachedr
cd ../../../
cat baseline/logging/YCSB/memcachedr >> lognearpmresult
sleep 30
cat nearpm/logging/memcached-pmem-NDP/memcachedr >> lognearpmresult
cd nearpm/logging/redis/redis-NDP/
sudo ./run.sh redis > redis1 &
cd ../../../../
cd baseline/logging/YCSB/
sleep 60
sudo ./runredis.sh  redis > redis1
cd ../../../
cat baseline/logging/YCSB/redis1 >> lognearpmresult
sleep 30
cat nearpm/logging/redis/redis-NDP/redis1 >> lognearpmresult
cd nearpm/logging/pmemkv-bench/
sudo ./run.sh pmemkv > pmemkv1 &
cd ../../../
sleep 30
cat nearpm/logging/pmemkv-bench/pmemkv1 >> lognearpmresult







