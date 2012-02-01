set(LIBS ${LIBS} odb pthread)

include_directories("../src/include")

add_executable(minimal-erbt minimal-erbt.cpp)
add_executable(minimal-odb minimal-odb.cpp)
add_executable(test-ind_ds test-ind_ds.cpp)
add_executable(schedtest schedtest.cpp)

target_link_libraries(minimal-erbt ${LIBS})
target_link_libraries(minimal-odb ${LIBS})
target_link_libraries(test-ind_ds ${LIBS})
target_link_libraries(schedtest ${LIBS})

add_dependencies(check minimal-erbt)
add_dependencies(check minimal-odb)
add_dependencies(check test-ind_ds)
add_dependencies(check schedtest)

add_test(erbt minimal-erbt)
add_test(odb minimal-odb)
add_test(schedulter schedtest)
add_test(index-odb verify.sh)
