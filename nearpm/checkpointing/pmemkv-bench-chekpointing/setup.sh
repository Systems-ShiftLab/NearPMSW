sudo mkfs.ext4 /dev/pmem0 
sudo mount -o dax /dev/pmem0 /mnt/pmem
sudo chown oem /mnt/pmem
#cd Research/Research/pmdk/src/examples/libpmemobj/map/
#cat run.sh
