# MPL2.0 HEADER START
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# MPL2.0 HEADER END
#
# Copyright 2010-2012 Michael Himbeault and Travis Friesen
#

cmake_minimum_required(VERSION 2.6)

enable_testing()

# The name of our project is "LIBODB".  CMakeLists files in this project can
# refer to the root source directory of the project as ${LIBODB_SOURCE_DIR} and
# to the root binary directory of the project as ${LIBODB_BINARY_DIR}.)
project(LIBODB)

message(STATUS "Operating System detected as ${CMAKE_SYSTEM_NAME}")

#set(CMAKE_LINKER "/bin/ld")
set(COMPILER_C)
set(COMPILER_CXX)
set(SKIP_SUN_COMPILER "false")
set(FORCE_64_BIT "true")

if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    add_definitions(-DSYSTEM_NAME_SUNOS)
endif(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")

if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS" AND ${SKIP_SUN_COMPILER} MATCHES "false")
    message(STATUS "Attempting to set compiler to Sun Studio on Solaris")
    find_program(COMPILER_C NAMES suncc)
    find_program(COMPILER_CXX NAMES sunCC)
    message(STATUS " -> Found ${COMPILER_C} and ${COMPILER_CXX}")

    if(${COMPILER_C} MATCHES "none" OR ${COMPILER_CXX} MATCHES "none")
	set(COMPILER_SUITE "none")
	set(COMPILER_C "none")
	set(COMPILER_CXX "none")
	message(STATUS "Sun Studio failed")
    else(${COMPILER_C} MATCHES "none" OR ${COMPILER_CXX} MATCHES "none")
	set(CMAKE_C_COMPILER "suncc")
	set(CMAKE_CXX_COMPILER "sunCC")
	set(COMPILER_SUITE "sun")
	add_definitions(-DCMAKE_COMPILER_SUITE_SUN)
    endif(${COMPILER_C} MATCHES "none" OR ${COMPILER_CXX} MATCHES "none")
else(${CMAKE_SYSTEM_NAME} MATCHES "SunOS" AND ${SKIP_SUN_COMPILER} MATCHES "false")
    message(STATUS "Skipping search for Sun Studio compiler on Solaris")
    set(COMPILER_SUITE "none")
endif(${CMAKE_SYSTEM_NAME} MATCHES "SunOS" AND ${SKIP_SUN_COMPILER} MATCHES "false")

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" OR ${COMPILER_SUITE} MATCHES "none")
    message(STATUS "Attempting to set compiler to GCC")
    set(COMPILER_C)
    set(COMPILER_CXX)
    find_program(COMPILER_C NAMES gcc)
    find_program(COMPILER_CXX NAMES g++)
    message(STATUS " -> Found ${COMPILER_C} and ${COMPILER_CXX}")
    
    if(${COMPILER_C} MATCHES "none" OR ${COMPILER_CXX} MATCHES "none")
	set(COMPILER_SUITE "none")
	set(COMPILER_C "none")
	set(COMPILER_CXX "none")
	message(STATUS "GCC failed")
    else(${COMPILER_C} MATCHES "none" OR ${COMPILER_CXX} MATCHES "none")
	set(CMAKE_C_COMPILER "gcc")
	set(CMAKE_CXX_COMPILER "g++")
	set(COMPILER_SUITE "gcc")
	add_definitions(-DCMAKE_COMPILER_SUITE_GCC)
    endif(${COMPILER_C} MATCHES "none" OR ${COMPILER_CXX} MATCHES "none")
endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" OR ${COMPILER_SUITE} MATCHES "none")

if(${COMPILER_SUITE} MATCHES "none")
    message(FATAL_ERROR "Unable to find a compiler collection (Sun Studio or GCC)")
    return()
endif(${COMPILER_SUITE} MATCHES "none")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
# See http://en.wikipedia.org/wiki/Tcphdr : The BSD-style headers are more portable than the Linux style.
# Need _BSD_SOURCE because tcp.h includes features.h which conditions on _BSD_SOURCE and sets __FAVOR_BSD from there.
    message(STATUS "Ensuring use of BSD-style headers on Linux")
    add_definitions(-D_BSD_SOURCE -U_POSIX_SOURCE -U_POSIX_C_SOURCE -U_XOPEN_SOURCE -U_GNU_SOURCE -U_SVID_SOURCE)
    add_definitions(-DSYSTEM_NAME_LINUX)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

set(CMAKE_FLAGS_X64 "")
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "CYGWIN" AND ${FORCE_64_BIT} MATCHES "true")
    message(STATUS "Forcing 64-bit compilation")
    set(CMAKE_FLAGS_X64 "-m64")
    add_definitions(-DSYSTEM_NAME_CYGWIN)
endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "CYGWIN" AND ${FORCE_64_BIT} MATCHES "true")

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND ${COMPILER_SUITE} MATCHES "gcc")
    list(APPEND CMAKE_C_FLAGS "-march=native -mtune=native") # These options don't exist for Sun's compilers
    add_definitions(-DSYSTEM_NAME_DARWIN)
endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND ${COMPILER_SUITE} MATCHES "gcc")

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    find_library(TCMALLOC_LIB tcmalloc_minimal)
    if(${TCMALLOC_LIB} MATCHES "tcmalloc")
        message(STATUS "Found tcmalloc as ${TCMALLOC_LIB}")
         list(APPEND LIBS ${TCMALLOC_LIB}) # Reference: http://www.cmake.org/Wiki/CMake_Performance_Tips
         list(APPEND CMAKE_C_FLAGS "-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
    endif(${TCMALLOC_LIB} MATCHES "tcmalloc")
endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "SunOS")

# set(CMAKE_C_FLAGS "${CMAKE_FLAGS_X64} -Wall -O0 -g -fno-inline")
#set(CMAKE_C_FLAGS "${CMAKE_FLAGS_X64} -Wall -O1 -pg -g -fprofile-arcs -ftest-coverage")
if(NOT ${COMPILER_SUITE} MATCHES "sun")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_FLAGS_X64} -Wall -g -O3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} ${CMAKE_FLAGS_X64}")
else(NOT ${COMPILER_SUITE} MATCHES "sun")
# http://dsc.sun.com/sunstudio/documentation/product/compiler.jsp for mor information on flags.
    set(CMAKE_SHARED_LIBRARY_C_FLAGS "-KPIC") # Make sure we don't use fPIC on Sun compilers.
    set(CMAKE_SHARED_LIBRARY_CXX_FLAGS "-KPIC") # Make sure we don't use fPIC on Sun compilers.
    set(CMAKE_USER_MAKE_RULES_OVERRIDE_C ":")
    set(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG ":")
    set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ":")
    set(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG ";")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_FLAGS_X64} -g -xO5 -fast")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_FLAGS_X64} -g0 -xO5 -fast -library=stlport4")
endif(NOT ${COMPILER_SUITE} MATCHES "sun")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_FLAGS_X64}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_FLAGS_X64}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_FLAGS_X64}")

#this is probably the wrong way to do this: setting compile-time defines
add_definitions(-DENABLE_PTHREAD_LOCKS)
add_definitions(-DPTHREAD_SIMPLE_LOCKS)

# Create some directories. Build fails if the directories don't already exist.
#   This is because a compiler won't write a file to a directory if the directory doesn't exit.
# file(MAKE_DIRECTORY "${LIBODB_BINARY_DIR}")

add_subdirectory("src")

add_custom_target(extra COMMAND "")
add_subdirectory("extra" EXCLUDE_FROM_ALL)

set(CMAKE_CTEST_COMMAND ctest)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
add_custom_target(checks)
add_dependencies(check checks)
add_subdirectory("test" EXCLUDE_FROM_ALL)

add_custom_target(doc COMMAND "doxygen doc/libodb-doc.cfg")

#target_link_libraries(py-demo boost_python-mt python2.6 ${LIBS})

# #install directives
# install(TARGETS odb LIBRARY DESTINATION lib)
# 
# install(FILES ${LIBODB_INCLUDE_SOURCE_DIR}/odb.hpp 
#               ${LIBODB_INCLUDE_SOURCE_DIR}/index.hpp 
#               ${LIBODB_INCLUDE_SOURCE_DIR}/iterator.hpp 
#               ${LIBODB_INCLUDE_SOURCE_DIR}/comparator.hpp 
#               ${LIBODB_INCLUDE_SOURCE_DIR}/archive.hpp 
#               ${LIBODB_INCLUDE_SOURCE_DIR}/lock.hpp 
#               ${LIBODB_INCLUDE_SOURCE_DIR}/scheduler.hpp 
#               ${LIBODB_INCLUDE_SOURCE_DIR}/lfqueue.hpp 
#               DESTINATION include)

# #Package generation directives
# SET(CPACK_GENERATOR "TGZ;DEB;RPM")
# SET(CPACK_PACKAGE_NAME "tm-odb")
# #SET(CPACK_PACKAGE_VERSION "0.1.1")
# SET(CPACK_PACKAGE_VERSION_MAJOR "0")
# SET(CPAGK_PACKAGE_VERSION_MINOR "1")
# SET(CPACK_PACKAGE_VERSION_PATCH "1")
# SET(CPACK_PACKAGE_CONTACT "travis_friesen@ieee.org")
# SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A fast and lightweight datastore")
# 
# #for the DEB package
# SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
# SET(CPACK_DEBIAN_PACKAGE_DEPENDS "")
# 
# #for RPM
# SET(CPACK_RPM_PACKAGE_RELEASE "1")
# 
# #build the packages
# INCLUDE(CPack)
