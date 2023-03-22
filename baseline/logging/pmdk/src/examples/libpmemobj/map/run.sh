#!/usr/bin/env bash
sudo rm -rf /mnt/mem/*
sudo ./data_store $1 /mnt/mem/map 10000 > out
tx=$(grep "TX" out)
tot=$(grep "tottime" out)
grep "ulog" out > time
ulog=$(awk '{sum+= $2;} END{print sum/10000000000000;}' time)
grep "meta" out > time
meta=$(awk '{sum+= $2;} END{print sum/10000000000000;}' time)
grep "clobber" out > time
clobber=$(awk '{sum+= $2;} END{print sum/10000000000000;}' time)
grep "redo" out > time
redo=$(awk '{sum+= $2;} END{print sum/10000000000000;}' time)
grep "redoclob" out > time
redoclob=$(awk '{sum+= $2;} END{print sum/10000000000000;}' time)
sumulog=$(echo $ulog $meta $clobber $redo $redoclob| awk '{print $1 + $2 + $3 + $4 + $5}')
echo $1$tx
echo $1$tot
echo $1'log' $sumulog


