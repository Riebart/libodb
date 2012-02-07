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

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

add_executable(minimal-erbt minimal-erbt.cpp)
add_executable(minimal-odb minimal-odb.cpp)

add_executable(comp-index_datastore comp-index_datastore.cpp)
add_executable(comp-scheduler comp-scheduler.cpp)

add_executable(unit-collator unit-collator.cpp)
add_executable(unit-protoparse unit-protoparse.cpp)

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

target_link_libraries(minimal-erbt ${LIBS})
target_link_libraries(minimal-odb ${LIBS})

target_link_libraries(comp-index_datastore ${LIBS})
target_link_libraries(comp-scheduler ${LIBS})

target_link_libraries(unit-collator ${LIBS})
target_link_libraries(unit-protoparse ${LIBS})

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

add_dependencies(checks minimal-erbt)
add_dependencies(checks minimal-odb)

add_dependencies(checks comp-index_datastore)
add_dependencies(checks comp-scheduler)

add_dependencies(checks unit-collator)
add_dependencies(checks unit-protoparse)

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

add_test(minimal-erbt test-output "392b78f9c2f4e0d6677bcf8575b3d24f89c39aea9ff30994526d36dd33eb2048" "" "./minimal-erbt")
add_test(minimal-odb  test-output "88da74e6623391aea0a8c8cfecbdffbbcd5abf94df9028237f35247cf5ede862" "" "./minimal-odb")

add_test(comp-scheduler.unsched_single comp-scheduler 0)
add_test(comp-scheduler.sched_sync comp-scheduler 1)
add_test(comp-scheduler.sched_defer comp-scheduler 2)
add_test(comp-scheduler.unsched_single_odb comp-scheduler 3)
add_test(comp-scheduler.sched_sync_odb comp-scheduler 4)

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

add_test(unit-collator.0  test-output "" "" "./unit-collator" "0")
add_test(unit-collator.1  test-output "" "" "./unit-collator" "1")
add_test(unit-collator.2  test-output "d7d551d92d81264dbb9a11ca61f31c7172ad82a2536d0ca1cc5367e77122934b" "" "./unit-collator" "2")
add_test(unit-collator.3  test-output "d7d551d92d81264dbb9a11ca61f31c7172ad82a2536d0ca1cc5367e77122934b" "" "./unit-collator" "3")
add_test(unit-collator.4  test-output "d7d551d92d81264dbb9a11ca61f31c7172ad82a2536d0ca1cc5367e77122934b" "" "./unit-collator" "4")
add_test(unit-collator.5  test-output "d7d551d92d81264dbb9a11ca61f31c7172ad82a2536d0ca1cc5367e77122934b" "" "./unit-collator" "5")
add_test(unit-collator.6  test-output "d7d551d92d81264dbb9a11ca61f31c7172ad82a2536d0ca1cc5367e77122934b" "" "./unit-collator" "6")
add_test(unit-collator.7  test-output "d7d551d92d81264dbb9a11ca61f31c7172ad82a2536d0ca1cc5367e77122934b" "" "./unit-collator" "7")
add_test(unit-collator.8  test-output "d7d551d92d81264dbb9a11ca61f31c7172ad82a2536d0ca1cc5367e77122934b" "" "./unit-collator" "8")
add_test(unit-collator.9  test-output "d7d551d92d81264dbb9a11ca61f31c7172ad82a2536d0ca1cc5367e77122934b" "" "./unit-collator" "9")
add_test(unit-collator.10 test-output "" "" "./unit-collator" "10")
add_test(unit-collator.11 test-output "" "" "./unit-collator" "11")
add_test(unit-collator.12 test-output "" "" "./unit-collator" "12")
add_test(unit-collator.13 test-output "" "" "./unit-collator" "13")
add_test(unit-collator.14 test-output "" "" "./unit-collator" "14")

add_test(unit-protoparse.tcp_cksum unit-protoparse 0)

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test")
file(COPY test-output DESTINATION "${CMAKE_BINARY_DIR}/test/")

