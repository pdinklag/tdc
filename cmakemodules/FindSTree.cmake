# Find STree
include(FindPackageHandleStandardArgs)

set(STREE_ROOT_DIR "" CACHE PATH "Path to STree")

find_path(STREE_INCLUDE_DIR veb/STree_orig.h PATHS ${STREE_ROOT_DIR})
find_package_handle_standard_args(STree DEFAULT_MSG STREE_INCLUDE_DIR)

if(STREE_FOUND)
    add_definitions(-DSTREE_FOUND)
    add_definitions(-DUSE_STD_MT_ALLOCATOR)
    set(STREE_INCLUDE_DIRS ${STREE_INCLUDE_DIR})
endif()
