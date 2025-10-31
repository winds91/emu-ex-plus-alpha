cmake_minimum_required(VERSION 4.1)

include("${CMAKE_CURRENT_LIST_DIR}/android-config.cmake")

set(CMAKE_SYSTEM_PROCESSOR i686)
set(ARCH x86)
set(CHOST i686-linux-android)
set(android_ndkSDK 16)
# Must declare min API 21 to compile with NDK r26+ headers
set(clangTarget i686-none-linux-android21)
set(CFLAGS_CODEGEN "-fPIC -mstackrealign")
set(cxxSupportLibs "-landroid_support")

include("${CMAKE_CURRENT_LIST_DIR}/android.cmake")
