# Find Powercap
include(FindPackageHandleStandardArgs)

find_path(POWERCAP_INCLUDE_DIR powercap/powercap.h)
find_library(POWERCAP_LIBRARY powercap)
find_package_handle_standard_args(POWERCAP DEFAULT_MSG POWERCAP_INCLUDE_DIR POWERCAP_LIBRARY)

if(POWERCAP_FOUND)
    add_definitions(-DPOWERCAP_FOUND)
    set(POWERCAP_INCLUDE_DIRS ${POWERCAP_INCLUDE_DIR})
    set(POWERCAP_LIBRARIES ${POWERCAP_LIBRARY})

    # try to find number of RAPL packages
    execute_process(
        COMMAND rapl-info -n
        RESULT_VARIABLE RAPL_INFO_RESULT
        OUTPUT_VARIABLE RAPL_INFO_OUT
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(${RAPL_INFO_RESULT} EQUAL 0)
        add_definitions(-DNUM_RAPL_PACKAGES=${RAPL_INFO_OUT})
    else()
        message(WARNING "Failed to find number of RAPL packages on the system, rapl-info returned: ${RAPL_INFO_RESULT}")
    endif()
endif()
