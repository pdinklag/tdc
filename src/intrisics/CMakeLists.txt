add_library(tdc-intrisics intrisics.cpp)

target_compile_options(tdc-intrisics PUBLIC -march=native -mbmi2)
