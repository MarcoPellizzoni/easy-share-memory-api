
set(MSHM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" )
set(MSHM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(BUILD_SHARED_LIBS)
   add_library(${MSHM_LIB_NAME} SHARED)
   set(_OUTPUT_NAME "mshm")
	target_compile_definitions( ${MSHM_LIB_NAME} 
		PRIVATE 
			_MSHM_LIB_EXPORT
			_BUILD_MSHM_LIB_DLL
	)
else()
    add_library(${MSHM_LIB_NAME} STATIC)
    set(_OUTPUT_NAME "mshmS")
endif()

if(UNIX)
    set(MSHM_SOURCE_FILES "${MSHM_SOURCE_DIR}/mshm_linux.cpp")
else()
    set(MSHM_SOURCE_FILES "${MSHM_SOURCE_DIR}/mshm_win.cpp")
endif()

set(MSHM_HEADER_FILES "${MSHM_INCLUDE_DIR}/mshm.h")

target_sources(${MSHM_LIB_NAME}
    PRIVATE 
        ${MSHM_SOURCE_FILES}
        ${MSHM_HEADER_FILES}
)

target_include_directories(${MSHM_LIB_NAME}
    PUBLIC
        ${MSHM_INCLUDE_DIR}
    PRIVATE 
        ${MSHM_SOURCE_DIR}
)

if(UNIX)
    target_link_libraries(${MSHM_LIB_NAME}
        PRIVATE
            -lrt
            -lpthread
    )
endif()

set_target_properties(${MSHM_LIB_NAME} PROPERTIES
    OUTPUT_NAME		${_OUTPUT_NAME}
    DEBUG_POSTFIX		"D"
    PUBLIC_HEADER    ${MSHM_HEADER_FILES}
)

install(TARGETS ${MSHM_LIB_NAME})


if(BUILD_TESTS)
	add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

