#!/bin/sh

# CLeans up all of the intermediate and temporary files left around by CMake.

make clean

(rm -r \
cmake_install.cmake \
CMakeCache.txt \
CMakeFiles \
CTestTestfile.cmake \
Makefile \
Testing) 2>/dev/null

(cd bin &&
rm -r \
cmake_install.cmake \
CMakeFiles \
CTestTestfile.cmake \
Makefile) 2>/dev/null

(cd src &&
rm -r \
cmake_install.cmake \
CMakeFiles \
CTestTestfile.cmake \
Makefile) 2>/dev/null

(cd test &&
rm -r \
cmake_install.cmake \
CMakeFiles \
CTestTestfile.cmake \
Makefile \
minimal_odb.archive.dat \
minimal_odb.archive.ind) 2>/dev/null
