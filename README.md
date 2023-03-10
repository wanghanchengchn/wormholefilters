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

