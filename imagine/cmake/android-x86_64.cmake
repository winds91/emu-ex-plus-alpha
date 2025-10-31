cmake_minimum_required(VERSION 4.1)

include("${CMAKE_CURRENT_LIST_DIR}/android-config.cmake")

set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(ARCH x86_64)
set(CHOST x86_64-linux-android)
set(android_ndkSDK 21)
set(clangTarget x86_64-none-linux-android21)
set(CFLAGS_CODEGEN "-fPIC")

include("${CMAKE_CURRENT_LIST_DIR}/android.cmake")
