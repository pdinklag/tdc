if(MPFR_FOUND)
    add_executable(dynds dynds.cpp)
    target_link_libraries(dynds tlx ${MPFR_LIBRARIES} tdc-random tdc-pred)
endif()

add_executable(permutation permutation.cpp)
target_link_libraries(permutation tlx tdc-random)

add_executable(primes primes.cpp)
target_link_libraries(primes tlx)
