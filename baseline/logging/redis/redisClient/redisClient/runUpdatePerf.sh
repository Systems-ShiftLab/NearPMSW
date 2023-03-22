#!/bin/bash
for i in 0 1 ; do
for j in 1.0 ; do
./redisClient 100000 $j 1000 $i 51000 0 ;
done
done
