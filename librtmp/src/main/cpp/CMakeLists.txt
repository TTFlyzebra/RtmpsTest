cmake_minimum_required(VERSION 3.10.2)

project("meitrack_rtmps")

include_directories(${CMAKE_SOURCE_DIR})

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_CRYPTO")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_OPENSSL")

add_library(
        meitrack_rtmps
        SHARED
        CallJava.cpp
        publish_jni.cpp
        push.cpp
        Queue.cpp
        Rtmp.cpp
        RtmpPush.cpp
        librtmp/amf.c
        librtmp/hashswf.c
        librtmp/log.c
        librtmp/parseurl.c
        librtmp/rtmp.c
)

add_library(
        ssl
        SHARED
        IMPORTED
)

set_target_properties(
        ssl
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libssl.so
)

add_library(
        crypto
        SHARED
        IMPORTED
)

set_target_properties(
        crypto
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libcrypto.so
)

target_link_libraries(
        meitrack_rtmps
        ssl
        crypto
        z
        log)