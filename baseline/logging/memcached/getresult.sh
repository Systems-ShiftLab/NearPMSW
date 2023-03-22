#!/usr/bin/env bash
pid=$(pidof memcached)
sudo kill $pid
grep "ulog" out > time
log=$(awk '{sum+= $2;} END{print sum;}' time)
echo $1'log' $log
