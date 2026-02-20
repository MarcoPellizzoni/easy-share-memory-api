cmake_minimum_required( VERSION 3.22 )

set( CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE STRING "Install directory")

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" )

project( SHM_API VERSION 1.0.0)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(MSVC)
    add_compile_options( "/EHsc" ) # for exceptions
	add_compile_options("/W1")
    add_compile_definitions( "WIN32" )
endif()

if( UNIX )
	add_compile_definitions( "UNIX" )
    set( CMAKE_POSITION_INDEPENDENT_CODE ON )
endif()

option( BUILD_SHARED_LIBS "Build shared libraries" ON )
option( BUILD_TESTS "Build tests" ON)
option( BUILD_EXAMPLES "Build examples" OFF)

set(MSHM_LIB_NAME mShm)
add_subdirectory( mShm )


