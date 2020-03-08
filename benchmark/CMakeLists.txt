add_executable(bench_bit_vector bit_vector.cpp)
target_link_libraries(bench_bit_vector tlx tdc-stat tdc-vec)

add_executable(bench_bit_rank bit_rank.cpp)
target_link_libraries(bench_bit_rank tlx tdc-stat tdc-vec)

add_executable(bench_bit_select bit_select.cpp)
target_link_libraries(bench_bit_select tlx tdc-stat tdc-vec)

add_executable(bench_int_vector int_vector.cpp)
target_link_libraries(bench_int_vector tlx tdc-stat tdc-vec)
