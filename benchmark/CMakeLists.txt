add_executable(bench_bit_vector bench_bit_vector.cpp)
set_target_properties(bench_bit_vector PROPERTIES OUTPUT_NAME bit-vector)
target_link_libraries(bench_bit_vector tlx tdc-stat tdc-vec)

add_executable(bench_bit_rank bench_bit_rank.cpp)
set_target_properties(bench_bit_rank PROPERTIES OUTPUT_NAME bit-rank)
target_link_libraries(bench_bit_rank tlx tdc-stat tdc-vec)

add_executable(bench_bit_select bench_bit_select.cpp)
set_target_properties(bench_bit_select PROPERTIES OUTPUT_NAME bit-select)
target_link_libraries(bench_bit_select tlx tdc-stat tdc-vec)

add_executable(bench_cmp bench_cmp.cpp)
set_target_properties(bench_cmp PROPERTIES OUTPUT_NAME cmp)
target_link_libraries(bench_cmp tlx tdc-intrisics tdc-stat)

add_executable(bench_coders bench_coders.cpp)
set_target_properties(bench_coders PROPERTIES OUTPUT_NAME coders)
target_link_libraries(bench_coders tlx tdc-code tdc-io tdc-stat tdc-vec)

add_executable(bench_hash_set bench_hash_set.cpp)
set_target_properties(bench_hash_set PROPERTIES OUTPUT_NAME hash-set)
target_link_libraries(bench_hash_set tlx tdc-stat tdc-random)

add_executable(bench_int_vector bench_int_vector.cpp)
set_target_properties(bench_int_vector PROPERTIES OUTPUT_NAME int-vector)
target_link_libraries(bench_int_vector tlx tdc-stat tdc-vec)

add_executable(bench_lz77 bench_lz77.cpp)
set_target_properties(bench_lz77 PROPERTIES OUTPUT_NAME lz77)
target_include_directories(bench_lz77 PUBLIC ${TDC_EXTLIB_BINARY_DIR}/libdivsufsort/include)
target_link_libraries(bench_lz77 tlx divsufsort tdc-stat tdc-random)

add_executable(bench_lz78 bench_lz78.cpp)
set_target_properties(bench_lz78 PROPERTIES OUTPUT_NAME lz78)
target_link_libraries(bench_lz78 tlx tdc-stat)

add_executable(bench_predecessor bench_predecessor.cpp)
set_target_properties(bench_predecessor PROPERTIES OUTPUT_NAME predecessor-static)
target_link_libraries(bench_predecessor tlx tdc-stat tdc-random tdc-pred tdc-vec)

add_executable(bench_predecessor_dynamic bench_predecessor_dynamic.cpp)
set_target_properties(bench_predecessor_dynamic PROPERTIES OUTPUT_NAME predecessor-dynamic)
target_link_libraries(bench_predecessor_dynamic tlx tdc-stat tdc-random tdc-rapl tdc-io tdc-pred tdc-intrisics tdc-vec)

if(LEDA_FOUND AND STREE_FOUND)
    target_include_directories(bench_predecessor_dynamic PUBLIC ${LEDA_INCLUDE_DIRS} ${STREE_INCLUDE_DIRS})
    target_link_libraries(bench_predecessor_dynamic ${LEDA_LIBRARIES})
endif()
target_include_directories(bench_predecessor_dynamic PUBLIC ${TDC_EXTLIB_SOURCE_DIR}/integer-structures)
    
add_executable(bench_sketch bench_sketch.cpp)
set_target_properties(bench_sketch PROPERTIES OUTPUT_NAME sketch)
target_link_libraries(bench_sketch tlx tdc-random tdc-stat)

add_executable(bench_sorted_sequence bench_sorted_sequence.cpp)
set_target_properties(bench_sorted_sequence PROPERTIES OUTPUT_NAME sorted-sequence)
target_link_libraries(bench_sorted_sequence tlx tdc-stat tdc-random tdc-vec)
