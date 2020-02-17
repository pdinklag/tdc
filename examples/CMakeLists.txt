add_executable(stats stats.cpp)
target_link_libraries(stats tdc-stat)

add_executable(stats-rapl stats_rapl.cpp)
target_link_libraries(stats-rapl tdc-stat tdc-rapl)
