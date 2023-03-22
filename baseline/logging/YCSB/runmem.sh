#!/usr/bin/env bash
./bin/ycsb run memcached -s -P workloads/w1         -p "memcached.hosts=127.0.0.1" > out
pid=$(pidof memcached)
sudo kill $pid
tot=$(grep RunTime out| awk '{print $NF/1000}')
echo $1'tottime ' $tot
