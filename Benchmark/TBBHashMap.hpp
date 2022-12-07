//
// Created by Libin Zhou on 12/6/22.
//

#ifndef CONCURRENT_HASH_MAP_TEST_TBBHASHMAP_HPP
#define CONCURRENT_HASH_MAP_TEST_TBBHASHMAP_HPP
#include "../Global.hpp"

class TBB_Hashmap_Benchmark{
public:
    static inline std::atomic_int64_t totalOp = 0;
    static inline std::atomic_bool flag = false;
    TBB_Hashmap_Benchmark(){};
    void michrobenchmark(){

    }
}
#endif //CONCURRENT_HASH_MAP_TEST_TBBHASHMAP_HPP
