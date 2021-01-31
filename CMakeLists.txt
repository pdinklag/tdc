cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)
project(tdc)

# enable tests
enable_testing()

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

# proper std::filesystem linking for broken g++8
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    link_libraries(stdc++fs)
endif()

# build type
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE not defined - defaulting to Debug.")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "CMAKE_BUILD_TYPE is Release")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    message(STATUS "CMAKE_BUILD_TYPE is Debug")
else()
    message(FATAL_ERROR "Unsupported CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

# init submodules
include(InitSubmodules)

# include extlibs
include_directories(${TDC_EXTLIB_SOURCE_DIR}/ips4o)
include_directories(${TDC_EXTLIB_SOURCE_DIR}/json/single_include)
include_directories(${TDC_EXTLIB_SOURCE_DIR}/robin-hood-hashing/src/include)
add_subdirectory(${TDC_EXTLIB_SOURCE_DIR}/tlx)
include_directories(${TDC_EXTLIB_SOURCE_DIR}/tlx)

# find optional packages
find_package(LEDA)
find_package(STree)

# include
include_directories(${CMAKE_SOURCE_DIR}/include)

# subdirectories
add_subdirectory(benchmark)
add_subdirectory(docs)
add_subdirectory(examples)
add_subdirectory(src)
