cmake_minimum_required(VERSION 3.10.2)

project("meitrack_rtmp")

include_directories(${CMAKE_SOURCE_DIR})

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -DUSE_OPENSSL)

add_library(
        meitrack_rtmp
        SHARED
        CallJava.cpp
        publish_jni.cpp
        push.cpp
        Queue.cpp
        Rtmp.cpp
        RtmpPush.cpp
)

add_library(
        rtmp
        STATIC
        IMPORTED
)

set_target_properties(
        rtmp
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/librtmp.so
)

add_library(
        c++
        SHARED
        IMPORTED
)

set_target_properties(
        c++
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libc++.so
)

target_link_libraries(
        meitrack_rtmp
        rtmp
        c++
        z
        log)