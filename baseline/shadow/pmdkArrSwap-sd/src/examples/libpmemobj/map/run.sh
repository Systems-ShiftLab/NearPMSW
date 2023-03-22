rm -rf /mnt/mem/*
sudo pmempool create --layout="data_store" obj myobjpool.set
sudo ./data_store /mnt/mem/data_store.pool
awk '{sum+= $3;} END{print sum;}' time
grep "time" out > time
grep "timecp" out > time
