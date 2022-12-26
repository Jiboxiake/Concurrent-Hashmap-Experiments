#include <iostream>
//#include <Global.hpp>
#include "Benchmark/Parallel_Hashmap_Bench.hpp"
#include "Benchmark/TBBHashMap.hpp"
#include "Benchmark/Libcuckoo_Benchmark.hpp"
void testTBBMap(){
    tbb::concurrent_hash_map<uint64_t, Transaction_Table_Entry>map;
    Transaction_Table_Entry entry;
    entry.op_count=10;
    entry.txn_status=1345;
    map.emplace(12345,entry);
    tbb::concurrent_hash_map<uint64_t, Transaction_Table_Entry>::const_accessor ac;
    map.find(ac,12345);
   // std::cout<<ac->second.op_count<<","<<ac->second.txn_status<<"\n";
   map.erase(ac);
    tbb::concurrent_hash_map<uint64_t, Transaction_Table_Entry>::const_accessor ac2;
    map.find(ac2,12345);
    assert(ac2.empty());
}
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
void testingParallelFlatHashMapPointer(){
  //  Parallel_Flat_Hashmap_Pointer_Bench bench;
  //  bench.microbenchmark();
}
void testParallelFlatHashMapValue(){
    Parallel_Flat_Hashmap_Value_Bench bench;
    bench.microbenchmark();
}
void testParallelFlatHashMapSharedPtr(){
    Parallel_Flat_Hashmap_shared_ptr_Bench bench;
    bench.microbenchmark();
}
void testTBBConcurrentHashMap(){
    TBB_Hashmap_Benchmark bench;
    bench.michrobenchmark();
}
void testLibcuckooHashTableVale(){
    Libcuckoo_Value_Benchmark bench;
    bench.microbenchmark();
}
void testLibcuckooHashTablePointer(){
    Libcuckoo_Pointer_Benchmark bench;
    bench.microbenchmark();
}
//todo: in real bw workload, we don't need to worry about deleting a pointer. So i assume using pointers are more efficient
int main() {
   /* phmap::parallel_node_hash_map<uint64_t,Transaction_Table_Entry> txnTable;
    Transaction_Table_Entry entry;
    entry.op_count=10;
    txnTable.emplace(12345,entry);

    Transaction_Table_Entry* entryPtr = &txnTable[12345];
    entryPtr->op_count.fetch_sub(5);

    Transaction_Table_Entry otherEntry = txnTable.at(12345);
    std::cout<<otherEntry.op_count<<"\n";*/
   // testingParallelNodeHashMap();
    testParallelFlatHashMapValue();
   // testParallelFlatHashMapSharedPtr();
   // testLibcuckooHashTableVale();
   // testLibcuckooHashTablePointer();
    //testTBBConcurrentHashMap();
    //testTBBMap();
    return 0;
}
