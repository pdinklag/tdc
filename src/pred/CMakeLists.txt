add_library(tdc-pred binary_search.cpp binary_search_hybrid.cpp fusion_node_8.cpp index.cpp octrie.cpp octrie_top.cpp dynamic/dynamic_fusion_node_8.cpp)
target_compile_options(tdc-pred PUBLIC -mbmi2 -mlzcnt -mpopcnt)
