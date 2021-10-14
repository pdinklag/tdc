add_executable(test_btree_min test_btree_min.cpp)
target_link_libraries(test_btree_min)
set_target_properties(test_btree_min PROPERTIES OUTPUT_NAME btree-min)
add_test(btree_min btree-min)

add_executable(test_linked_list_pool test_linked_list_pool.cpp)
set_target_properties(test_linked_list_pool PROPERTIES OUTPUT_NAME linked_list_pool)
add_test(linked_list_pool linked_list_pool)

add_executable(test_mmap test_mmap.cpp)
set_target_properties(test_mmap PROPERTIES OUTPUT_NAME map)
target_link_libraries(test_mmap tdc-io)
add_test(mmap mmap)

add_executable(test_framework test_framework.cpp)
set_target_properties(test_framework PROPERTIES OUTPUT_NAME framework)
target_link_libraries(test_framework tdc-framework)
add_test(framework framework)

add_executable(test_rolling_hash test_rolling_hash.cpp)
set_target_properties(test_rolling_hash PROPERTIES OUTPUT_NAME rolling_hash)
target_link_libraries(test_rolling_hash)
add_test(rolling_hash rolling_hash)

add_executable(test_vectors test_vectors.cpp)
set_target_properties(test_vectors PROPERTIES OUTPUT_NAME vectors)
target_link_libraries(test_vectors tdc-vec)
add_test(vectors vectors)
