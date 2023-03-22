sudo mount -o dax /dev/pmem0 /mnt/pmem
sudo chown oem /mnt/pmem
./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1
