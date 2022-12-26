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
    static inline std::atomic_int64_t total_erase = 0;
    static inline std::atomic_int64_t total_read = 0;
    static inline std::atomic_int64_t total_insert = 0;
    // reader will do lazy updates
    Parallel_Node_Hashmap_Bench(){};
    static void reader(Map* (txnTables)[THREAD_NUM]){
        std::random_device dev;
        std::mt19937 rng(dev());
        int64_t local_total_delete = 0 ;
        int64_t local_read = 0;
        std::uniform_int_distribution<std::mt19937::result_type> threadDist(0,THREAD_NUM-1);
        std::uniform_int_distribution<std::mt19937::result_type> txnIDDist(0,(1ul<<LAZY_UPDATE_WORKLOAD_ORDER)-1);
        //Transaction_Table_Entry* entryPtr;
        while(!Parallel_Node_Hashmap_Bench::flag.load());
        auto start = std::chrono::high_resolution_clock::now();
        while(true){
            int threadID = threadDist(rng);
            int local_txnID = txnIDDist(rng);
            uint64_t txnID = bwtransaction::generateTxnID(threadID,local_txnID);
            bool erase = false;
            txnTables[threadID]->modify_if(txnID,[&erase](typename Map::value_type& pair){erase=(pair.second.op_count.fetch_sub(1)==1);});
            if(erase){
                local_total_delete++;
                txnTables[threadID]->erase(txnID);
            }
            local_read++;
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            if(duration.count()>60000000){
                break;
            }
        }
        total_erase.fetch_add(local_total_delete);
        total_read.fetch_add(local_read);
    }
    //writer will insert entries.
    //range: 2^24 to 2^50
    static void writer(uint8_t threadID,Map* (txnTables)[THREAD_NUM]){
        long local_insert=0;
        while(!Parallel_Node_Hashmap_Bench::flag.load());
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
    }void microbenchmark(){
        //std::cout<<"size is "<<sizeof(Map)<<"\n";
        Map* txnTables[THREAD_NUM];
        for(int i=0;i<THREAD_NUM;i++){
            Map* newMap = new Map();
            txnTables[i]=newMap;
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
        Parallel_Node_Hashmap_Bench::flag.store(true);
        for (int i = 0; i < THREAD_NUM; i++) {
            readers.at(i).join();
            writers.at(i).join();
        }
        totalOp.fetch_add(total_erase);
        totalOp.fetch_add(total_read);
        totalOp.fetch_add(total_insert);
        std::cout << "total Node Hashmap op count is " << Parallel_Node_Hashmap_Bench::totalOp << "\n";
        std::cout<<"total Node Hashmap delete count is "<<total_erase<<"\n";
        std::cout<<"total Node Hashmap insert count is "<<total_insert<<"\n";
        std::cout<<"total Node Hashmap read count is "<<total_read<<"\n\n";
        for(int i=0;i<THREAD_NUM;i++){
           delete txnTables[i];
        }
    }
};
//template<class Map>
//todo: don't use it.
class Parallel_Flat_Hashmap_Pointer_Bench{
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

    Parallel_Flat_Hashmap_Pointer_Bench(){};

    static void initializer(uint8_t threadID,Map* (txnTables)[THREAD_NUM]){
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist10(1,20); // distribution in range [1, 6]
        for(uint64_t i =0; i<= 1ul<<LAZY_UPDATE_WORKLOAD_ORDER;i++){
            uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
            Transaction_Table_Entry* entry = new Transaction_Table_Entry();
            entry->op_count = dist10(rng);
            txnTables[threadID]->emplace(txnID,entry);
        }
    }

