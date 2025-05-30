# wormholefilters

We propose a novel approximate membership query data structure called wormhole filter, which exhibits high performance on persistent memory.


## Dependencies

* `libpmemobj` in PMDK
* `OpenSSL`


## Build and run

```sh
mkdir build
cd build
cmake ..
cd test
make
./evaluation
```


## Evaluation

|Algorithm| Description|
|:----:|----|
|Bloom Filters (BF)|Burton H. Bloom. 1970. Space/Time Trade-offs in Hash Coding with Allowable Errors. Commun. ACM 13, 7 (1970), 422–426. Implementation: https://github.com/ArashPartow/bloom|
|Cuckoo Filters (CF)|Bin Fan, David G. Andersen, Michael Kaminsky, and Michael Mitzenmacher. 2014. Cuckoo Filter: Practically Better Than Bloom. In Proceedings of ACM International Conference on Emerging Networking Experiments and Technologies. ACM, 75–88. Implementation: https://github.com/efficient/cuckoofilter|
|Quotient Filters (QF)|Michael A. Bender, Martin Farach-Colton, Rob Johnson, Russell Kraner, Bradley C.Kuszmaul, Dzejla Medjedovic, Pablo Montes, Pradeep Shetty, Richard P. Spillane, and Erez Zadok. 2012. Don’t Thrash: How to Cache Your Hash on Flash. Proceedings of the VLDB Endowment 5, 11 (2012), 1627–1637. Implementation: https://github.com/vedantk/quotient-filter|
|Counting Quotient Filters (CQF)|Prashant Pandey, Michael A. Bender, Rob Johnson, and Rob Patro. 2017. A General-Purpose Counting Filter: Making Every Bit Count. In Proceedings of International Conference on Management of Data. ACM, 775–787. Implementation: https://github.com/splatlab/cqf|
|One Hash Blocked Bloom Filters (OHBBF)|Elakkiya Prakasam and Arun Manoharan. 2022. A Cache Efficient One Hashing Blocked Bloom Filter (OHBB) for Random Strings and the K-mer Strings in DNA Sequence. Symmetry 14, 9 (2022), 1–24.|
|Tagged Cuckoo Filters (TCF)|Kun Huang and Tong Yang. 2021. Tagged Cuckoo Filters. In Proceedings of International Conference on Computer Communications and Networks. IEEE, 1–10.|
|Vector Quotient Filters (VQF)|Prashant Pandey, Alex Conway, Joe Durie, Michael A. Bender, Martin Farach-Colton, and Rob Johnson. 2021. Vector Quotient Filters: Overcoming the Time/Space Trade-Off in Filter Design. In Proceedings of International Conference on Management of Data. ACM, 1386–1399. Implementation: https://github.com/splatlab/vqf|
|Prefix Filters (PF)|Tomer Even, Guy Even, and Adam Morrison. 2022. Prefix Filter: Practically and Theoretically Better Than Bloom. Proceedings of the VLDB Endowment 15, 7 (2022), 1311–1323. Implementation: https://github.com/TomerEven/Prefix-Filter|

### Leveldb

For the experiment of integrating the Wormhole Filter into LevelDB, we implemented the code in a manner similar to the Bloom Filter `util/bloom.cc` and used the libpmemobj library to allocate the memory space for the Wormhole Filter on PMEM.  
To more accurately measure the impact of replacing the filter on LevelDB's read performance, we disabled LevelDB's built-in compression and block cache, and implemented direct I/O to eliminate the impact of the file system page cache.  
Using LevelDB's db_bench `benchmarks/db_bench.cc`, we first inserted 10 million elements and then queried 10 million non-existent elements to measure the read performance under different filter configurations.

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

