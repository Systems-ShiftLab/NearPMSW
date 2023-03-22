#!/usr/bin/env bash
sudo rm -rf /mnt/mem/*
sudo ./tatp_nvm > out
tot=$(grep "tottime" out)
grep "ulog" out > time
ulog=$(awk '{sum+= $2;} END{print sum/20000000000000;}' time)
grep "meta" out > time
meta=$(awk '{sum+= $2;} END{print sum/20000000000000;}' time)

sumulog=$(echo $ulog $meta $clobber $redo $redoclob| awk '{print $1 + $2 }')
echo $1$tot
echo $1'log' $sumulog