    static void reader(Map* (txnTables)[THREAD_NUM]){
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> threadDist(0,THREAD_NUM-1);
        std::uniform_int_distribution<std::mt19937::result_type> txnIDDist(0,(1ul<<LAZY_UPDATE_WORKLOAD_ORDER)-1);
        //Transaction_Table_Entry* entryPtr;
        while(!Parallel_Flat_Hashmap_Pointer_Bench::flag.load());
        auto start = std::chrono::high_resolution_clock::now();
        long opCount=0;
        while(true){
            int threadID = threadDist(rng);
            int local_txnID = txnIDDist(rng);
            uint64_t txnID = bwtransaction::generateTxnID(threadID,local_txnID);
            bool erase = false;
            Transaction_Table_Entry* copy;
            txnTables[threadID]->if_contains(txnID,[&erase,&copy](typename Map::value_type& pair){copy =pair.second;erase=(copy->op_count.fetch_sub(1)==1);});
            if(erase){
                txnTables[threadID]->erase(txnID);
                delete copy;
            }
            opCount++;
            //opCount++;
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            if(duration.count()>60000000){
                break;
            }
        }
        totalOp.fetch_add(opCount);
    }
    static void writer(uint8_t threadID,Map* (txnTables)[THREAD_NUM]){
        long opCount = 0;
        while (!Parallel_Flat_Hashmap_Pointer_Bench::flag.load());
        auto start = std::chrono::high_resolution_clock::now();
        for(uint64_t i = 1ul<<LAZY_UPDATE_WORKLOAD_ORDER; i<=1ul<<50;i++){
            uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
            Transaction_Table_Entry* entry = new Transaction_Table_Entry();
            txnTables[threadID]->emplace(txnID,entry);
            opCount++;
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            if(duration.count()>60000000){
                break;
            }
        }
        totalOp.fetch_add(opCount);
    }
    void microbenchmark(){
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
        Parallel_Flat_Hashmap_Pointer_Bench::flag.store(true);
        for (int i = 0; i < THREAD_NUM; i++) {
            readers.at(i).join();
            writers.at(i).join();
        }
        for(int i=0; i<THREAD_NUM;i++){
            for(auto it = txnTables[i]->begin();it!=txnTables[i]->end();it++){
                delete it->second;
            }
        }
        for(int i=0;i<THREAD_NUM;i++){
            delete txnTables[i];
        }
        std::cout << "total Flat Hashmap op count is " << Parallel_Flat_Hashmap_Pointer_Bench::totalOp << "\n";
    }
};

