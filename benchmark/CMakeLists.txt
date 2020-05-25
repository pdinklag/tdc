add_executable(bench_bit_vector bench_bit_vector.cpp)
target_link_libraries(bench_bit_vector tlx tdc-stat tdc-vec)

add_executable(bench_bit_rank bench_bit_rank.cpp)
target_link_libraries(bench_bit_rank tlx tdc-stat tdc-vec)

add_executable(bench_bit_select bench_bit_select.cpp)
target_link_libraries(bench_bit_select tlx tdc-stat tdc-vec)

add_executable(bench_coders bench_coders.cpp)
target_link_libraries(bench_coders tlx tdc-code tdc-io tdc-stat tdc-vec)

add_executable(bench_int_vector bench_int_vector.cpp)
target_link_libraries(bench_int_vector tlx tdc-stat tdc-vec)

add_executable(bench_predecessor bench_predecessor.cpp)
target_link_libraries(bench_predecessor tlx tdc-stat tdc-random tdc-math tdc-pred tdc-vec)

add_executable(bench_predecessor_dynamic bench_predecessor_dynamic.cpp)
target_link_libraries(bench_predecessor_dynamic tlx tdc-stat tdc-random tdc-math tdc-pred tdc-vec)

add_executable(bench_sorted_sequence bench_sorted_sequence.cpp)
target_link_libraries(bench_sorted_sequence tlx tdc-stat tdc-random tdc-math tdc-vec)
