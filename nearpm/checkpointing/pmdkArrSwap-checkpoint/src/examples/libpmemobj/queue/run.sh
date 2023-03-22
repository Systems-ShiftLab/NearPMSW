sudo rm -rf /mnt/mem/queue.pool
sudo pmempool create --layout="queue" obj myobjpool.set
#sudo ../../../tools/pmempool/pmempool create obj /mnt/mem/queue.pool --layout queue
sudo ./queue /mnt/mem/queue.pool new 10000
sudo ./queue /mnt/mem/queue.pool enqueue hello>enqueue
sudo ./queue /mnt/mem/queue.pool show
grep tx enqueue | awk '{print $3}'>file
grep tx dequeue | awk '{print $3}'>file
make -j12  EXTRA_CFLAGS+=-DGET_NDP_PERFORMENCE  EXTRA_CFLAGS+=-DRUN_COUNT=10000 EXTRA_CFLAGS="-Wno-error"


