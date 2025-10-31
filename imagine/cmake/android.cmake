# Included by arch-specific Android makefiles

set(CMAKE_SYSTEM_NAME Generic)
set(ENV android)
set(ENV_KERNEL linux)

if(NOT IS_DIRECTORY ${ANDROID_NDK_PATH})
	message(FATAL_ERROR "androidNDKPath.cmake was not included in toolchain file")
endif()

set(ANDROID_CLANG_TOOLCHAIN_PATH "$ENV{ANDROID_CLANG_TOOLCHAIN_PATH}")
if(NOT ANDROID_CLANG_TOOLCHAIN_PATH)
	file(GLOB toolchains "${ANDROID_NDK_PATH}/toolchains/llvm/prebuilt/*")
	list(GET toolchains 0 firstToolchain)
	set(ANDROID_CLANG_TOOLCHAIN_PATH "${firstToolchain}")
endif()
set(ANDROID_CLANG_TOOLCHAIN_BIN_PATH "${ANDROID_CLANG_TOOLCHAIN_PATH}/bin")

if(IS_DIRECTORY ${ANDROID_CLANG_TOOLCHAIN_PATH}/sysroot)
	set(ANDROID_CLANG_SYSROOT_PATH "${ANDROID_CLANG_TOOLCHAIN_PATH}/sysroot")
	set(android_implicitSysroot 1)
else() # Custom toolchain without sysroot, use one from NDK
	file(GLOB toolchains "${ANDROID_NDK_PATH}/toolchains/llvm/prebuilt/*")
	list(GET toolchains 0 firstToolchain)
	set(ANDROID_CLANG_SYSROOT_PATH "${firstToolchain}/sysroot")
endif()

if(NOT IS_DIRECTORY ${ANDROID_CLANG_SYSROOT_PATH})
	message(FATAL_ERROR "ANDROID_CLANG_SYSROOT_PATH not found in NDK or in ANDROID_CLANG_TOOLCHAIN_PATH: ${ANDROID_CLANG_TOOLCHAIN_PATH}")
endif()

if(android_ndkSDK LESS_EQUAL 16)
	# SDK 9 no longer supported since NDK r16 & SDK 16 since NDK r24, use bundled system lib stubs
	string(APPEND CFLAGS_COMMON " -DANDROID_COMPAT_API=${android_ndkSDK}")
	set(android_ndkLinkSysroot "${IMAGINE_PATH}/bundle/android/${CHOST}/${android_ndkSDK}")
endif()

set(CMAKE_C_COMPILER "${ANDROID_CLANG_TOOLCHAIN_BIN_PATH}/clang")
set(CMAKE_CXX_COMPILER "${CMAKE_C_COMPILER}++")
set(CMAKE_AR "${ANDROID_CLANG_TOOLCHAIN_BIN_PATH}/llvm-ar")
set(CMAKE_RANLIB "${ANDROID_CLANG_TOOLCHAIN_BIN_PATH}/llvm-ranlib")
set(CMAKE_STRIP "${ANDROID_CLANG_TOOLCHAIN_BIN_PATH}/llvm-strip")
set(CMAKE_OBJDUMP "${ANDROID_CLANG_TOOLCHAIN_BIN_PATH}/llvm-objdump")

string(APPEND CFLAGS_CODEGEN " -target ${clangTarget} -no-canonical-prefixes")

# libc++
set(useExternalLibcxx 1)
if(NOT useExternalLibcxx)
	string(APPEND LDFLAGS " -static-libstdc++")
endif()

if(NOT CMAKE_ASM_FLAGS_INIT)
	set(CMAKE_ASM_FLAGS_INIT "-target ${clangTarget} -Wa,--noexecstack")
endif()
if(android_ndkLinkSysroot)
	string(APPEND LDFLAGS " --sysroot=${android_ndkLinkSysroot}")
else()
	if(NOT android_implicitSysroot)
		string(APPEND LDFLAGS " --sysroot=${ANDROID_CLANG_SYSROOT_PATH}")
	endif()
endif()
string(APPEND CFLAGS_COMMON " -DANDROID")
if(NOT android_implicitSysroot)
	string(APPEND CFLAGS_COMMON " --sysroot=${ANDROID_CLANG_SYSROOT_PATH}")
endif()
string(APPEND LDFLAGS " -Wl,--no-undefined,-z,noexecstack,-z,relro,-z,now \
-Wl,-O3,--gc-sections,--icf=all,--as-needed,--warn-shared-textrel,--fatal-warnings \
-Wl,--exclude-libs,ALL,--lto-whole-program-visibility")

include("${CMAKE_CURRENT_LIST_DIR}/clang.cmake")
