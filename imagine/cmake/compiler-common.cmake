if(NOT SUBARCH)
	set(SUBARCH ${ARCH})
endif()

if(NOT IMAGINE_SDK_PLATFORM)
	if(SUBENV)
		set(IMAGINE_SDK_PLATFORM ${ENV}-${SUBARCH}-${SUBENV})
	else()
		set(IMAGINE_SDK_PLATFORM ${ENV}-${SUBARCH})
	endif()
endif()

set(IMAGINE_SDK_PLATFORM_PATH "${IMAGINE_SDK_PATH}/${IMAGINE_SDK_PLATFORM}")
set(PKG_CONFIG_PATH "${IMAGINE_SDK_PLATFORM_PATH}/lib/pkgconfig:${PKG_CONFIG_PATH}")
include_directories("${IMAGINE_SDK_PLATFORM_PATH}/include")
link_directories("${IMAGINE_SDK_PLATFORM_PATH}/lib")

if(useExternalLibcxx)
	string(APPEND LDFLAGS " -nostdlib++ -lc++ -lc++abi -lc++experimental ${cxxSupportLibs}")
	string(APPEND CFLAGS_COMMON " -nostdinc++")
	foreach(LANG CXX OBJCXX)
		list(APPEND CMAKE_${LANG}_STANDARD_INCLUDE_DIRECTORIES "${IMAGINE_SDK_PLATFORM_PATH}/include/c++/v1")
	endforeach()
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CFLAGS_DEBUG "-Og -gz=zstd -ffunction-sections -fdata-sections")
set(CFLAGS_OPTIMIZE "-O3 -ffast-math -flto -fomit-frame-pointer -fno-stack-protector -fno-asynchronous-unwind-tables")
string(APPEND CFLAGS_CODEGEN " -fvisibility=hidden -fno-semantic-interposition")
string(APPEND CFLAGS_COMMON " -pipe")

if(NOT cxxThreadSafeStatics)
	string(APPEND CXXFLAGS " -fno-threadsafe-statics")
endif()

# Disable some undefined sanitizers that greatly increase compile time or are not needed

if(NOT compiler_noSanitizeMode)
	set(compiler_noSanitizeMode "unreachable,return,vptr,enum,nonnull-attribute")
endif()

# setup optimizations

if(compiler_sanitizeMode)
	string(APPEND CFLAGS_DEBUG " -fsanitize=${compiler_sanitizeMode} -fno-omit-frame-pointer")
	if(compiler_noSanitizeMode)
		string(APPEND CFLAGS_DEBUG " -fno-sanitize=${compiler_noSanitizeMode}")
	endif()
endif()

string(APPEND CFLAGS_COMMON_RELEASE " -DNDEBUG -Wdisabled-optimization")

string(APPEND LDFLAGS_STRIP " -s")

# setup warnings

string(APPEND CFLAGS_COMMON " -Wall -Wextra -Werror=return-type -Wno-comment")

# assign all the flags

set(CMAKE_C_FLAGS_INIT "${CFLAGS_COMMON} ${CFLAGS} ${CFLAGS_CODEGEN}")
set(CMAKE_C_FLAGS_DEBUG_INIT "${CFLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE_INIT "${CFLAGS_COMMON_RELEASE} ${CFLAGS_OPTIMIZE}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "${CMAKE_C_FLAGS_RELEASE_INIT}")

set(CMAKE_CXX_FLAGS_INIT "${CFLAGS_COMMON} ${CXXFLAGS} ${CFLAGS_CODEGEN}")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "${CFLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "${CFLAGS_COMMON_RELEASE} ${CFLAGS_OPTIMIZE}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "${CMAKE_CXX_FLAGS_RELEASE_INIT}")

set(CMAKE_EXE_LINKER_FLAGS_INIT "${LDFLAGS} ${CFLAGS_CODEGEN}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT "${CFLAGS_DEBUG}")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT "${CFLAGS_OPTIMIZE} ${LDFLAGS_STRIP}")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT "${CFLAGS_OPTIMIZE}")

set(CMAKE_SHARED_LINKER_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT}")
set(CMAKE_SHARED_LINKER_DEBUG_INIT "${CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT}")
set(CMAKE_SHARED_LINKER_RELEASE_INIT "${CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT}")
set(CMAKE_SHARED_LINKER_RELWITHDEBINFO_INIT "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT}")
