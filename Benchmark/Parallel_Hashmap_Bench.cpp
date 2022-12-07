//
// Created by Libin Zhou on 11/27/22.
//

#include "Parallel_Hashmap_Bench.hpp"

/*void Parallel_Node_Hashmap_Bench::microbenchmark() {
    phmap::parallel_node_hash_map<uint64_t,Transaction_Table_Entry>txnTables[THREAD_NUM];
    //initialize the txn table
    std::vector<std::thread> initializers;
    for(int i=0; i<THREAD_NUM;i++){
        initializers.push_back(std::thread(initializer,i,std::ref(txnTables)));
    }
    for(int i=0;i<THREAD_NUM;i++){
        initializers.at(i).join();
    }
    std::cout<<"initialize done\n";
    std::vector<std::thread>readers;
    std::vector<std::thread>writers;
    for(int i=0; i<THREAD_NUM;i++){
        readers.push_back(std::thread(reader,std::ref(txnTables)));
        writers.push_back(std::thread(writer,i,std::ref(txnTables)));
    }
    Parallel_Node_Hashmap_Bench::flag.store(true);
    for(int i=0;i<THREAD_NUM;i++){
        readers.at(i).join();
        writers.at(i).join();
    }
    std::cout<<"total op count is "<<Parallel_Node_Hashmap_Bench::totalOp<<"\n";
}*/