# Find LEDA
include(FindPackageHandleStandardArgs)

set(LEDA_ROOT_DIR "" CACHE PATH "Path to LEDA")

find_path(LEDA_INCLUDE_DIR LEDA/internal/system.h PATHS ${LEDA_ROOT_DIR} PATH_SUFFIXES incl)
find_library(LEDA_LIBRARY leda PATHS ${LEDA_ROOT_DIR})
find_package_handle_standard_args(LEDA DEFAULT_MSG LEDA_INCLUDE_DIR LEDA_LIBRARY)

if(LEDA_FOUND)
    find_package(X11 REQUIRED)

    add_definitions(-DLEDA_FOUND)
    add_definitions(-DLEDA_CHECKING_OFF)
    set(LEDA_INCLUDE_DIRS ${LEDA_INCLUDE_DIR})
    set(LEDA_LIBRARIES ${LEDA_LIBRARY} ${X11_LIBRARIES})
endif()
