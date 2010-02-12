cmake_minimum_required(VERSION 2.6)

# The name of our project is "TM-PROJECT".  CMakeLists files in this project can
# refer to the root source directory of the project as ${TM-PROJECT_SOURCE_DIR} and
# to the root binary directory of the project as ${TM-PROJECT_BINARY_DIR}.)
project (TM-PROJECT)

#set(CMAKE_C_COMPILER "gcc")
#set(CMAKE_CXX_COMPILER "g++")

#set(CMAKE_CXX_FLAGS "-Wall -O0 -g")
#set(CMAKE_CXX_FLAGS "-Wall -O1 -pg -g -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -fopenmp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

#this is probably the wrong way to do this: setting compile-time defines
add_definitions(-DPTHREAD_LOCK_SPIN)

# Recurse into the "Hello" and "Demo" subdirectories.  This does not actually
# cause another cmake executable to run.  The same process will walk through
# the project's entire directory structure.
#add_subdirectory (src)
#add_subdirectory (Demo)

# Make sure the compiler can find include files from our Hello library.
include_directories (${TM-PROJECT_SOURCE_DIR}/include)

add_library(odb SHARED datastore.cpp index.cpp odb.cpp linkedlistds.cpp linkedlisti.cpp bankds.cpp redblacktreei.cpp iterator.cpp)

# Make sure the linker can find the Hello library once it is built.
#link_directories (${HELLO_BINARY_DIR}/Hello)


# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx".  The extensions are automatically found.
#add_executable (helloDemo demo.cxx demo_b.cxx)
add_executable (test test.cpp)
add_executable (minimal minimal.cpp)
add_executable (demo demo.cpp)

#link_directories(/usr/local/lib)
target_link_libraries(minimal tcmalloc_minimal pthread odb)
target_link_libraries(test tcmalloc_minimal pthread odb)
target_link_libraries(demo z ft tcmalloc_minimal odb)

#install directives
install(TARGETS odb LIBRARY DESTINATION lib)
install(FILES include/odb.hpp include/index.hpp DESTINATION include)

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
