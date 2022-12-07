//
// Created by Libin Zhou on 11/27/22.
//

#ifndef CONCURRENT_HASH_MAP_TEST_GLOBAL_HPP
#define CONCURRENT_HASH_MAP_TEST_GLOBAL_HPP

#include <cstdint>
#include <atomic>
#define txnIDMask 0x00FFFFFFFFFFFFFF
#define THREAD_NUM 64
#define LAZY_UPDATE_WORKLOAD_ORDER 14
struct Transaction_Table_Entry{
    Transaction_Table_Entry(){};
    Transaction_Table_Entry(const Transaction_Table_Entry& other){
        this->txn_status.store(other.txn_status);
        this->op_count.store(other.op_count);
    }
    std::atomic_uint64_t txn_status;//which is either in progress, abort or commit (ts).
    std::atomic_int64_t op_count;
    char padding[40];
};
//static_assert(sizeof(Transaction_Table_Entry)==64);
class bwtransaction{
public:
    static uint8_t getThreadID(uint64_t txnID){
        return txnID >> 56;
    }
    static uint64_t generateTxnID(uint8_t threadID,uint64_t txnID){
        return (((uint64_t)threadID)<<56)|(txnID&txnIDMask);
    }
};
#endif //CONCURRENT_HASH_MAP_TEST_GLOBAL_HPP
