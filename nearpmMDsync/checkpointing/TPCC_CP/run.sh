#!/usr/bin/env bash
sudo rm -rf /mnt/mem/*
sudo ./tpcc_nvm > out
tot=$(grep "tottime" out)
grep "cp" out > time
cp=$(awk '{sum+= $2;} END{print sum;}' time)
echo $1$tot
echo $1'cp' $cp


