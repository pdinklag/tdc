add_library(tdc-pred
    fusion_node.cpp
    fusion_node_internals.cpp
    index.cpp
    octrie.cpp
    octrie_top.cpp
    dynamic/dynamic_fusion_node.cpp
    dynamic/btree.cpp
    dynamic/dynamic_rankselect.cpp)

target_compile_options(tdc-pred PUBLIC -mlzcnt -mpopcnt)
target_link_libraries(tdc-pred tdc-intrisics)

if(PLADS_FOUND)
    target_include_directories(tdc-pred PUBLIC ${PLADS_INCLUDE_DIRS})
endif()
