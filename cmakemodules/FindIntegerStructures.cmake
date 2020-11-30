# Find Integer Structures by Bick Nash
include(FindPackageHandleStandardArgs)

set(INTEGERSTRUCTURES_ROOT_DIR "" CACHE PATH "Path to Integer Structures repository by Nick Nash")

find_path(INTEGERSTRUCTURES_INCLUDE_DIR btrie/lpcbtrie.h PATHS ${INTEGERSTRUCTURES_ROOT_DIR})
find_package_handle_standard_args(IntegerStructures DEFAULT_MSG INTEGERSTRUCTURES_INCLUDE_DIR)

if(INTEGERSTRUCTURES_FOUND)
    add_definitions(-DINTEGERSTRUCTURES_FOUND)
    set(INTEGERSTRUCTURES_INCLUDE_DIRS ${INTEGERSTRUCTURES_INCLUDE_DIR})
endif()
