cmake_minimum_required(VERSION 4.1)

project(
	Imagine
	VERSION 1.5.84
	DESCRIPTION "Game/Multimedia Engine"
	HOMEPAGE_URL "https://www.explusalpha.com/"
)

set(CMAKE_INSTALL_PREFIX "${IMAGINE_SDK_PLATFORM_PATH}" CACHE PATH "Installation directory" FORCE)

if(applePlatform)
	enable_language(OBJCXX)
endif()

printConfigInfo()
add_library(imagine STATIC)
set_target_properties(imagine PROPERTIES OUTPUT_NAME "imagine${GEN_TARGET_EXT}")
set_property(TARGET imagine PROPERTY C_STANDARD 23)
target_include_directories(imagine PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_options(imagine PRIVATE -Werror)
install(TARGETS imagine DESTINATION lib)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/imagine" DESTINATION include)

add_subdirectory(src/base)
add_subdirectory(src/audio)
add_subdirectory(src/input)
add_subdirectory(src/gfx)
add_subdirectory(src/fs)
add_subdirectory(src/io)
add_subdirectory(src/bluetooth)
add_subdirectory(src/gui)
add_subdirectory(src/font)
add_subdirectory(src/data-type/image)
add_subdirectory(src/vmem)
add_subdirectory(src/logger/stdio)
add_subdirectory(src/util)

if(SUBENV STREQUAL pandora)
	addConfigEnable(imagine CONFIG_MACHINE_PANDORA)
endif()

generateConfigHeader(imagine)
evalPkgConfigCFlags(imagine)
writePkgConfigFiles(imagine)
