//
// Created by Libin Zhou on 11/27/22.
//

#ifndef CONCURRENT_HASH_MAP_TEST_PARALLEL_HASHMAP_BENCH_HPP
#define CONCURRENT_HASH_MAP_TEST_PARALLEL_HASHMAP_BENCH_HPP
#include "./Global.hpp"
#include <parallel_hashmap/phmap.h>
#include <vector>
#include <thread>
#include <random>
#include <iostream>

/*
 *note this microbenchmark is not perfect. It is hard to simulate the logic that a find request for a key will not happen after it is deleted but it should be rare.
 * We use this unperfect benchmark to test performance.
 * Node hash map is slower and more costly but it stores the object directly and we don't need to manage memory
 */
template<class Map>
class Parallel_Node_Hashmap_Bench{
public:
    static inline std::atomic_int64_t totalOp = 0;
    // static phmap::parallel_node_hash_map<uint64_t,Transaction_Table_Entry>txnTables[THREAD_NUM];

    static inline std::atomic_bool flag = false;
    // reader will do lazy updates
    Parallel_Node_Hashmap_Bench(){};
    static void reader(Map (&txnTables)[THREAD_NUM]){
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> threadDist(0,THREAD_NUM-1);
        std::uniform_int_distribution<std::mt19937::result_type> txnIDDist(0,(1ul<<LAZY_UPDATE_WORKLOAD_ORDER)-1);
        //Transaction_Table_Entry* entryPtr;
        while(!Parallel_Node_Hashmap_Bench::flag.load());
        auto start = std::chrono::high_resolution_clock::now();
        long opCount=0;
        while(true){
            int threadID = threadDist(rng);
            int local_txnID = txnIDDist(rng);
            uint64_t txnID = bwtransaction::generateTxnID(threadID,local_txnID);
            bool erase = false;
            txnTables[threadID].modify_if(txnID,[&erase](typename Map::value_type& pair){erase=(pair.second.op_count.fetch_sub(1)==1);});
            if(erase){
                txnTables[threadID].erase(txnID);
            }
            opCount++;
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            if(duration.count()>60000000){
                break;
            }
        }
        Parallel_Node_Hashmap_Bench::totalOp.fetch_add(opCount);
    }
    //writer will insert entries.
    //range: 2^24 to 2^50
    static void writer(uint8_t threadID,Map (&txnTables)[THREAD_NUM]){
        long op_count=0;
        while(!Parallel_Node_Hashmap_Bench::flag.load());
        auto start = std::chrono::high_resolution_clock::now();
        for(uint64_t i = 1ul<<LAZY_UPDATE_WORKLOAD_ORDER; i<=1ul<<50;i++){
            uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
            Transaction_Table_Entry entry;
            txnTables[threadID].emplace(txnID,entry);
            op_count++;
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            if(duration.count()>60000000){
                break;
            }
        }
        Parallel_Node_Hashmap_Bench::totalOp.fetch_add(op_count);
    }
    static void initializer(uint8_t threadID,Map (&txnTables)[THREAD_NUM]){
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist10(1,20);
        for(uint64_t i =0; i<= 1ul<<LAZY_UPDATE_WORKLOAD_ORDER;i++){
            uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
            Transaction_Table_Entry entry;
            entry.op_count = dist10(rng);
            txnTables[threadID].emplace(txnID,entry);
        }
    }

    void microbenchmark(){
        Map txnTables[THREAD_NUM];
        std::vector<std::thread> initializers;
        for (int i = 0; i < THREAD_NUM; i++) {
            initializers.push_back(std::thread(initializer, i, std::ref(txnTables)));
        }
        for (int i = 0; i < THREAD_NUM; i++) {
            initializers.at(i).join();
        }
        std::cout << "initialize done\n";
        std::vector<std::thread> readers;
        std::vector<std::thread> writers;
        for (int i = 0; i < THREAD_NUM; i++) {
            readers.push_back(std::thread(reader, std::ref(txnTables)));
            writers.push_back(std::thread(writer, i, std::ref(txnTables)));
        }
        Parallel_Node_Hashmap_Bench::flag.store(true);
        for (int i = 0; i < THREAD_NUM; i++) {
            readers.at(i).join();
            writers.at(i).join();
        }
        std::cout << "total op count is " << Parallel_Node_Hashmap_Bench::totalOp << "\n";
    }
};
//template<class Map>
class Parallel_Flat_Hashmap_Bench{
    using Map = phmap::parallel_flat_hash_map<
            uint64_t,
            Transaction_Table_Entry*,
            phmap::priv::hash_default_hash<uint64_t>,
            phmap::priv::hash_default_eq<uint64_t>,
            std::allocator<std::pair<const uint64_t, Transaction_Table_Entry*>>,
            12,
            std::mutex>;
public:
    static inline std::atomic_int64_t totalOp = 0;
    // static phmap::parallel_node_hash_map<uint64_t,Transaction_Table_Entry>txnTables[THREAD_NUM];
    static inline std::atomic_bool flag = false;

