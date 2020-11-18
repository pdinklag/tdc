if(MPFR_FOUND)
    add_executable(dynds dynds.cpp)
    target_link_libraries(dynds tlx ${MPFR_LIBRARIES} tdc-random tdc-pred tdc-math)
endif()

add_executable(permutation permutation.cpp)
target_link_libraries(permutation tlx tdc-random tdc-math)
