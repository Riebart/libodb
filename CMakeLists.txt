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

# FILE doesn't have COPY operation before 2.8 I think. Might be 2.7, but 2.6 is
# definitely missing it.
cmake_minimum_required(VERSION 2.8)

enable_testing()

# The name of our project is "LIBODB".  CMakeLists files in this project can
# refer to the root source directory of the project as ${LIBODB_SOURCE_DIR} and
# to the root binary directory of the project as ${LIBODB_BINARY_DIR}.)
project(LIBODB)

message(STATUS "Operating System detected as ${CMAKE_SYSTEM_NAME}")

#set(CMAKE_LINKER "/bin/ld")
set(COMPILER_C)
set(COMPILER_CXX)

# Environment variables that can be specified that control the configuration
# parameters
#   SKIP_SUN_COMPILER
#   NO_FORCE_64_BIT
#   NO_OPTIMIZE

if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    add_definitions(-DSYSTEM_NAME_SUNOS)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS" AND "$ENV{SKIP_SUN_COMPILER}" STREQUAL "")
    message(STATUS "Attempting to set compiler to Sun Studio on Solaris")
    find_program(COMPILER_C NAMES suncc)
    find_program(COMPILER_CXX NAMES sunCC)
    message(STATUS " -> Found ${COMPILER_C} and ${COMPILER_CXX}")

    if(${COMPILER_C} MATCHES "none" OR ${COMPILER_CXX} MATCHES "none")
	set(COMPILER_SUITE "none")
	set(COMPILER_C "none")
	set(COMPILER_CXX "none")
	message(STATUS "Sun Studio failed")
    else()
	set(CMAKE_C_COMPILER "suncc")
	set(CMAKE_CXX_COMPILER "sunCC")
	set(COMPILER_SUITE "sun")
	add_definitions(-DCMAKE_COMPILER_SUITE_SUN)
    endif()
else()
    message(STATUS "Skipping search for Sun Studio compiler on Solaris")
    set(COMPILER_SUITE "none")
endif()

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
    else()
	set(CMAKE_C_COMPILER "gcc")
	set(CMAKE_CXX_COMPILER "g++")
	set(COMPILER_SUITE "gcc")
	add_definitions(-DCMAKE_COMPILER_SUITE_GCC)
    endif()
endif()

if(${COMPILER_SUITE} MATCHES "none")
    message(FATAL_ERROR "Unable to find a compiler collection (Sun Studio or GCC)")
    return()
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
# See http://en.wikipedia.org/wiki/Tcphdr : The BSD-style headers are more portable than the Linux style.
# Need _BSD_SOURCE because tcp.h includes features.h which conditions on _BSD_SOURCE and sets __FAVOR_BSD from there.
    message(STATUS "Ensuring use of BSD-style headers on Linux")
    # _GNU_SOURCE is required in some situations, so I've removed that un-define
    # even though if ti is defined that will prevent __FAVOR_BSD from getting
    # set in features.h which enables the BSD-style TCP header. I'm handling
    # that by branching on SYSTEM_NAME_LINUX in protoparse.hpp and defining a
    # tcphdr_t type to either be the BSD-style header, or a newly defined
    # struct that mirror the BSD-style struct.
    add_definitions(-D_BSD_SOURCE -U_POSIX_SOURCE -U_POSIX_C_SOURCE -U_XOPEN_SOURCE -U_SVID_SOURCE)
    add_definitions(-DSYSTEM_NAME_LINUX)
endif()

set(CMAKE_FLAGS_X64 "")
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "CYGWIN" AND "$ENV{NO_FORCE_64_BIT}" STREQUAL "")
    message(STATUS "Forcing 64-bit compilation")
    set(CMAKE_FLAGS_X64 "-m64")
    add_definitions(-DSYSTEM_NAME_CYGWIN)
endif()

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND ${COMPILER_SUITE} MATCHES "gcc")
    list(APPEND CMAKE_C_FLAGS "-march=native -mtune=native") # These options don't exist for Sun's compilers
else()
    add_definitions(-DSYSTEM_NAME_DARWIN)
endif()

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" AND "$ENV{NO_OPTIMIZE}" STREQUAL "" AND "$ENV{SKIP_TCMALLOC}" STREQUAL "")
    find_library(TCMALLOC_LIB tcmalloc_minimal)
    if(${TCMALLOC_LIB} MATCHES "tcmalloc")
        message(STATUS "Found tcmalloc as ${TCMALLOC_LIB}")
        list(APPEND LIBS ${TCMALLOC_LIB}) # Reference: http://www.cmake.org/Wiki/CMake_Performance_Tips
#        list(APPEND CMAKE_C_FLAGS "-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
    endif()
endif()

