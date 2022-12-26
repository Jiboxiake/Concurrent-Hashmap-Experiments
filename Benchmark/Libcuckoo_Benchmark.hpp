//
// Created by zhou822 on 12/9/22.
//

#ifndef CONCURRENT_HASH_MAP_TEST_LIBCUCKOO_BENCHMARK_HPP
#define CONCURRENT_HASH_MAP_TEST_LIBCUCKOO_BENCHMARK_HPP

#include <atomic>
#include <libcuckoo/cuckoohash_map.hh>
#include <random>
#include "Global.hpp"

class Libcuckoo_Value_Benchmark{
public:
    using Table = libcuckoo::cuckoohash_map<uint64_t , Transaction_Table_Entry>;
    static inline std::atomic_int64_t total_erase = 0;
    static inline std::atomic_int64_t total_insert=0;
    static inline std::atomic_int64_t total_read=0;
    static inline std::atomic_int64_t totalOp = 0;
    static inline std::atomic_bool flag = false;
    static void reader(Table* (txnTables)[THREAD_NUM]){
        std::random_device dev;
        std::mt19937 rng(dev());
        int64_t local_delete=0;
        int64_t local_read=0;
        std::uniform_int_distribution<std::mt19937::result_type> threadDist(0,THREAD_NUM-1);
        std::uniform_int_distribution<std::mt19937::result_type> txnIDDist(0,(1ul<<LAZY_UPDATE_WORKLOAD_ORDER)-1);
        //Transaction_Table_Entry* entryPtr;
        while(!Libcuckoo_Value_Benchmark::flag.load());
        auto start = std::chrono::high_resolution_clock::now();
        //long opCount=0;
        while(true){
            int threadID = threadDist(rng);
            int local_txnID = txnIDDist(rng);
            uint64_t txnID = bwtransaction::generateTxnID(threadID,local_txnID);
            bool erase = false;
            txnTables[threadID]->find_fn(txnID,[&erase](Transaction_Table_Entry& entry){erase = entry.op_count.fetch_sub(1)==1;});
            if(erase){
                local_delete++;
                txnTables[threadID]->erase(txnID);
            }
            local_read++;
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            if(duration.count()>60000000){
                break;
            }
            //txnTables[threadID]->update_fn(txnID,[&erase](Transaction_Table_Entry& entry){erase = entry.op_count.fetch_sub(1)==1;});
        }
        total_erase.fetch_add(local_delete);
        total_read.fetch_add(local_read);
    }
    static void writer(uint8_t threadID,Table* (txnTables)[THREAD_NUM]){
        int64_t local_insert=0;
        while(!Libcuckoo_Value_Benchmark::flag.load());
        auto start = std::chrono::high_resolution_clock::now();
        for(uint64_t i = 1ul<<LAZY_UPDATE_WORKLOAD_ORDER; i<=1ul<<50;i++){
            uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
            Transaction_Table_Entry entry;
            txnTables[threadID]->insert(txnID,entry);
            local_insert++;
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            if(duration.count()>60000000){
                break;
            }
        }
        total_insert.fetch_add(local_insert);
    }
    static void initializer(uint8_t threadID,Table* (txnTables)[THREAD_NUM]){
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist10(1,100);
        for(uint64_t i =0; i< 1ul<<LAZY_UPDATE_WORKLOAD_ORDER;i++){
            uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
            Transaction_Table_Entry entry;
            entry.op_count = dist10(rng);
            txnTables[threadID]->insert(txnID,entry);
        }
    }

    void microbenchmark(){
        Table* txnTables[THREAD_NUM];
        for(int i=0;i<THREAD_NUM;i++){
            Table* newMap = new Table();
            txnTables[i]=newMap;
        }
        std::vector<std::thread> initializers;
        for (int i = 0; i < THREAD_NUM; i++) {
            initializers.push_back(std::thread(initializer, i, txnTables));
        }
        for (int i = 0; i < THREAD_NUM; i++) {
            initializers.at(i).join();
        }
        std::vector<std::thread> readers;
        std::vector<std::thread> writers;
        for (int i = 0; i < THREAD_NUM; i++) {
            readers.push_back(std::thread(reader, txnTables));
            writers.push_back(std::thread(writer, i, txnTables));
        }
        Libcuckoo_Value_Benchmark::flag.store(true);
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
        std::cout << "total Libcuckoo Hashmap value op count is " << totalOp << "\n";
        std::cout<<"total Libcuckoo Hashmap value delete count is "<<total_erase<<"\n";
        std::cout<<"total Libcuckoo Hashmap value insert count is "<<total_insert<<"\n";
        std::cout<<"total Libcuckoo Hashmap value read count is "<<total_read<<"\n\n";
    }
};