class Parallel_Flat_Hashmap_Value_Bench{
     using Map = phmap::parallel_flat_hash_map<
             uint64_t,
             Transaction_Table_Entry,
             phmap::priv::hash_default_hash<uint64_t>,
             phmap::priv::hash_default_eq<uint64_t>,
             std::allocator<std::pair<const uint64_t, Transaction_Table_Entry>>,
             12,
             std::mutex>;
 public:
     static inline std::atomic_int64_t total_erase = 0;
     static inline std::atomic_int64_t total_read=0;
     static inline std::atomic_int64_t total_insert=0;
     static inline std::atomic_int64_t totalOp = 0;
     // static phmap::parallel_node_hash_map<uint64_t,Transaction_Table_Entry>txnTables[THREAD_NUM];
     static inline std::atomic_bool flag = false;
     // reader will do lazy updates
     Parallel_Flat_Hashmap_Value_Bench(){};
     static void reader(Map* (txnTables)[THREAD_NUM]){
         std::random_device dev;
         std::mt19937 rng(dev());
         int64_t local_delete = 0;
         int64_t local_read =0;
         std::uniform_int_distribution<std::mt19937::result_type> threadDist(0,THREAD_NUM-1);
         std::uniform_int_distribution<std::mt19937::result_type> txnIDDist(0,(1ul<<LAZY_UPDATE_WORKLOAD_ORDER)-1);
         //Transaction_Table_Entry* entryPtr;
         while(!Parallel_Flat_Hashmap_Value_Bench::flag.load());
         auto start = std::chrono::high_resolution_clock::now();
         //long opCount=0;
         while(true){
             int threadID = threadDist(rng);
             int local_txnID = txnIDDist(rng);
             uint64_t txnID = bwtransaction::generateTxnID(threadID,local_txnID);
             bool erase = false;
             txnTables[threadID]->modify_if(txnID,[&erase](typename Map::value_type& pair){erase=(pair.second.op_count.fetch_sub(1)==1);});
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
         }
         total_erase.fetch_add(local_delete);
         total_read.fetch_add(local_read);
     }
     //writer will insert entries.
     //range: 2^24 to 2^50
     static void writer(uint8_t threadID,Map* (txnTables)[THREAD_NUM]){
         int64_t local_insert=0;
         while(!Parallel_Flat_Hashmap_Value_Bench::flag.load());
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
     void microbenchmark(){
         //std::cout<<"size is "<<sizeof(Map)<<"\n";
         Map* txnTables[THREAD_NUM];
         for(int i=0;i<THREAD_NUM;i++){
             Map* newMap = new Map();
             txnTables[i]=newMap;
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
         Parallel_Flat_Hashmap_Value_Bench::flag.store(true);
         for (int i = 0; i < THREAD_NUM; i++) {
             readers.at(i).join();
             writers.at(i).join();
         }
         totalOp.fetch_add(total_erase);
         totalOp.fetch_add(total_read);
         totalOp.fetch_add(total_insert);
         std::cout << "total Flat Hashmap Value op count is " << totalOp << "\n";
         std::cout<<"total Flat Hashmap Value delete count is "<<total_erase<<"\n";
         std::cout<<"total Flat Hashmap Value insert count is "<<total_insert<<"\n";
         std::cout<<"total Flat Hashmap Value read count is "<<total_read<<"\n\n";
         for(int i=0;i<THREAD_NUM;i++){
             delete txnTables[i];
         }
     }
 };

 class Parallel_Flat_Hashmap_shared_ptr_Bench{
    using entry_ptr = std::shared_ptr<Transaction_Table_Entry>;
     using Map = phmap::parallel_flat_hash_map<
             uint64_t,
             entry_ptr ,
             phmap::priv::hash_default_hash<uint64_t>,
             phmap::priv::hash_default_eq<uint64_t>,
             std::allocator<std::pair<const uint64_t, entry_ptr>>,
             12,
             std::mutex>;
 public:
     static inline std::atomic_int64_t total_erase = 0;
     static inline std::atomic_int64_t total_insert=0;
     static inline std::atomic_int64_t total_read =0;
     static inline std::atomic_int64_t totalOp = 0;
     // static phmap::parallel_node_hash_map<uint64_t,Transaction_Table_Entry>txnTables[THREAD_NUM];
     static inline std::atomic_bool flag = false;

     static void writer(uint8_t threadID,Map* (txnTables)[THREAD_NUM]){
         //long opCount=0;
         int64_t local_insert=0;
         while(!flag.load());
         auto start = std::chrono::high_resolution_clock::now();
         for(uint64_t i = 1ul<<LAZY_UPDATE_WORKLOAD_ORDER; i<=1ul<<50;i++){
             uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
             entry_ptr entry (new Transaction_Table_Entry());
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
         int64_t local_delete = 0;
         int64_t local_read =0;
         //Transaction_Table_Entry* entryPtr;
         while(!flag.load());
         auto start = std::chrono::high_resolution_clock::now();
         //long opCount=0;
         while(true){
             int threadID = threadDist(rng);
             int local_txnID = txnIDDist(rng);
             uint64_t txnID = bwtransaction::generateTxnID(threadID,local_txnID);
             entry_ptr ptr= nullptr;
             //entry_ptr ptr =txnTables[threadID]->find(txnID)->second;
             txnTables[threadID]->if_contains(txnID,[&ptr](typename Map::value_type& pair){ptr = pair.second;});
             if(ptr!=nullptr &&ptr->op_count.fetch_sub(1)==1){
                 local_delete++;
                 txnTables[threadID]->erase(txnID);
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

     static void initializer(uint8_t threadID,Map* (txnTables)[THREAD_NUM]){
         std::random_device dev;
         std::mt19937 rng(dev());
         std::uniform_int_distribution<std::mt19937::result_type> dist10(1,100);
         for(uint64_t i =0; i< 1ul<<LAZY_UPDATE_WORKLOAD_ORDER;i++){
             uint64_t txnID = bwtransaction::generateTxnID(threadID,i);
             //Transaction_Table_Entry entry;
             entry_ptr entry (new Transaction_Table_Entry());
             entry->op_count = dist10(rng);
             txnTables[threadID]->emplace(txnID,entry);
         }
     }
     void microbenchmark(){
         //std::cout<<"size is "<<sizeof(Map)<<"\n";
         Map* txnTables[THREAD_NUM];
         for(int i=0;i<THREAD_NUM;i++){
             Map* newMap = new Map();
             txnTables[i]=newMap;
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
         flag.store(true);
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
         std::cout << "total Flat Hashmap pointer op count is " << totalOp << "\n";
         std::cout<<"total Flat Hashmap pointer delete count is "<<total_erase<<"\n";
         std::cout<<"total Flat Hashmap pointer insert count is "<<total_insert<<"\n";
         std::cout<<"total Flat Hashmap pointer read count is "<<total_read<<"\n\n";
     }
 };

#endif //CONCURRENT_HASH_MAP_TEST_PARALLEL_HASHMAP_BENCH_HPP
