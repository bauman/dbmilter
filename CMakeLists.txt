cmake_minimum_required(VERSION 3.3)
project(dbmilter)

INCLUDE_DIRECTORIES(/usr/include/ /usr/include/libbson-1.0/ )
LINK_DIRECTORIES(/usr/lib64/ /lib64/)

set(SOURCE_FILES
        dbmilter.c
        dbmilter_serializer.c
        dbmilter_cclient.c
        dbmilter_writer.c)


add_executable(dbmilter ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(dbmilter bson-1.0 )
TARGET_LINK_LIBRARIES(dbmilter czmq )
TARGET_LINK_LIBRARIES(dbmilter zmq )
TARGET_LINK_LIBRARIES(dbmilter bsoncompare )
TARGET_LINK_LIBRARIES(dbmilter milter)


enable_testing()


function(dbmilter add_test test_name)
    add_executable(${test_name} ${ARGN})
endfunction()