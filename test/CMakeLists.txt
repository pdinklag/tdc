add_executable(test_btree_min test_btree_min.cpp)
target_link_libraries(test_btree_min)
add_test(btree_min test_btree_min)

add_executable(test_min test_min.cpp)
target_link_libraries(test_min)
add_test(min test_min)

add_executable(test_mmap test_mmap.cpp)
target_link_libraries(test_mmap tdc-io)
add_test(mmap test_mmap)

add_executable(test_framework test_framework.cpp)
set_target_properties(test_framework PROPERTIES OUTPUT_NAME framework)
target_link_libraries(test_framework)
add_test(framework framework)

add_executable(test_vectors test_vectors.cpp)
target_link_libraries(test_vectors tdc-vec)
add_test(vectors test_vectors)
