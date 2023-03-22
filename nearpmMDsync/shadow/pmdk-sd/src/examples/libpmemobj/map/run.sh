#!/usr/bin/env bash
sudo rm -rf /mnt/mem/*
sudo ./data_store $1 /mnt/mem/map 10000 > out
tx=$(grep "TX" out)
tot=$(grep "tottime" out)
grep "cp" out > time
cp=$(awk '{sum+= $2;} END{print sum;}' time)
echo $1$tx
echo $1$tot
echo $1'cp' $cp


