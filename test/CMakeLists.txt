add_executable(test_btree_min test_btree_min.cpp)
target_link_libraries(test_btree_min)
set_target_properties(test_btree_min PROPERTIES OUTPUT_NAME btree-min)
add_test(btree_min btree-min)

add_executable(test_min test_min.cpp)
set_target_properties(test_min PROPERTIES OUTPUT_NAME min)
target_link_libraries(test_min)
add_test(min min)

add_executable(test_mmap test_mmap.cpp)
set_target_properties(test_mmap PROPERTIES OUTPUT_NAME map)
target_link_libraries(test_mmap tdc-io)
add_test(mmap mmap)

add_executable(test_framework test_framework.cpp)
set_target_properties(test_framework PROPERTIES OUTPUT_NAME framework)
target_link_libraries(test_framework)
add_test(framework framework)

add_executable(test_vectors test_vectors.cpp)
set_target_properties(test_vectors PROPERTIES OUTPUT_NAME vectors)
target_link_libraries(test_vectors tdc-vec)
add_test(vectors vectors)
