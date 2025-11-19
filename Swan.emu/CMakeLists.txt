cmake_minimum_required(VERSION 4.1)

project(
	Swan.emu
	DESCRIPTION "WonderSwan emulator"
	HOMEPAGE_URL "https://www.explusalpha.com/"
)

include(emuFrameworkUtils.cmake)
printConfigInfo()
configureAppTarget(swanemu)
addMednafenFlags(swanemu)
addPkgConfigDepMultiConfig(swanemu emuframework)
evalPkgConfigFlags(swanemu all)
add_subdirectory(src)