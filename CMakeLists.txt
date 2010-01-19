# The name of our project is "TM-PROJECT".  CMakeLists files in this project can
# refer to the root source directory of the project as ${TM-PROJECT_SOURCE_DIR} and
# to the root binary directory of the project as ${TM-PROJECT_BINARY_DIR}.)
project (TM-PROJECT)

#set(CMAKE_C_COMPILER "gcc-3.4")
#set(CMAKE_CXX_COMPILER "g++-3.4")

#set(CMAKE_CXX_FLAGS "-Wall -O0 -g")
#set(CMAKE_CXX_FLAGS "-Wall -O1 -pg -g -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

#this is probably the wrong way to do this: setting compile-time defines
add_definitions(-DBANK)

# Recurse into the "Hello" and "Demo" subdirectories.  This does not actually
# cause another cmake executable to run.  The same process will walk through
# the project's entire directory structure.
#add_subdirectory (src)
#add_subdirectory (Demo)

# Make sure the compiler can find include files from our Hello library.
include_directories (${TM-PROJECT_SOURCE_DIR}/include)

# Make sure the linker can find the Hello library once it is built.
#link_directories (${HELLO_BINARY_DIR}/Hello)

# Link the executable to the Hello library.
#target_link_libraries (helloDemo Hello)

#target_link_libraries(pthread state c profiler)

#link_directories(/usr/local/lib)
target_link_libraries(tcmalloc_minimal)


# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx".  The extensions are automatically found.
#add_executable (helloDemo demo.cxx demo_b.cxx)
add_executable (test test.cpp)
add_executable (indextest indextest.cpp)