class Libcuckoo_Pointer_Benchmark{
public:
    static inline std::atomic_int64_t total_erase = 0;
    static inline std::atomic_int64_t total_insert =0;
    static inline std::atomic_int64_t total_read =0;
    using entry_ptr = std::shared_ptr<Transaction_Table_Entry>;
    using Table = libcuckoo::cuckoohash_map<uint64_t ,entry_ptr>;
    static inline std::atomic_int64_t totalOp = 0;
    static inline std::atomic_bool flag = false;

    static void writer(uint8_t threadID,Table* (txnTables)[THREAD_NUM]){
        //long opCount=0;
        int64_t local_insert=0;
        while(!Libcuckoo_Pointer_Benchmark::flag.load());
        auto start = std::chrono::high_resolution_clock::now();
        for(uint64_t i = 1ul<<LAZY_UPDATE_WORKLOAD_ORDER; i<=1ul<<50;i++){
            uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
            //Transaction_Table_Entry* entry = new Transaction_Table_Entry();
            entry_ptr entry (new Transaction_Table_Entry());
            txnTables[threadID]->insert(txnID,entry);
            local_insert++;
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            if(duration.count()>60000000){
                break;
            }
        }
        total_insert.fetch_add(local_insert);
    }
    static void reader(Table* (txnTables)[THREAD_NUM]){
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> threadDist(0,THREAD_NUM-1);
        std::uniform_int_distribution<std::mt19937::result_type> txnIDDist(0,(1ul<<LAZY_UPDATE_WORKLOAD_ORDER)-1);
        //Transaction_Table_Entry* entryPtr;
        int64_t local_delete =0;
        int64_t local_read=0;
        while(!Libcuckoo_Pointer_Benchmark::flag.load());
        auto start = std::chrono::high_resolution_clock::now();
        //long opCount=0;
        while(true){
            int threadID = threadDist(rng);
            int local_txnID = txnIDDist(rng);
            uint64_t txnID = bwtransaction::generateTxnID(threadID,local_txnID);
            entry_ptr ptr;
            if(txnTables[threadID]->find(txnID,ptr)){
                if(ptr->op_count.fetch_sub(1)==1){
                    local_delete++;
                    txnTables[threadID]->erase(txnID);
                }
            }
            local_read++;
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            if(duration.count()>60000000){
                break;
            }
            //txnTables[threadID]->update_fn(txnID,[&erase](Transaction_Table_Entry& entry){erase = entry.op_count.fetch_sub(1)==1;});
        }
        total_erase.fetch_add(local_delete);
        total_read.fetch_add(local_read);
    }


    static void initializer(uint8_t threadID,Table* (txnTables)[THREAD_NUM]){
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist10(1,100);
        for(uint64_t i =0; i< 1ul<<LAZY_UPDATE_WORKLOAD_ORDER;i++){
            uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
            /*Transaction_Table_Entry entry;
            entry.op_count = dist10(rng);*/
            Transaction_Table_Entry* entry_ptr = new Transaction_Table_Entry();
            entry_ptr->op_count= dist10(rng);
            txnTables[threadID]->insert(txnID,entry_ptr);
        }
    }
    void microbenchmark(){
        Table* txnTables[THREAD_NUM];
        for(int i=0;i<THREAD_NUM;i++){
            Table* newMap = new Table();
            txnTables[i]=newMap;
        }
        std::vector<std::thread> initializers;
        for (int i = 0; i < THREAD_NUM; i++) {
            initializers.push_back(std::thread(initializer, i, txnTables));
        }
        for (int i = 0; i < THREAD_NUM; i++) {
            initializers.at(i).join();
        }
        std::vector<std::thread> readers;
        std::vector<std::thread> writers;
        for (int i = 0; i < THREAD_NUM; i++) {
            readers.push_back(std::thread(reader, txnTables));
            writers.push_back(std::thread(writer, i, txnTables));
        }
        Libcuckoo_Pointer_Benchmark::flag.store(true);
        for (int i = 0; i < THREAD_NUM; i++) {
            readers.at(i).join();
            writers.at(i).join();
        }
        for(int i=0; i<THREAD_NUM;i++){
            txnTables[i]->clear();
        }
        for(int i=0;i<THREAD_NUM;i++){
            delete txnTables[i];
        }
        totalOp.fetch_add(total_erase);
        totalOp.fetch_add(total_read);
        totalOp.fetch_add(total_insert);
        std::cout << "total Libcuckoo Hashmap pointer op count is " << totalOp << "\n";
        std::cout<<"total Libcuckoo Hashmap pointer delete count is "<<total_erase<<"\n";
        std::cout<<"total Libcuckoo Hashmap pointer insert count is "<<total_insert<<"\n";
        std::cout<<"total Libcuckoo Hashmap pointer read count is "<<total_read<<"\n\n";
    }
};
#endif //CONCURRENT_HASH_MAP_TEST_LIBCUCKOO_BENCHMARK_HPP
