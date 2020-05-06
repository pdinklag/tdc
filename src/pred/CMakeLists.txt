add_library(tdc-pred binary_search.cpp binary_search_hybrid.cpp compressed_trie8.cpp index.cpp octrie.cpp octrie_top.cpp)
target_compile_options(tdc-pred PUBLIC -mbmi2 -mlzcnt -mpopcnt)
