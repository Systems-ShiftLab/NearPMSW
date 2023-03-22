#!/usr/bin/env bash
cd nearpm/shadow/pmdk-sd
make -j EXTRA_CFLAGS="-Wno-error" 
sudo make install
cd src/examples/libpmemobj/map/
sudo ./run.sh btree > btree
sudo ./run.sh rbtree > rbtree
sudo ./run.sh skiplist > skip
sudo ./run.sh hashmap_tx > hashmp
cd ../../../../../../../
cat nearpm/shadow/pmdk-sd/src/examples/libpmemobj/map/btree > shadownearpmresult
cat nearpm/shadow/pmdk-sd/src/examples/libpmemobj/map/rbtree >> shadownearpmresult
cat nearpm/shadow/pmdk-sd/src/examples/libpmemobj/map/skip >> shadownearpmresult
cat nearpm/shadow/pmdk-sd/src/examples/libpmemobj/map/hashmp >> shadownearpmresult
cd nearpm/shadow/TATP_SD/
sudo ./run.sh tatp > tatp
cd ../../../
cat nearpm/shadow/TATP_SD/tatp >> shadownearpmresult
cd nearpm/shadow/TPCC_SD/
sudo ./run.sh tpcc > tpcc
cd ../../../
cat nearpm/shadow/TPCC_SD/tpcc >> shadownearpmresult
cd nearpm/shadow/memcached-pmem-sd/
sudo ./run.sh memcached > memcachedr &
cd ../../../
cd baseline/logging/YCSB/
sleep 30 
sudo ./runmem.sh  memcached > memcachedr
cd ../../../
cat baseline/logging/YCSB/memcachedr >> shadownearpmresult
sleep 30
cat nearpm/shadow/memcached-pmem-sd/memcachedr >> shadownearpmresult
cd nearpm/shadow/redis-NDP-sd/
sudo ./run.sh redis > redis1 &
cd ../../../
cd baseline/logging/YCSB/
sleep 60
sudo ./runredis.sh  redis > redis1
cd ../../../
cat baseline/logging/YCSB/redis1 >> shadownearpmresult
sleep 30
cat nearpm/shadow/redis-NDP-sd/redis1 >> shadownearpmresult
cd nearpm/shadow/pmemkv-bench-sd/
sudo ./run.sh pmemkv > pmemkv1 &
cd ../../../
sleep 30
cat nearpm/shadow/pmemkv-bench-sd/pmemkv1 >> shadownearpmresult
sed -i "s/cp/shadow/g" shadownearpmresult
