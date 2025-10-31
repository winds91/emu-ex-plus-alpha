cmake_minimum_required(VERSION 4.1)

include("${CMAKE_CURRENT_LIST_DIR}/config.cmake")

set(CHOST x86_64-pc-linux-gnu)
set(ARCH x86_64)
set(CFLAGS_CODEGEN "-m64 -march=x86-64-v3 -mtune=generic")
set(CMAKE_ASM_FLAGS_INIT "-m64 -O3")
if(NOT compiler_sanitizeMode)
	set(compiler_sanitizeMode "address,undefined")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/linux.cmake")
