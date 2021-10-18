add_library(tdc-framework
    algorithm_info.cpp
    application.cpp
    config.cpp
)

target_link_libraries(tdc-framework tlx)