# set(CMAKE_C_FLAGS "${CMAKE_FLAGS_X64} -Wall -O0 -g -fno-inline")
#set(CMAKE_C_FLAGS "${CMAKE_FLAGS_X64} -Wall -O1 -pg -g -fprofile-arcs -ftest-coverage")
if(NOT ${COMPILER_SUITE} MATCHES "sun")
    if("$ENV{NO_OPTIMIZE}" STREQUAL "")
        message(STATUS "Enabling optimizations.")
        set(CMAKE_C_OPTIMIZE_FLAGS "-g -O3")
    else()
        message(STATUS "Disabling all optimizations.")
        set(CMAKE_C_OPTIMIZE_FLAGS "-g -O0 -fno-inline")
    endif()
    
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_FLAGS_X64} -Wall ${CMAKE_C_OPTIMIZE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} ${CMAKE_FLAGS_X64}")
else()
# http://dsc.sun.com/sunstudio/documentation/product/compiler.jsp for mor information on flags.
    set(CMAKE_SHARED_LIBRARY_C_FLAGS "-KPIC") # Make sure we don't use fPIC on Sun compilers.
    set(CMAKE_SHARED_LIBRARY_CXX_FLAGS "-KPIC") # Make sure we don't use fPIC on Sun compilers.
    set(CMAKE_USER_MAKE_RULES_OVERRIDE_C ":")
    set(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG ":")
    set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ":")
    set(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG ";")
    
    if("$ENV{NO_OPTIMIZE}" STREQUAL "")
        message(STATUS "Enabling optimizations.")
        set(CMAKE_C_OPTIMIZE_FLAGS "-g -xO5 -fast")
        set(CMAKE_CXX_OPTIMIZE_FLAGS "-g0 -xO5 -fast")
    else()
        message(STATUS "Disabling all optimizations.")
        set(CMAKE_C_OPTIMIZE_FLAGS "-g -xO0")
        set(CMAKE_CXX_OPTIMIZE_FLAGS "-g -xO0")
    endif()
    
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_FLAGS_X64} ${CMAKE_C_OPTIMIZE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_FLAGS_X64} ${CMAKE_CXX_OPTIMIZE_FLAGS} -library=stlport4")
endif()

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

# Look for cccc, pdflatex and doxygen and build configuration files when we find
# them.
add_custom_target(doc)
set(${CMAKE_DOC_BUILD_COMMAND} "")
set(${CMAKE_SOURCE_FILES} "")
execute_process(COMMAND ${LIBODB_SOURCE_DIR}/scripts/files.sh WORKING_DIRECTORY ${LIBODB_SOURCE_DIR} OUTPUT_VARIABLE CMAKE_SOURCE_FILES)
file(MAKE_DIRECTORY ${LIBODB_BINARY_DIR}/doc)
file(MAKE_DIRECTORY ${LIBODB_BINARY_DIR}/doc/latex)

find_program(DOXYGEN_BINARY NAMES doxygen)
if(${DOXYGEN_BINARY} MATCHES "doxygen")
    message(STATUS "Doxygen found at ${DOXYGEN_BINARY}")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/libodb-doc.cfg.in ${LIBODB_BINARY_DIR}/doc/libodb-doc.cfg @ONLY)
    add_custom_command(TARGET doc POST_BUILD
                   COMMAND ${DOXYGEN_BINARY} ${LIBODB_BINARY_DIR}/doc/libodb-doc.cfg
                   WORKING_DIRECTORY ${LIBODB_BINARY_DIR}
                   VERBATIM)
    
    find_program(PDFLATEX_BINARY NAMES pdflatex)
    if(${PDFLATEX_BINARY} MATCHES "pdflatex")
        message(STATUS "PDFLaTeX found at ${PDFLATEX_BINARY}")
        add_custom_command(TARGET doc POST_BUILD
                           COMMAND make
                           WORKING_DIRECTORY ${LIBODB_BINARY_DIR}/doc/latex/
                           VERBATIM)
    else()
        message(STATUS "No pdflatex was found in your path. Get it at http://www.tug.org/texlive/")
    endif()
else()
    message(STATUS "No doxygen was found in your path. Get it at http://www.stack.nl/~dimitri/doxygen/")
    message(STATUS " -> Skipping check for pdflatex.")
endif()

find_program(CCCC_BINARY NAMES cccc)
if(${CCCC_BINARY} MATCHES "cccc")
    message(STATUS "CCCC found at ${CCCC_BINARY}")
    string(REPLACE " " ";" CMAKE_SOURCE_FILES ${CMAKE_SOURCE_FILES})
    add_custom_command(TARGET doc POST_BUILD
                       COMMAND ${CCCC_BINARY} --outdir=${LIBODB_BINARY_DIR}/doc/metrics ${CMAKE_SOURCE_FILES}
                       WORKING_DIRECTORY ${LIBODB_BINARY_DIR}/doc/latex/
                       VERBATIM)
else()
    message(STATUS "No cccc was found in your path. Get it at http://cccc.sourceforge.net/")
endif()

add_custom_target(everything COMMAND make checks extra doc check)

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
