cmake_minimum_required(VERSION 2.6)

# The name of our project is "TM-PROJECT".  CMakeLists files in this project can
# refer to the root source directory of the project as ${TM-PROJECT_SOURCE_DIR} and
# to the root binary directory of the project as ${TM-PROJECT_BINARY_DIR}.)
project (TM-PROJECT)

message(STATUS "Operating System detected as ${CMAKE_SYSTEM_NAME}")

#set(CMAKE_LINKER "/bin/ld")
set(COMPILER_C)
set(COMPILER_CXX)
set(SKIP_SUN_COMPILER "true")

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
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "CYGWIN")
    message(STATUS "Forcing 64-bit compilation")
    set(CMAKE_FLAGS_X64 "-m64")
    add_definitions(-DSYSTEM_NAME_CYGWIN)
endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "CYGWIN")

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND ${COMPILER_SUITE} MATCHES "gcc")
    list(APPEND CMAKE_C_FLAGS "-march=native -mtune=native") # These options don't exist for Sun's compilers
    add_definitions(-DSYSTEM_NAME_DARWIN)
endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND ${COMPILER_SUITE} MATCHES "gcc")

#this is probably the wrong way to do this: setting compile-time defines
add_definitions(-DENABLE_PTHREAD_LOCKS)
add_definitions(-DPTHREAD_RW_LOCKS)

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

set(LIBS ${LIBS} odb pthread)

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    find_library(TCMALLOC_LIB tcmalloc_minimal)
    if(${TCMALLOC_LIB} MATCHES "tcmalloc")
	message(STATUS "Found tcmalloc as ${TCMALLOC_LIB}")
#	list(APPEND LIBS ${TCMALLOC_LIB}) # Reference: http://www.cmake.org/Wiki/CMake_Performance_Tips
#	list(APPEND CMAKE_C_FLAGS "-fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
    endif(${TCMALLOC_LIB} MATCHES "tcmalloc")
endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "SunOS")

# Recurse into the "Hello" and "Demo" subdirectories.  This does not actually
# cause another cmake executable to run.  The same process will walk through
# the project's entire directory structure.
#add_subdirectory (src)
#add_subdirectory (Demo)

# Make sure the compiler can find include files from our Hello library.
include_directories (${TM-PROJECT_SOURCE_DIR}/include ${TM-PROJECT_SOURCE_DIR}/include/spinlock /usr/include/python2.6 /usr/src/linux/arch/x86/include /usr/src/linux/include)

add_library(odb SHARED datastore.cpp index.cpp odb.cpp linkedlistds.cpp linkedlisti.cpp bankds.cpp redblacktreei.cpp archive.cpp iterator.cpp scheduler.cpp lfqueue.cpp)
#add_library(pca STATIC pca.c)

# Make sure the linker can find the Hello library once it is built.
#link_directories (${HELLO_BINARY_DIR}/Hello)

# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx".  The extensions are automatically found.
#add_executable (helloDemo demo.cxx demo_b.cxx)
add_executable (test test.cpp)
add_executable (schedtest schedtest.cpp)
add_executable (minimal_erbt minimal_erbt.cpp)
add_executable (minimal_odb minimal_odb.cpp)
add_executable (demo-flows demo-flows.cpp)
add_executable (demo-dns demo-dns.cpp)
add_executable (demo-1 demo-1.cpp)
add_executable (dnsstats dnsstats.cpp)
add_executable (ipstats ipstats.cpp)
add_executable (dpep pca.c demo-pack-ent-pca.cpp)
add_executable (trace-proc trace-proc.cpp)
#add_executable (pca_test pca.c)
#add_executable (py-demo py-demo.cpp)

target_link_libraries(test ${LIBS})
target_link_libraries(schedtest ${LIBS} -lrt)
target_link_libraries(minimal_erbt ${LIBS})
target_link_libraries(minimal_odb ${LIBS})
target_link_libraries(demo-flows z ft ${LIBS})
target_link_libraries(demo-dns ${LIBS})
target_link_libraries(demo-1 ${LIBS})
target_link_libraries(dpep ${LIBS})
# target_link_libraries(pca_test m)
if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    target_link_libraries(trace-proc ${LIBS} nsl)
    target_link_libraries(dnsstats ${LIBS} pcap socket nsl) # http://www.opensolaris.org/jive/thread.jspa?threadID=24384&tstart=0#94409
    target_link_libraries(ipstats ${LIBS} pcap socket nsl)
else(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    target_link_libraries(trace-proc ${LIBS})
    target_link_libraries(dnsstats ${LIBS} pcap)
    target_link_libraries(ipstats ${LIBS} pcap)
endif(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
#target_link_libraries(py-demo boost_python-mt python2.6 ${LIBS})

#install directives
install(TARGETS odb LIBRARY DESTINATION lib)
install(FILES include/odb.hpp include/index.hpp include/iterator.hpp include/comparator.hpp include/archive.hpp include/lock.hpp include/scheduler.hpp include/lfqueue.hpp DESTINATION include)

#Package generation directives
SET(CPACK_GENERATOR "TGZ;DEB;RPM")
SET(CPACK_PACKAGE_NAME "tm-odb")
#SET(CPACK_PACKAGE_VERSION "0.1.1")
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPAGK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_PACKAGE_VERSION_PATCH "1")
SET(CPACK_PACKAGE_CONTACT "travis_friesen@ieee.org")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A fast and lightweight datastore")

#for the DEB package
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "")

#for RPM
SET(CPACK_RPM_PACKAGE_RELEASE "1")

#build the packages
INCLUDE(CPack)
