set(LIBS ${LIBS} odb pthread)

include_directories("../src/include" "../extralib/include")

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

add_executable(minimal-erbt minimal-erbt.cpp)
add_executable(minimal-odb minimal-odb.cpp)

add_executable(comp-index_datastore comp-index_datastore.cpp)
add_executable(comp-scheduler comp-scheduler.cpp)

add_executable(unit-collator unit-collator.cpp ../extralib/collator.cpp)

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

target_link_libraries(minimal-erbt ${LIBS})
target_link_libraries(minimal-odb ${LIBS})

target_link_libraries(comp-index_datastore ${LIBS})
target_link_libraries(comp-scheduler ${LIBS})

target_link_libraries(unit-collator ${LIBS})

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

add_dependencies(checks minimal-erbt)
add_dependencies(checks minimal-odb)

add_dependencies(checks comp-index_datastore)
add_dependencies(checks comp-scheduler)

add_dependencies(checks unit-collator)

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

add_test(minimal-erbt test-output "392b78f9c2f4e0d6677bcf8575b3d24f89c39aea9ff30994526d36dd33eb2048" "" "./minimal-erbt")
add_test(minimal-odb  test-output "88da74e6623391aea0a8c8cfecbdffbbcd5abf94df9028237f35247cf5ede862" "" "./minimal-odb")

add_test(comp-scheduler.1 comp-scheduler 0)
add_test(comp-scheduler.2 comp-scheduler 1)
add_test(comp-scheduler.3 comp-scheduler 2)
add_test(comp-scheduler.4 comp-scheduler 3)
add_test(comp-scheduler.5 comp-scheduler 4)

add_test(comp-rbt.bank.none  test-output "" "43031308a1919ec69606810188462336ceab795a057a987a2ea3112898ff5c2c" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 0 -T 0")
add_test(comp-rbt.bank.drop  test-output "" "bbfd379491b29ba724f4a2163972bf827c83ff7bc14a5a49058fc39d23d658e0" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 1 -T 0")
add_test(comp-rbt.ll.none    test-output "" "43031308a1919ec69606810188462336ceab795a057a987a2ea3112898ff5c2c" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 0 -T 1")
add_test(comp-rbt.ll.drop    test-output "" "bbfd379491b29ba724f4a2163972bf827c83ff7bc14a5a49058fc39d23d658e0" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 1 -T 1")
add_test(comp-rbt.banki.none test-output "" "43031308a1919ec69606810188462336ceab795a057a987a2ea3112898ff5c2c" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 0 -T 2")
add_test(comp-rbt.banki.drop test-output "" "bbfd379491b29ba724f4a2163972bf827c83ff7bc14a5a49058fc39d23d658e0" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 1 -T 2")
add_test(comp-rbt.lli.none   test-output "" "43031308a1919ec69606810188462336ceab795a057a987a2ea3112898ff5c2c" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 0 -T 3")
add_test(comp-rbt.lli.drop   test-output "" "bbfd379491b29ba724f4a2163972bf827c83ff7bc14a5a49058fc39d23d658e0" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 1 -T 3")
add_test(comp-rbt.llv.none   test-output "" "5f95f2dd442d9ed865ff201f7e07b5a224cab688cdd506dc617f06616f8b9369" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 0 -T 4")
add_test(comp-rbt.llv.drop   test-output "" "6d81f80fa65fe4873024a1df6a066c1d633421b06a38c2b29c299800a3967fb7" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 1 -T 4")

add_test(comp-ll.bank.none  test-output "" "43031308a1919ec69606810188462336ceab795a057a987a2ea3112898ff5c2c" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 2 -T 0")
add_test(comp-ll.bank.drop  test-output "" "bbfd379491b29ba724f4a2163972bf827c83ff7bc14a5a49058fc39d23d658e0" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 3 -T 0")
add_test(comp-ll.ll.none    test-output "" "43031308a1919ec69606810188462336ceab795a057a987a2ea3112898ff5c2c" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 2 -T 1")
add_test(comp-ll.ll.drop    test-output "" "bbfd379491b29ba724f4a2163972bf827c83ff7bc14a5a49058fc39d23d658e0" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 3 -T 1")
add_test(comp-ll.banki.none test-output "" "43031308a1919ec69606810188462336ceab795a057a987a2ea3112898ff5c2c" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 2 -T 2")
add_test(comp-ll.banki.drop test-output "" "bbfd379491b29ba724f4a2163972bf827c83ff7bc14a5a49058fc39d23d658e0" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 3 -T 2")
add_test(comp-ll.lli.none   test-output "" "43031308a1919ec69606810188462336ceab795a057a987a2ea3112898ff5c2c" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 2 -T 3")
add_test(comp-ll.lli.drop   test-output "" "bbfd379491b29ba724f4a2163972bf827c83ff7bc14a5a49058fc39d23d658e0" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 3 -T 3")
add_test(comp-ll.llv.none   test-output "" "5f95f2dd442d9ed865ff201f7e07b5a224cab688cdd506dc617f06616f8b9369" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 2 -T 4")
add_test(comp-ll.llv.drop   test-output "" "6d81f80fa65fe4873024a1df6a066c1d633421b06a38c2b29c299800a3967fb7" "./comp-index_datastore" "-e 8 -n 100000 -t 10 -i 3 -T 4")

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test")
file(COPY test-output DESTINATION "${CMAKE_BINARY_DIR}/test/")

