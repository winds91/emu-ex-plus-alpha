set(CMAKE_SYSTEM_NAME Generic)
set(applePlatform 1)
set(ENV ios)
set(ENV_KERNEL mach)

set(linkLoadableModuleAction "-bundle -flat_namespace -undefined suppress")

string(APPEND OBJCFLAGS " -fobjc-arc")
string(APPEND CFLAGS_COMMON " -Wno-error=deprecated-declarations")

set(CMAKE_ASM_COMPILER "as")

set(CCTOOLS_TOOCHAIN_PATH $ENV{CCTOOLS_TOOCHAIN_PATH})

if(CCTOOLS_TOOCHAIN_PATH)
	set(CMAKE_AR llvm-ar)
	set(CMAKE_RANLIB llvm-ranlib)
	file(GLOB ccToolsClangPath "${CCTOOLS_TOOCHAIN_PATH}/bin/*-clang")
	list(GET ccToolsClangPath 0 ccToolsClangPath)
	message("cctools-port Compiler: ${ccToolsClangPath}")
	set(CMAKE_C_COMPILER "${ccToolsClangPath}")
	set(CMAKE_CXX_COMPILER "${CMAKE_C_COMPILER}++")
	set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
	set(iosSimulatorSDKsPath "${CCTOOLS_TOOCHAIN_PATH}/SDK")
	set(iosSDKsPath "${CCTOOLS_TOOCHAIN_PATH}/SDK")
	# Use --no-default-config to prevent distro's Clang config from adding its flags to the build
	string(APPEND CFLAGS_CODEGEN " --no-default-config")
else()
	if(NOT ENV{CXX})
		set(CMAKE_C_COMPILER clang)
		set(CMAKE_CXX_COMPILER clang++)
	endif()
	set(CMAKE_AR ar)
	execute_process(
		COMMAND xcode-select --print-path
		OUTPUT_VARIABLE XCODE_PATH
		OUTPUT_STRIP_TRAILING_WHITESPACE
		COMMAND_ERROR_IS_FATAL ANY
	)
	set(iosSimulatorSDKsPath "${XCODE_PATH}/Platforms/iPhoneSimulator.platform/Developer/SDKs")
	set(iosSDKsPath "${XCODE_PATH}/Platforms/iPhoneOS.platform/Developer/SDKs")
	string(APPEND LDFLAGS " -fobjc-arc")
endif()

if(CMAKE_CXX_COMPILER)
	set(CMAKE_OBJCXX_COMPILER ${CMAKE_CXX_COMPILER})
endif()

execute_process(
	COMMAND ${CMAKE_C_COMPILER} -arch ${SUBARCH} -dumpmachine
	OUTPUT_VARIABLE CHOST
	OUTPUT_STRIP_TRAILING_WHITESPACE
	COMMAND_ERROR_IS_FATAL ANY
)

if(ARCH STREQUAL x86)
	if(NOT IOS_SYSROOT)
 		if(IOS_SDK)
			set(IOS_SYSROOT "${iosSimulatorSDKsPath}/iPhoneSimulator${IOS_SDK}.sdk")
		else()
			file(GLOB iosSimulatorSDKPath "${iosSimulatorSDKsPath}/iPhoneSimulator*.sdk")
			list(GET iosSimulatorSDKPath 0 IOS_SYSROOT)
		endif()
	endif()
	string(APPEND CFLAGS_CODEGEN " -isysroot ${IOS_SYSROOT} -mios-simulator-version-min=${minIOSVer}")
	string(APPEND OBJCFLAGS " -fobjc-abi-version=2 -fobjc-legacy-dispatch")
else()
	if(NOT IOS_SYSROOT)
  		if(IOS_SDK)
			set(IOS_SYSROOT "${iosSDKsPath}/iPhoneOS${IOS_SDK}.sdk")
		else()
			file(GLOB iosSimulatorSDKPath "${iosSDKsPath}/iPhoneOS*.sdk")
			list(GET iosSimulatorSDKPath 0 IOS_SYSROOT)
		endif()
	endif()
	string(APPEND CFLAGS_CODEGEN " -isysroot ${IOS_SYSROOT} -miphoneos-version-min=${minIOSVer}")
endif()

string(APPEND LDFLAGS " -dead_strip -Wl,-no_pie,-x,-dead_strip_dylibs")

string(APPEND CFLAGS_COMMON_RELEASE " -DNS_BLOCK_ASSERTIONS")
set(LDFLAGS_STRIP " -Wl,-S")

# libc++
set(useExternalLibcxx 1)
if(useExternalLibcxx)
	string(APPEND CFLAGS_COMMON " -D_LIBCPP_DISABLE_AVAILABILITY")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/clang.cmake")

set(CMAKE_OBJCXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} ${OBJCFLAGS}")
