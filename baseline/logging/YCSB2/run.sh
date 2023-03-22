./bin/ycsb run memcached -s -P workloads/w1         -p "memcached.hosts=127.0.0.1" > outputLoad.txt
./bin/ycsb run redis -s -P workloads/w1         -p "redis.host=127.0.0.1" -p "redis.port=6379" > outputLoad.txt
./bin/ycsb run redis -s -p measurement.histogram.verbose=true -P workloads/w1         -p "redis.host=127.0.0.1" -p "redis.port=6379" > outputLoad.txt
./bin/ycsb run memcached -s -p measurement.histogram.verbose=true -P workloads/w1         -p "memcached.hosts=127.0.0.1" > outputLoad.txt
grep INSERT outputLoad.txt > time
awk -F "," '{print $2}' time 

