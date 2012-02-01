#!/bin/sh

# CLeans up all of the intermediate and temporary files left around by CMake.

make clean
rm -r bin

(rm -r \
cmake_install.cmake \
CMakeCache.txt \
CMakeFiles \
CTestTestfile.cmake \
Makefile \
Testing)

(cd src &&
rm -r \
bin \
cmake_install.cmake \
CMakeFiles \
CTestTestfile.cmake \
Makefile)

(cd test &&
rm -r \
cmake_install.cmake \
CMakeFiles \
CTestTestfile.cmake \
Makefile \
minimal_odb.archive.dat \
minimal_odb.archive.ind)

