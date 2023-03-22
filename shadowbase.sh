#!/usr/bin/env bash
cd baseline/shadow/pmdk-sd
make -j EXTRA_CFLAGS="-Wno-error" 
sudo make install
cd src/examples/libpmemobj/map/
sudo ./run.sh btree > btree
sudo ./run.sh rbtree > rbtree
sudo ./run.sh skiplist > skip
sudo ./run.sh hashmap_tx > hashmp
cd ../../../../../../../
cat baseline/shadow/pmdk-sd/src/examples/libpmemobj/map/btree > shadowbaseresult
cat baseline/shadow/pmdk-sd/src/examples/libpmemobj/map/rbtree >> shadowbaseresult
cat baseline/shadow/pmdk-sd/src/examples/libpmemobj/map/skip >> shadowbaseresult
cat baseline/shadow/pmdk-sd/src/examples/libpmemobj/map/hashmp >> shadowbaseresult
cd baseline/shadow/TATP_SD/
sudo ./run.sh tatp > tatp
cd ../../../
cat baseline/shadow/TATP_SD/tatp >> shadowbaseresult
cd baseline/shadow/TPCC_SD/
sudo ./run.sh tpcc > tpcc
cd ../../../
cat baseline/shadow/TPCC_SD/tpcc >> shadowbaseresult
cd baseline/shadow/memcached-pmem-sd/
sudo ./run.sh memcached > memcachedr &
cd ../../../
cd baseline/logging/YCSB/
sleep 30 
sudo ./runmem.sh  memcached > memcachedr
cd ../../../
cat baseline/logging/YCSB/memcachedr >> shadowbaseresult
sleep 30
cat baseline/shadow/memcached-pmem-sd/memcachedr >> shadowbaseresult
cd baseline/shadow/redis-NDP-sd/
sudo ./run.sh redis > redis1 &
cd ../../../
cd baseline/logging/YCSB/
sleep 60
sudo ./runredis.sh  redis > redis1
cd ../../../
cat baseline/logging/YCSB/redis1 >> shadowbaseresult
sleep 30
cat baseline/shadow/redis-NDP-sd/redis1 >> shadowbaseresult
cd baseline/shadow/pmemkv-bench-sd/
sudo ./run.sh pmemkv > pmemkv1 &
cd ../../../
sleep 30
cat baseline/shadow/pmemkv-bench-sd/pmemkv1 >> shadowbaseresult
sed -i "s/cp/shadow/g" shadowbaseresult 

