add_executable(bench_bit_vector bench_bit_vector.cpp)
target_link_libraries(bench_bit_vector tlx tdc-stat tdc-vec)

add_executable(bench_bit_rank bench_bit_rank.cpp)
target_link_libraries(bench_bit_rank tlx tdc-stat tdc-vec)

add_executable(bench_bit_select bench_bit_select.cpp)
target_link_libraries(bench_bit_select tlx tdc-stat tdc-vec)

add_executable(bench_cmp bench_cmp.cpp)
target_link_libraries(bench_cmp tlx tdc-intrisics tdc-stat)

add_executable(bench_coders bench_coders.cpp)
target_link_libraries(bench_coders tlx tdc-code tdc-io tdc-stat tdc-vec)

add_executable(bench_hash_set bench_hash_set.cpp)
target_link_libraries(bench_hash_set tlx tdc-stat tdc-random tdc-math)

add_executable(bench_int_vector bench_int_vector.cpp)
target_link_libraries(bench_int_vector tlx tdc-stat tdc-vec)

add_executable(bench_predecessor bench_predecessor.cpp)
target_link_libraries(bench_predecessor tlx tdc-stat tdc-random tdc-math tdc-pred tdc-vec)

add_executable(bench_predecessor_dynamic bench_predecessor_dynamic.cpp)
target_link_libraries(bench_predecessor_dynamic tlx tdc-stat tdc-random tdc-rapl tdc-math tdc-io tdc-pred tdc-intrisics tdc-vec)

if(LEDA_FOUND AND STREE_FOUND)
    target_include_directories(bench_predecessor_dynamic PUBLIC ${LEDA_INCLUDE_DIRS} ${STREE_INCLUDE_DIRS})
    target_link_libraries(bench_predecessor_dynamic ${LEDA_LIBRARIES})
endif()
target_include_directories(bench_predecessor_dynamic PUBLIC ${TDC_EXTLIB_SOURCE_DIR}/integer-structures)
    
add_executable(bench_sketch bench_sketch.cpp)
target_link_libraries(bench_sketch tlx tdc-random tdc-math tdc-stat)

add_executable(bench_sorted_sequence bench_sorted_sequence.cpp)
target_link_libraries(bench_sorted_sequence tlx tdc-stat tdc-random tdc-math tdc-vec)
