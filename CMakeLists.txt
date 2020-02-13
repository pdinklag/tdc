cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)
project(tdc)

# include CMake modules
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmakemodules")

# extlib
set(TDC_EXTLIB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extlib")
set(TDC_EXTLIB_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/extlib")

# build flags
set(CXX_STANDARD c++17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=gnu++17 ${GCC_WARNINGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")

# build type
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE not defined - defaulting to Debug.")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    message(STATUS "CMAKE_BUILD_TYPE is Release")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    message(STATUS "CMAKE_BUILD_TYPE is Debug")
else()
    message(FATAL_ERROR "Unsupported CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

# init submodules
include(InitSubmodules)

# use tlx
add_subdirectory(${TDC_EXTLIB_SOURCE_DIR}/tlx)
include_directories(${TDC_EXTLIB_SOURCE_DIR}/tlx/tlx)

# include
include_directories(${CMAKE_SOURCE_DIR}/include)

# subdirectories
add_subdirectory(docs)
add_subdirectory(src)
add_subdirectory(test)
