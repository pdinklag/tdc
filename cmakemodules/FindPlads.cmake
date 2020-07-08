# Find PlaDS
include(FindPackageHandleStandardArgs)

set(PLADS_ROOT_DIR "" CACHE PATH "Path to PlaDS")

find_path(PLADS_INCLUDE_DIR plads/vector/util.hpp PATHS ${PLADS_ROOT_DIR} PATH_SUFFIXES include)
find_package_handle_standard_args(Plads DEFAULT_MSG PLADS_INCLUDE_DIR)

if(PLADS_FOUND)
    add_definitions(-DPLADS_FOUND)
    set(PLADS_INCLUDE_DIRS ${PLADS_INCLUDE_DIR})
endif()
