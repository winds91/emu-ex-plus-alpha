cmake_minimum_required(VERSION 4.1)

include("${CMAKE_CURRENT_LIST_DIR}/android-config.cmake")

set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(ARCH aarch64)
set(SUBARCH arm64)
set(CHOST aarch64-linux-android)
set(android_ndkSDK 21)
set(clangTarget aarch64-none-linux-android21)
set(CFLAGS_CODEGEN "-fpic")
set(LDFLAGS "-Wl,-z,max-page-size=16384")

include("${CMAKE_CURRENT_LIST_DIR}/android.cmake")
