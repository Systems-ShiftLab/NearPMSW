sudo rm -rf /mnt/mem/fifo.pool
sudo pmempool create --layout="list" obj myobjpool.set
sudo ../../../tools/pmempool/pmempool create obj /mnt/mem/fifo.pool --layout list
sudo ./fifo /mnt/mem/fifo.pool insert a
sudo ./fifo /mnt/mem/fifo.pool remove a

