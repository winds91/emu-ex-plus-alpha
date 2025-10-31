cmake_minimum_required(VERSION 4.1)

include("${CMAKE_CURRENT_LIST_DIR}/android-config.cmake")

set(CMAKE_SYSTEM_PROCESSOR armv7-a)
set(ARCH arm)
set(SUBARCH armv7)
set(CHOST arm-linux-androideabi)
set(android_ndkSDK 9)
# Must declare min API 21 to compile with NDK r26+ headers
set(clangTarget armv7-none-linux-androideabi21)
set(CFLAGS_CODEGEN "-march=armv7-a -mthumb -mtune=generic -fpic")
set(LDFLAGS "-Wl,--fix-cortex-a8")
set(cxxSupportLibs "-landroid_support")
set(CMAKE_ASM_FLAGS_INIT "--noexecstack -EL -mfloat-abi=softfp -march=armv7-a")

# Directly call the GNU assembler until assembly in projects is updated for clang's integrated assembler

find_program(gnuAs arm-none-linux-gnueabi-as)
if(NOT gnuAs)
	find_program(gnuAs arm-linux-gnueabi-as)
endif()
set(CMAKE_ASM_COMPILER "${gnuAs}")

include("${CMAKE_CURRENT_LIST_DIR}/android.cmake")
