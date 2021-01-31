add_executable(bench_cmp bench_cmp.cpp)
target_link_libraries(bench_cmp tlx tdc-intrisics tdc-stat)

add_executable(bench_predecessor_dynamic bench_predecessor_dynamic.cpp)
target_link_libraries(bench_predecessor_dynamic tlx tdc-stat tdc-random tdc-math tdc-pred tdc-intrisics tdc-vec)

if(LEDA_FOUND AND STREE_FOUND)
    target_include_directories(bench_predecessor_dynamic PUBLIC ${LEDA_INCLUDE_DIRS} ${STREE_INCLUDE_DIRS})
    target_link_libraries(bench_predecessor_dynamic ${LEDA_LIBRARIES})
endif()
target_include_directories(bench_predecessor_dynamic PUBLIC ${TDC_EXTLIB_SOURCE_DIR}/integer-structures)
