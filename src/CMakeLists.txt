set(LIBS ${LIBS} odb pthread)

include_directories(include)

add_library(odb SHARED 
            datastore.cpp 
            index.cpp 
            odb.cpp 
            linkedlistds.cpp 
            linkedlisti.cpp 
            bankds.cpp 
            redblacktreei.cpp 
            archive.cpp 
            iterator.cpp
            scheduler.cpp 
            lfqueue.cpp)

#add_library(pca STATIC pca.c)

add_executable (../bin/traceflow traceflow.cpp)
add_executable (../bin/demo-flows demo-flows.cpp)
add_executable (../bin/demo-dns demo-dns.cpp)
add_executable (../bin/demo-1 demo-1.cpp)
add_executable (../bin/dnsstats dnsstats.cpp)
add_executable (../bin/ipstats ipstats.cpp)
add_executable (../bin/dpep pca.c demo-pack-ent-pca.cpp)
add_executable (../bin/trace-proc trace-proc.cpp)

target_link_libraries(../bin/demo-flows z ft ${LIBS})
target_link_libraries(../bin/demo-dns ${LIBS})
target_link_libraries(../bin/demo-1 ${LIBS})
target_link_libraries(../bin/dpep ${LIBS})
# target_link_libraries(pca_test m)
if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    target_link_libraries(../bin/traceflow ${LIBS} nsl socket)
    target_link_libraries(../bin/trace-proc ${LIBS} nsl)
    target_link_libraries(../bin/dnsstats ${LIBS} pcap socket nsl) # http://www.opensolaris.org/jive/thread.jspa?threadID=24384&tstart=0#94409
    target_link_libraries(../bin/ipstats ${LIBS} pcap socket nsl)
else(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    target_link_libraries(../bin/traceflow ${LIBS})
    target_link_libraries(../bin/trace-proc ${LIBS})
    target_link_libraries(../bin/dnsstats ${LIBS} pcap)
    target_link_libraries(../bin/ipstats ${LIBS} pcap)
endif(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")

#add_executable (pca_test pca.c)
#add_executable (py-demo py-demo.cpp)
