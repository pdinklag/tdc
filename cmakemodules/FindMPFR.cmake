# Find GNU Multiple Precision Arithmetic Library
include(FindPackageHandleStandardArgs)

find_path(GMP_INCLUDE_DIR gmp.h)
find_library(GMP_LIBRARY gmp)
find_package_handle_standard_args(GMP DEFAULT_MSG GMP_INCLUDE_DIR GMP_LIBRARY)

find_path(MPFR_INCLUDE_DIR mpfr.h)
find_library(MPFR_LIBRARY mpfr)
find_package_handle_standard_args(MPFR DEFAULT_MSG MPFR_INCLUDE_DIR MPFR_LIBRARY)

if(GMP_FOUND AND MPFR_FOUND)
    add_definitions(-DMPFR_FOUND)
    set(MPFR_INCLUDE_DIRS ${MPFR_INCLUDE_DIR} ${GMP_INCLUDE_DIR})
    set(MPFR_LIBRARIES ${MPFR_LIBRARY} ${GMP_LIBRARY})
endif()
