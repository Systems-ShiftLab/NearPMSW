#!/usr/bin/env bash
cd nearpmMDsync/shadow/pmdk-sd
make -j EXTRA_CFLAGS="-Wno-error" 
sudo make install
cd src/examples/libpmemobj/map/
sudo ./run.sh btree > btree
sudo ./run.sh rbtree > rbtree
sudo ./run.sh skiplist > skip
sudo ./run.sh hashmap_tx > hashmp
cd ../../../../../../../
cat nearpmMDsync/shadow/pmdk-sd/src/examples/libpmemobj/map/btree > shadownearpmsyncresult
cat nearpmMDsync/shadow/pmdk-sd/src/examples/libpmemobj/map/rbtree >> shadownearpmsyncresult
cat nearpmMDsync/shadow/pmdk-sd/src/examples/libpmemobj/map/skip >> shadownearpmsyncresult
cat nearpmMDsync/shadow/pmdk-sd/src/examples/libpmemobj/map/hashmp >> shadownearpmsyncresult
cd nearpmMDsync/shadow/TATP_SD/
sudo ./run.sh tatp > tatp
cd ../../../
cat nearpmMDsync/shadow/TATP_SD/tatp >> shadownearpmsyncresult
cd nearpmMDsync/shadow/TPCC_SD/
sudo ./run.sh tpcc > tpcc
cd ../../../
cat nearpmMDsync/shadow/TPCC_SD/tpcc >> shadownearpmsyncresult
cd nearpmMDsync/shadow/memcached-pmem-sd/
sudo ./run.sh memcached > memcachedr &
cd ../../../
cd baseline/logging/YCSB/
sleep 30 
sudo ./runmem.sh  memcached > memcachedr
cd ../../../
cat baseline/logging/YCSB/memcachedr >> shadownearpmsyncresult
sleep 30
cat nearpmMDsync/shadow/memcached-pmem-sd/memcachedr >> shadownearpmsyncresult
cd nearpmMDsync/shadow/redis-NDP-sd/
sudo ./run.sh redis > redis1 &
cd ../../../
cd baseline/logging/YCSB/
sleep 60
sudo ./runredis.sh  redis > redis1
cd ../../../
cat baseline/logging/YCSB/redis1 >> shadownearpmsyncresult
sleep 30
cat nearpmMDsync/shadow/redis-NDP-sd/redis1 >> shadownearpmsyncresult
cd nearpmMDsync/shadow/pmemkv-bench-sd/
sudo ./run.sh pmemkv > pmemkv1 &
cd ../../../
sleep 30
cat nearpmMDsync/shadow/pmemkv-bench-sd/pmemkv1 >> shadownearpmsyncresult
sed -i "s/cp/shadow/g" shadownearpmsyncresult
