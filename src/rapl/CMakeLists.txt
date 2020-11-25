add_library(tdc-rapl STATIC rapl_phase_extension.cpp reader.cpp)
target_include_directories(tdc-rapl PUBLIC ${POWERCAP_INCLUDE_DIRS})
target_link_libraries(tdc-rapl ${POWERCAP_LIBRARIES})
