#!/usr/bin/env bash
sudo rm -rf /mnt/mem/*
sed -i "s/Werror/Wno-error/g" Makefile
make -j USE_PMDK=yes STD=-std=gnu99
sudo ./memcached -u root -m 0 -t 1 -o pslab_policy=pmem,pslab_file=/mnt/mem/pool,pslab_force > out
grep "cp" out > time
log=$(awk '{sum+= $2;} END{print sum;}' time)
echo $1'cp' $log
