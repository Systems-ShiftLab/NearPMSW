#!/usr/bin/env bash
#./bin/ycsb run redis -s -P workloads/w1         -p "redis.host=127.0.0.1" -p "redis.port=6379" 
./bin/ycsb run redis -s -P workloads/w1         -p "redis.host=127.0.0.1" -p "redis.port=6379" > out
pid=$(pidof redis-server)
sudo kill $pid
tot=$(grep RunTime out| awk '{print $NF/1000}')
echo $1'tottime ' $tot
