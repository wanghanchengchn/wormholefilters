# wormholefilters

We propose a novel approximate membership query data structure called wormhole filter, which exhibits high performance on persistent memory.


## Dependencies

### For building

#### Required

* `libpmemobj` in PMDK

## Build and run

```
mkdir build
cd build
cmake ..
cd test
make
./example
```

## To generate YCSB workloads
```sh
cd YCSB
wget https://github.com/brianfrankcooper/YCSB/releases/download/0.17.0/ycsb-0.17.0.tar.gz
tar -xvf ycsb-0.17.0.tar.gz
mv ycsb-0.17.0 YCSB
#Then run workload generator
mkdir workloads
./generate_all_workloads.sh
```
