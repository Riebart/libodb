/*
 * for h in odb.hpp index.hpp iterator.hpp comparator.hpp archive.hpp
 * do
 *     cat "../../src/include/$h" | grep -v "\"dll.hpp\"" | sed 's/LIBODB_API //' > odb_include/$h
 * done
 *
 * - First build the odb_static target in the build directory.
 * swig -python -c++ libodb.i
 * g++ -O3 -fpic -I/usr/include/python2.7/ -shared -o _odb_wrap.so odb_wrap.cxx funcptr.cpp ../../build/src/libodb_static.a
 */

%module libodb_wrap
%{
/* Includes the header in the wrapper code */
#include "odb_include/odb.hpp"
#include "odb_include/archive.hpp"
#include "odb_include/index.hpp"
#include "odb_include/iterator.hpp"

#include "funcptr.hpp"
%}

/* Parse the header file to generate wrappers */
%ignore libodb::odb_sched_workload;
%ignore libodb::ig_sched_workload;
%ignore libodb::mem_checker;
%ignore update_time;
%ignore get_time;
%include "odb_include/odb.hpp"
%include "odb_include/archive.hpp"
%ignore libodb::add_data_v_wrapper;
%include "odb_include/index.hpp"
%include "odb_include/iterator.hpp"

%include "funcptr.hpp"
