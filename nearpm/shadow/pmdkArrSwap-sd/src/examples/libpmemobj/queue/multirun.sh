#!/bin/bash
sudo rm -rf /mnt/mem/queue.pool
sudo pmempool create --layout="queue" obj myobjpool.set
sudo ./queue /mnt/mem/queue.pool new 10000
#for (( c=1; c<=10000; c++ ))
#do 
#echo "$c"	
   sudo ./queue /mnt/mem/queue.pool enqueue hello
#done
