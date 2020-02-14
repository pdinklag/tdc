# docs target - builds the Doxygen documentation
find_package(Doxygen)

if(DOXYGEN_FOUND)
    # modules
    list(APPEND TDC_DOXYGEN_INPUT
        ${CMAKE_SOURCE_DIR}/README.md
        ${CMAKE_SOURCE_DIR}/include
        # additional special pages go here
        ${CMAKE_CURRENT_SOURCE_DIR}/namespaces.hpp
    )

    string (REPLACE ";" " " TDC_DOXYGEN_INPUT "${TDC_DOXYGEN_INPUT}")

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

    add_custom_target(docs
        #invoke doxygen
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating doxygen documentation"
    )
endif()
