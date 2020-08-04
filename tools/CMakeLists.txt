add_executable(dynds dynds.cpp)
target_link_libraries(dynds tlx tdc-random tdc-math)

add_executable(permutation permutation.cpp)
target_link_libraries(permutation tlx tdc-random tdc-math)
