cmake_minimum_required(VERSION 4.1)

project(
	EmuFramework
	VERSION 1.5.84
	DESCRIPTION "Emulator App Framework"
	HOMEPAGE_URL "https://www.explusalpha.com/"
)

set(CMAKE_INSTALL_PREFIX "${IMAGINE_SDK_PLATFORM_PATH}" CACHE PATH "Installation directory" FORCE)

printConfigInfo()
add_library(emuframework STATIC)
set_target_properties(emuframework PROPERTIES OUTPUT_NAME "emuframework${GEN_TARGET_EXT}")
target_include_directories(emuframework PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_options(emuframework PRIVATE -Werror)
addPkgConfigDepMultiConfig(emuframework imagine)
install(TARGETS emuframework DESTINATION lib)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/emuframework" DESTINATION include)

add_subdirectory(src)

evalPkgConfigCFlags(emuframework)
writePkgConfigFiles(emuframework)

add_library(mednafen_common STATIC)
set_target_properties(mednafen_common PROPERTIES OUTPUT_NAME "mednafen_common${GEN_TARGET_EXT}")
target_include_directories(mednafen_common PRIVATE
	${PROJECT_SOURCE_DIR}/include/shared/mednafen
	${PROJECT_SOURCE_DIR}/include/shared/lzma
	${PROJECT_SOURCE_DIR}/include/shared
	${PROJECT_SOURCE_DIR}/src/shared
)
target_compile_options(mednafen_common PRIVATE -Wno-missing-field-initializers -Wno-unused-parameter -Wno-unused-function)
target_compile_definitions(mednafen_common PRIVATE HAVE_CONFIG_H)
install(TARGETS mednafen_common DESTINATION lib)

add_subdirectory(src/shared)