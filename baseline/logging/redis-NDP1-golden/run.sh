#!/usr/bin/env bash
sudo rm -rf /mnt/mem/*
make -j USE_PMDK=yes STD=-std=gnu99
sudo ./src/redis-server redis.conf > out
grep "ulog" out > time
log=$(awk '{sum+= $2;} END{print sum;}' time)
echo $1'log' $log
