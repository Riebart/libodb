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

set(LIBS ${LIBS} odb pthread)

include_directories("../src/include" "../extralib/include")

add_executable (traceflow traceflow.cpp)
add_executable (demo-flows demo-flows.cpp)
add_executable (demo-dns demo-dns.cpp)
add_executable (demo-1 demo-1.cpp)
add_executable (dnsstats dnsstats.cpp)
add_executable (ipstats ipstats.cpp)
add_executable (dpep pca.c demo-pack-ent-pca.cpp)
add_executable (trace-proc trace-proc.cpp)

target_link_libraries(demo-flows z ft ${LIBS})
target_link_libraries(demo-dns ${LIBS})
target_link_libraries(demo-1 ${LIBS})
target_link_libraries(dpep ${LIBS})
# target_link_libraries(pca_test m)

if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    target_link_libraries(traceflow ${LIBS} nsl socket pcap)
    target_link_libraries(trace-proc ${LIBS} nsl)
    target_link_libraries(dnsstats ${LIBS} pcap socket nsl) # http://www.opensolaris.org/jive/thread.jspa?threadID=24384&tstart=0#94409
    target_link_libraries(ipstats ${LIBS} pcap socket nsl)
else(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    target_link_libraries(traceflow ${LIBS})
    target_link_libraries(trace-proc ${LIBS})
    target_link_libraries(dnsstats ${LIBS} pcap)
    target_link_libraries(ipstats ${LIBS} pcap)
endif(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")

#add_executable (pca_test pca.c)
#add_executable (py-demo py-demo.cpp)


add_dependencies(extra traceflow)
add_dependencies(extra demo-flows)
add_dependencies(extra demo-dns)
add_dependencies(extra demo-1)
add_dependencies(extra dnsstats)
add_dependencies(extra ipstats)
add_dependencies(extra dpep)
add_dependencies(extra trace-proc)

