cmake_minimum_required(VERSION 2.8)
project(libReliableStream)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3")

set(SOURCE_FILES
        ReliableStream.c
        ReliableStream.h Checksum.c Packet.c IO.c Pipe.c)

add_library(ReliableStream SHARED ${SOURCE_FILES} tests/test.c)
add_library(ReliableStream_Static STATIC ${SOURCE_FILES})
set_target_properties(ReliableStream_Static PROPERTIES OUTPUT_NAME ReliableStream)

install(TARGETS ReliableStream ReliableStream_Static DESTINATION lib)
install(FILES ReliableStream.h DESTINATION include/)

add_executable(teest tests/test.c)
target_link_libraries(teest ReliableStream)

add_executable(teest_pipe tests/test-pipe.c)
target_link_libraries(teest_pipe ReliableStream pthread)