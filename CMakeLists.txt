cmake_minimum_required(VERSION 3.23)
project(Concurrent_Hash_Map_Test)
set(CMAKE_CXX_FLAGS_DEBUG "-ltbb -std=c++17 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ltbb -std=c++17  -g")
include_directories(${PROJECT_SOURCE_DIR})
add_library(parallel_hashmap_lib STATIC parallel_hashmap/test.cpp )

find_package(TBB REQUIRED)
include_directories(${TBB_INCLUDE_DIRS})
link_directories(${TBB_LIBRARY_DIRS})
link_libraries(${TBB_LIBRARIES})
add_subdirectory(libcuckoo)
#target_link_libraries(parallel_hashmap_lib -L./parallel_hashmap/)
#parallel_hashmap/btree.h parallel_hashmap/meminfo.h parallel_hashmap/phmap.h parallel_hashmap/phmap_base.h parallel_hashmap/phmap_bits.h parallel_hashmap/phmap_config.h parallel_hashmap/phmap_dump.h parallel_hashmap/phmap_fwd_decl.h parallel_hashmap/phmap_utils.h
find_package(Boost REQUIRED)
set(CMAKE_CXX_STANDARD 17)
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)
add_executable(Concurrent_Hash_Map_Test main.cpp Benchmark/Parallel_Hashmap_Bench.cpp Benchmark/Parallel_Hashmap_Bench.hpp Global.hpp Benchmark/TBBHashMap.hpp Benchmark/Libcuckoo_Benchmark.hpp)
target_include_directories(parallel_hashmap_lib PUBLIC parallel_hashmap)
target_link_libraries(Concurrent_Hash_Map_Test parallel_hashmap_lib)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,stack-size=100000000")
#target_link_libraries(Concurrent_Hash_Map_Test PRIVATE "-Wl,--stack,10000000")
