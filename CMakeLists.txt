cmake_minimum_required(VERSION 3.16)

project(rtc_telemetry)

set(CMAKE_CXX_STANDARD 17)

set(LIBDATACHANNEL_DIR ${CMAKE_SOURCE_DIR}/depend/libdatachannel)

add_executable(sender src/sender.cpp src/sender.hpp)
add_executable(receiver src/receiver.cpp src/receiver.hpp)

foreach(TARGET sender receiver)
    target_include_directories(${TARGET} PRIVATE
        ${LIBDATACHANNEL_DIR}/include
    )
    target_link_libraries(${TARGET} ${LIBDATACHANNEL_DIR}/lib/libdatachannel.so)
    set_target_properties(${TARGET} PROPERTIES
        BUILD_RPATH ${LIBDATACHANNEL_DIR}/lib
    )
endforeach()