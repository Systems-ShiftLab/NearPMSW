sudo ./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1 --engine=tree3 --benchmarks=fillseq
sudo ./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1 --engine=tree3 --benchmarks=fillrandom
sudo ./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1 --engine=tree3 --benchmarks=fillseq,overwrite
sudo ./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1 --engine=tree3 --benchmarks=fillseq,readseq
sudo ./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1 --engine=tree3 --benchmarks=fillseq,readrandom
sudo ./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1 --engine=tree3 --benchmarks=fillseq,readmissing
sudo ./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1 --engine=tree3 --benchmarks=fillseq,deleteseq
sudo ./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1 --engine=tree3 --benchmarks=fillseq,deleterandom
sudo ./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1 --engine=tree3 --benchmarks=fillseq,readwhilewriting
sudo ./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1 --engine=tree3 --benchmarks=fillseq,readrandomwriterandom
sudo ./pmemkv_bench --db=/mnt/pmem/pmemkv --db_size_in_gb=1 --engine=tree3 --benchmarks=fillseq,txfillrandom

