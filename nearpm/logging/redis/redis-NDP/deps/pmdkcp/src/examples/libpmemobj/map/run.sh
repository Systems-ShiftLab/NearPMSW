sudo rm -rf /mnt/mem/queue.pool
sudo pmempool create --layout="queue" obj myobjpool.set
#sudo ../../../tools/pmempool/pmempool create obj /mnt/mem/queue.pool --layout queue
sudo ./queue /mnt/mem/queue.pool new 20
sudo ./queue /mnt/mem/queue.pool enqueue hello
sudo ./queue /mnt/mem/queue.pool show
sudo rm -f /mnt/mem/map
sudo ./mapcli hashmap_tx /mnt/mem/map
sudo ./mapcli ctree /mnt/mem/map
sudo ./data_store btree /mnt/mem/map 10000

