#!/usr/bin/env bash
while read p; do
	sudo chmod +x "$p"
done < shfiles
cd baseline/logging/pmdk
make -j EXTRA_CFLAGS="-Wno-error" 
sudo make install
cd src/examples/libpmemobj/map/
sudo ./run.sh btree > btree
sudo ./run.sh rbtree > rbtree
sudo ./run.sh skiplist > skip
sudo ./run.sh hashmap_tx > hashmp
cd ../../../../../../../
cat baseline/logging/pmdk/src/examples/libpmemobj/map/btree > logbaseresult
cat baseline/logging/pmdk/src/examples/libpmemobj/map/rbtree >> logbaseresult
cat baseline/logging/pmdk/src/examples/libpmemobj/map/skip >> logbaseresult
cat baseline/logging/pmdk/src/examples/libpmemobj/map/hashmp >> logbaseresult
cd baseline/logging/TATP/
sudo ./run.sh tatp > tatp
cd ../../../
cat baseline/logging/TATP/tatp >> logbaseresult
cd baseline/logging/TPCC/
sudo ./run.sh tpcc > tpcc
cd ../../../
cat baseline/logging/TPCC/tpcc >> logbaseresult
cd baseline/logging/memcached/
sudo ./run.sh memcached > memcachedr &
cd ../YCSB/
sleep 30
sudo ./runmem.sh  memcached > memcachedr
cd ../../../
cat baseline/logging/YCSB/memcachedr >> logbaseresult
sleep 30
cat baseline/logging/memcached/memcachedr >> logbaseresult
cd baseline/logging/redis-NDP1-golden/
sudo ./run.sh redis > redis1 &
cd ../YCSB/
sleep 60
sudo ./runredis.sh  redis > redis1
cd ../../../
cat baseline/logging/YCSB/redis1 >> logbaseresult
sleep 30
cat baseline/logging/redis-NDP1-golden/redis1 >> logbaseresult
cd baseline/logging/pmemkv-bench/
sudo ./run.sh pmemkv > pmemkv1 &
cd ../../../
sleep 30
cat baseline/logging/pmemkv-bench/pmemkv1 >> logbaseresult

