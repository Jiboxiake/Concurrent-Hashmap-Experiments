#include <iostream>
//#include <Global.hpp>
#include "Benchmark/Parallel_Hashmap_Bench.hpp"
void testingParallelNodeHashMap(){
    using PMap = phmap::parallel_node_hash_map<
            uint64_t,
            Transaction_Table_Entry,
            phmap::priv::hash_default_hash<uint64_t>,
            phmap::priv::hash_default_eq<uint64_t>,
            std::allocator<std::pair<const uint64_t, Transaction_Table_Entry>>,
            12,
            std::mutex>;
    Parallel_Node_Hashmap_Bench<PMap> bench;
    bench.microbenchmark();
}
void testingParallelFlatHashMap(){
    Parallel_Flat_Hashmap_Bench bench;
    bench.microbenchmark();
}
int main() {
   /* phmap::parallel_node_hash_map<uint64_t,Transaction_Table_Entry> txnTable;
    Transaction_Table_Entry entry;
    entry.op_count=10;
    txnTable.emplace(12345,entry);

    Transaction_Table_Entry* entryPtr = &txnTable[12345];
    entryPtr->op_count.fetch_sub(5);

    Transaction_Table_Entry otherEntry = txnTable.at(12345);
    std::cout<<otherEntry.op_count<<"\n";*/
    //testingParallelNodeHashMap();
    testingParallelFlatHashMap();
    return 0;
}
