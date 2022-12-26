//
// Created by Libin Zhou on 12/6/22.
//

#ifndef CONCURRENT_HASH_MAP_TEST_TBBHASHMAP_HPP
#define CONCURRENT_HASH_MAP_TEST_TBBHASHMAP_HPP
#include "../Global.hpp"
#include <random>
#include <thread>
#include <iostream>
#include "oneapi/tbb/concurrent_hash_map.h"
class TBB_Hashmap_Benchmark{
    using Map = tbb::concurrent_hash_map<uint64_t,Transaction_Table_Entry>;
    using entryAccessor = tbb::concurrent_hash_map<uint64_t,Transaction_Table_Entry>::accessor;
    using const_entryAccessor = tbb::concurrent_hash_map<uint64_t,Transaction_Table_Entry>::const_accessor;
public:
    static inline std::atomic_int64_t totalOp = 0;
    static inline std::atomic_bool flag = false;
    static inline std::atomic_int64_t total_erase = 0;
    static inline std::atomic_int64_t total_read=0;
    static inline std::atomic_int64_t total_insert=0;
    TBB_Hashmap_Benchmark(){};
    static void initializer(uint8_t threadID,Map* (txnTables)[THREAD_NUM]){
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist10(1,100);
        for(uint64_t i =0; i< 1ul<<LAZY_UPDATE_WORKLOAD_ORDER;i++){
            uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
            Transaction_Table_Entry entry;
            entry.op_count = dist10(rng);
            txnTables[threadID]->emplace(txnID,entry);
        }
    }
    static void writer(uint8_t threadID,Map* (txnTables)[THREAD_NUM]){
        //long opCount=0;
        int64_t local_insert=0;
        while(!TBB_Hashmap_Benchmark::flag.load());
        auto start = std::chrono::high_resolution_clock::now();
        for(uint64_t i = 1ul<<LAZY_UPDATE_WORKLOAD_ORDER; i<=1ul<<50;i++){
            uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
            Transaction_Table_Entry entry;
            txnTables[threadID]->emplace(txnID,entry);
            local_insert++;
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            if(duration.count()>60000000){
                break;
            }
        }
        total_insert.fetch_add(local_insert);
    }

    static void reader(Map* (txnTables)[THREAD_NUM]){
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> threadDist(0,THREAD_NUM-1);
        std::uniform_int_distribution<std::mt19937::result_type> txnIDDist(0,(1ul<<LAZY_UPDATE_WORKLOAD_ORDER)-1);
        int64_t local_delete=0;
        int64_t local_read=0;
        //Transaction_Table_Entry* entryPtr;
        while(!TBB_Hashmap_Benchmark::flag.load());
        auto start = std::chrono::high_resolution_clock::now();
        //long opCount=0;
        while(true){
            int threadID = threadDist(rng);
            int local_txnID = txnIDDist(rng);
            uint64_t txnID = bwtransaction::generateTxnID(threadID,local_txnID);
            entryAccessor accessor;
            const auto isFound = txnTables[threadID]->find(accessor,txnID);
            if(isFound){
                if(accessor->second.op_count.fetch_sub(1)==1){
                    local_delete++;
                    txnTables[threadID]->erase(accessor);
                }
            }
            local_read++;
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            if(duration.count()>60000000){
                break;
            }
        }
        total_erase.fetch_add(local_delete);
        total_read.fetch_add(local_read);
    }

    void michrobenchmark(){
        Map* txnTables[THREAD_NUM];
        for(int i=0;i<THREAD_NUM;i++){
            txnTables[i] = new Map();
        }
        std::vector<std::thread> initializers;
        for (int i = 0; i < THREAD_NUM; i++) {
            initializers.push_back(std::thread(initializer, i, txnTables));
        }
        for (int i = 0; i < THREAD_NUM; i++) {
            initializers.at(i).join();
        }
        //std::cout << "initialize done\n";
        std::vector<std::thread> readers;
        std::vector<std::thread> writers;
        for (int i = 0; i < THREAD_NUM; i++) {
            readers.push_back(std::thread(reader, txnTables));
            writers.push_back(std::thread(writer, i, txnTables));
        }
        TBB_Hashmap_Benchmark::flag.store(true);
        for (int i = 0; i < THREAD_NUM; i++) {
            readers.at(i).join();
            writers.at(i).join();
        }
        for(int i=0;i<THREAD_NUM;i++){
            delete txnTables[i];
        }
        totalOp.fetch_add(total_erase);
        totalOp.fetch_add(total_read);
        totalOp.fetch_add(total_insert);
        std::cout << "total TBB Hashmap op count is " << totalOp << "\n";
        std::cout<<"total TBB Hashmap delete count is "<<total_erase<<"\n";
        std::cout<<"total TBB Hashmap insert count is "<<total_insert<<"\n";
        std::cout<<"total TBB Hashmap read count is "<<total_read<<"\n\n";
    }
};
#endif //CONCURRENT_HASH_MAP_TEST_TBBHASHMAP_HPP
