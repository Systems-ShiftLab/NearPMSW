sudo mount -o dax /dev/pmem0 /mnt/pmem
sudo rm -rf /mnt/mem/*
sudo chown oem /mnt/pmem
#./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1
#sudo ./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1 --num=10000  --engine=cmap --benchmarks=fillseq,fillrandom,overwrite > out
make bench
sudo ./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1 --num=10000  --engine=cmap --benchmarks=fillrandom > out
#grep "timecp" out > time
#awk '{sum+= $3;} END{print sum;}' time
grep "cp" out > time
log=$(awk '{sum+= $2;} END{print sum;}' time)
echo ""
echo $1'cp' $log
tottime=$(tail -1  out | awk '{print $NF}' | cut -d "," -f10|awk '{print $1/100}')
echo $1'tottime' $tottime
