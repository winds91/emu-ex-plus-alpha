cmake_minimum_required(VERSION 4.1)

include("${CMAKE_CURRENT_LIST_DIR}/config.cmake")

set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(ARCH aarch64)
set(SUBARCH arm64)
set(minIOSVer 7.0)
set(targetSuffix -arm64)
set(CMAKE_ASM_FLAGS_INIT "-arch arm64")
# TODO: remove when min iOS target is above 11.0
set(CFLAGS_CODEGEN "-faligned-allocation")

include("${CMAKE_CURRENT_LIST_DIR}/ios.cmake")
