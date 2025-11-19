cmake_minimum_required(VERSION 4.1)

project(
	FrameRateTest
	DESCRIPTION "Frame Rate Test"
	HOMEPAGE_URL "https://www.explusalpha.com/"
)

printConfigInfo()
configureAppTarget(frameRateTest)
target_compile_options(frameRateTest PRIVATE -Werror)
addPkgConfigDepMultiConfig(frameRateTest imagine)
evalPkgConfigFlags(frameRateTest all)
target_sources(frameRateTest PRIVATE src/main/main.cc src/main/tests.cc src/main/TestPicker.cc src/main/cpuUtils.cc)