    Parallel_Flat_Hashmap_Bench(){};

    static void initializer(uint8_t threadID,Map (&txnTables)[THREAD_NUM]){
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist10(1,20); // distribution in range [1, 6]
        for(uint64_t i =0; i<= 1ul<<LAZY_UPDATE_WORKLOAD_ORDER;i++){
            uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
            Transaction_Table_Entry* entry = new Transaction_Table_Entry();
            entry->op_count = dist10(rng);
            txnTables[threadID].emplace(txnID,entry);
        }
    }

    static void reader(Map (&txnTables)[THREAD_NUM]){
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> threadDist(0,THREAD_NUM-1);
        std::uniform_int_distribution<std::mt19937::result_type> txnIDDist(0,(1ul<<LAZY_UPDATE_WORKLOAD_ORDER)-1);
        //Transaction_Table_Entry* entryPtr;
        while(!Parallel_Flat_Hashmap_Bench::flag.load());
        auto start = std::chrono::high_resolution_clock::now();
        long opCount=0;
        while(true){
            int threadID = threadDist(rng);
            int local_txnID = txnIDDist(rng);
            uint64_t txnID = bwtransaction::generateTxnID(threadID,local_txnID);
            bool erase = false;
            Transaction_Table_Entry* copy;
            txnTables[threadID].if_contains(txnID,[&erase,&copy](typename Map::value_type& pair){copy =pair.second;erase=(copy->op_count.fetch_sub(1)==1);});
            if(erase){
                txnTables[threadID].erase(txnID);
                delete copy;
            }
            opCount++;
            opCount++;
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            if(duration.count()>60000000){
                break;
            }
        }
        totalOp.fetch_add(opCount);
    }
    static void writer(uint8_t threadID,Map (&txnTables)[THREAD_NUM]){
        long op_count = 0;
        while (!Parallel_Flat_Hashmap_Bench::flag.load());
        auto start = std::chrono::high_resolution_clock::now();
        for(uint64_t i = 1ul<<LAZY_UPDATE_WORKLOAD_ORDER; i<=1ul<<50;i++){
            uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
            Transaction_Table_Entry* entry = new Transaction_Table_Entry();
            txnTables[threadID].emplace(txnID,entry);
            op_count++;
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            if(duration.count()>60000000){
                break;
            }
        }
    }
    void microbenchmark(){
        Map txnTables[THREAD_NUM];
        std::vector<std::thread> initializers;
        for (int i = 0; i < THREAD_NUM; i++) {
            initializers.push_back(std::thread(initializer, i, std::ref(txnTables)));
        }
        for (int i = 0; i < THREAD_NUM; i++) {
            initializers.at(i).join();
        }
        std::cout << "initialize done\n";
        std::vector<std::thread> readers;
        std::vector<std::thread> writers;
        for (int i = 0; i < THREAD_NUM; i++) {
            readers.push_back(std::thread(reader, std::ref(txnTables)));
            writers.push_back(std::thread(writer, i, std::ref(txnTables)));
        }
        Parallel_Flat_Hashmap_Bench::flag.store(true);
        for (int i = 0; i < THREAD_NUM; i++) {
            readers.at(i).join();
            writers.at(i).join();
        }
        std::cout << "total op count is " << Parallel_Flat_Hashmap_Bench::totalOp << "\n";
    }
};

#endif //CONCURRENT_HASH_MAP_TEST_PARALLEL_HASHMAP_BENCH_HPP
