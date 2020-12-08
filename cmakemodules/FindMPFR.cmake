# Find GNU Multiple Precision Arithmetic Library
include(FindPackageHandleStandardArgs)

set(GMP_ROOT_DIR "" CACHE PATH "Path to GMP")
find_path(GMP_INCLUDE_DIR gmp.h PATHS ${GMP_ROOT_DIR})
find_library(GMP_LIBRARY gmp PATHS ${GMP_ROOT_DIR} PATH_SUFFIXES ".libs")
find_package_handle_standard_args(GMP DEFAULT_MSG GMP_INCLUDE_DIR GMP_LIBRARY)

set(MPFR_ROOT_DIR "" CACHE PATH "Path to MPFR")
find_path(MPFR_INCLUDE_DIR mpfr.h PATHS ${MPFR_ROOT_DIR} PATH_SUFFIXES "src")
find_library(MPFR_LIBRARY mpfr PATHS ${MPFR_ROOT_DIR} PATH_SUFFIXES "src/.libs")
find_package_handle_standard_args(MPFR DEFAULT_MSG MPFR_INCLUDE_DIR MPFR_LIBRARY)

if(GMP_FOUND AND MPFR_FOUND)
    add_definitions(-DMPFR_FOUND)
    set(MPFR_INCLUDE_DIRS ${MPFR_INCLUDE_DIR} ${GMP_INCLUDE_DIR})
    set(MPFR_LIBRARIES ${MPFR_LIBRARY} ${GMP_LIBRARY})
endif()
