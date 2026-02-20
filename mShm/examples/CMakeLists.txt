
set(TEST_TOOL_SHM test_tool_${MSHM_LIB_NAME})

add_executable(${TEST_TOOL_SHM}_writer
    main_writer.cpp
)

set_target_properties(${TEST_TOOL_SHM}_writer PROPERTIES
	FOLDER "TESTS"
)

target_link_libraries(${TEST_TOOL_SHM}_writer
	PUBLIC 
        ${MSHM_LIB_NAME}
)

install(TARGETS ${TEST_TOOL_SHM}_writer ${RUNTIME_VAR}
	DESTINATION ${CMAKE_INSTALL_PREFIX}/examples
	COMPONENT ${TEST_TOOL_SHM}
)


add_executable(${TEST_TOOL_SHM}_reader
    main_reader.cpp
)

set_target_properties(${TEST_TOOL_SHM}_reader PROPERTIES
    FOLDER "TESTS"
)

target_link_libraries(${TEST_TOOL_SHM}_reader
    PUBLIC
        ${MSHM_LIB_NAME}
)

install(TARGETS ${TEST_TOOL_SHM}_reader ${RUNTIME_VAR}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/examples
    COMPONENT ${TEST_TOOL_SHM}
)

