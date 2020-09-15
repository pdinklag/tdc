add_library(tdc-pred
    binary_search.cpp
    binary_search_hybrid.cpp
    fusion_node_8.cpp
    fusion_node_internals.cpp
    index.cpp
    octrie.cpp
    octrie_top.cpp
    dynamic/dynamic_fusion_node_8.cpp
    dynamic/btree.cpp
    dynamic/dynamic_rankselect.cpp)

target_compile_options(tdc-pred PUBLIC -mbmi2 -mlzcnt -mpopcnt)

if(PLADS_FOUND)
    target_include_directories(tdc-pred PUBLIC ${PLADS_INCLUDE_DIRS})
endif()
