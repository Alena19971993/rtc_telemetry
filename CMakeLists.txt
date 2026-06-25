cmake_minimum_required(VERSION 3.16)
project(rtc_telemetry)

set(CMAKE_CXX_STANDARD 17)
set(LIBDATACHANNEL_DIR ${CMAKE_SOURCE_DIR}/depend/libdatachannel)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil)

add_compile_options(
    -Wall
    -Wextra
    -Wshadow
    -Wuninitialized
    -Wconversion
    -Wpedantic
)

add_executable(sender src/sender.cpp src/sender.hpp src/sdp_file.hpp src/message.hpp src/video_reader.hpp src/video_reader.cpp)
add_executable(receiver src/receiver.cpp src/receiver.hpp src/sdp_file.hpp src/message.hpp)

foreach(TARGET sender receiver)
    target_include_directories(${TARGET} SYSTEM PRIVATE
        ${LIBDATACHANNEL_DIR}/include
    )
    target_link_libraries(${TARGET} ${LIBDATACHANNEL_DIR}/lib/libdatachannel.so)
    target_link_libraries(${TARGET}
        ${LIBDATACHANNEL_DIR}/lib/libdatachannel.so
        PkgConfig::FFMPEG
    )
    set_target_properties(${TARGET} PROPERTIES
        BUILD_RPATH ${LIBDATACHANNEL_DIR}/lib
    )
endforeach()
