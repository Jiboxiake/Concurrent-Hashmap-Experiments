cmake_minimum_required(VERSION 3.23)
project(Concurrent_Hash_Map_Test)
include_directories(${PROJECT_SOURCE_DIR})
add_library(parallel_hashmap_lib STATIC parallel_hashmap/test.cpp )
#target_link_libraries(parallel_hashmap_lib -L./parallel_hashmap/)
#parallel_hashmap/btree.h parallel_hashmap/meminfo.h parallel_hashmap/phmap.h parallel_hashmap/phmap_base.h parallel_hashmap/phmap_bits.h parallel_hashmap/phmap_config.h parallel_hashmap/phmap_dump.h parallel_hashmap/phmap_fwd_decl.h parallel_hashmap/phmap_utils.h
find_package(Boost REQUIRED)
set(CMAKE_CXX_STANDARD 17)

#set_property(TARGET parallel_hashmap PROPERTY POSITION_INDEPENDENT_CODE ON)
add_executable(Concurrent_Hash_Map_Test main.cpp Benchmark/Parallel_Hashmap_Bench.cpp Benchmark/Parallel_Hashmap_Bench.hpp Global.hpp Benchmark/TBBHashMap.hpp)
target_include_directories(parallel_hashmap_lib PUBLIC parallel_hashmap)
target_link_libraries(Concurrent_Hash_Map_Test parallel_hashmap_lib)
