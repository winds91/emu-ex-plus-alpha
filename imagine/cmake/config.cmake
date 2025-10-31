# common utility functions and variable setup included by all toolchain files

function(setFromEnv var defaultVal)
	if(NOT ENV{${var}})
		set(${var} "${defaultVal}" PARENT_SCOPE)
	else()
		set(${var} "$ENV{${var}}" PARENT_SCOPE)
	endif()
endFunction()

function(getProp out target prop)
	get_target_property(v ${target} ${prop})
	if(NOT v)
		set(${out} "" PARENT_SCOPE)
		return()
	endif()
	set(${out} "${v}" PARENT_SCOPE)
endFunction()

function(getJoinedProp out target prop)
	getProp(v ${target} ${prop})
	list(JOIN v " " v)
	set(${out} "${v}" PARENT_SCOPE)
endFunction()

function(printConfigInfo)
	message("C Flags (Common): ${CMAKE_C_FLAGS}")
	message("C Flags (Debug): ${CMAKE_C_FLAGS_DEBUG}")
	message("C Flags (Release): ${CMAKE_C_FLAGS_RELEASE}")
	message("C Flags (Release + Debug Info): ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
	message("C++ Flags (Common): ${CMAKE_CXX_FLAGS}")
	message("C++ Flags (Debug): ${CMAKE_CXX_FLAGS_DEBUG}")
	message("C++ Flags (Release): ${CMAKE_CXX_FLAGS_RELEASE}")
	message("C++ Flags (Release + Debug Info): ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
	message("Linker Flags (Common): ${CMAKE_EXE_LINKER_FLAGS}")
	message("Linker Flags (Debug): ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
	message("Linker Flags (Release): ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
	message("Linker Flags (Release + Debug Info): ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}")
	message("SDK Path: ${IMAGINE_SDK_PLATFORM_PATH}")
endFunction()

# config.h generation

function(addConfigEnable target v)
	set_property(TARGET ${target} APPEND PROPERTY configEnable "${v}")
endFunction()

function(generateConfigHeader target)
	getJoinedProp(configEnable ${target} configEnable)
	getJoinedProp(configDisable ${target} configDisable)
	getJoinedProp(configInc ${target} configInc)
	if(NOT configEnable AND NOT configDisable AND NOT configInc)
		return()
	endif()
	set(configFilename "${target}-config.h")
	set(genDir "${CMAKE_BINARY_DIR}/gen")
	set(configFilePath "${genDir}/${configFilename}")
	message("Config Header: ${configFilePath}")
	execute_process(
		COMMAND mkdir -p ${genDir}
		COMMAND bash ${IMAGINE_PATH}/make/writeConfig.sh "${configFilePath}" "${configEnable}" "${configDisable}" "${configInc}"
		COMMAND_ERROR_IS_FATAL ANY
	)	
	target_include_directories(${target} PRIVATE ${genDir})
	install(FILES "${configFilePath}" DESTINATION include)
endFunction()

# pkg-config support

function(addPkgConfigDeps target dep)
	set_property(TARGET ${target} APPEND PROPERTY PKG_CONFIG_TARGET_DEPS "${dep}")
endFunction()

function(addPkgConfigDepsForConfig target dep config)
	set_property(TARGET ${target} APPEND PROPERTY PKG_CONFIG_TARGET_DEPS_${config} "${dep}")
endFunction()

function(addPkgConfigDepMultiConfig target dep)
	foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
		addPkgConfigDepsForConfig(${target} ${dep}${TARGET_EXT_${config}} ${config})
	endforeach()
endFunction()

function(addPkgConfigLibs target lib)
	set_property(TARGET ${target} APPEND PROPERTY PKG_CONFIG_TARGET_LIBS "${lib}")
endFunction()

function(evalPkgConfigCFlags target)
	getJoinedProp(pkgConfigDeps ${target} PKG_CONFIG_TARGET_DEPS)
	getJoinedProp(pkgConfigDepsRelease ${target} PKG_CONFIG_TARGET_DEPS_Release)
	string(APPEND pkgConfigDeps " ${pkgConfigDepsRelease}")

	if(pkgConfigDeps)
		message("Config Packages: ${pkgConfigDeps}")
	else()
		message("No config packages")
		return()
	endif()

	set(ENV{PKG_CONFIG_PATH} "${PKG_CONFIG_PATH}")
	set(ENV{PKG_CONFIG_SYSTEM_INCLUDE_PATH} "${PKG_CONFIG_SYSTEM_INCLUDE_PATH}")
	set(ENV{PKG_CONFIG_SYSTEM_LIBRARY_PATH} "${PKG_CONFIG_SYSTEM_LIBRARY_PATH}")

	execute_process(
		COMMAND pkg-config ${pkgConfigDeps} --cflags ${pkgConfigOpts} ${pkgConfigDepsRelease}
		OUTPUT_VARIABLE pkgConfigCFlagsOutput
		OUTPUT_STRIP_TRAILING_WHITESPACE
		COMMAND_ERROR_IS_FATAL ANY
	)

	string(REPLACE " " ";" pkgConfigCFlagsOutput "${pkgConfigCFlagsOutput}")
	target_compile_options(${target} PUBLIC ${pkgConfigCFlagsOutput})
	get_target_property(targetCompileOptions ${target} COMPILE_OPTIONS)
	list(JOIN targetCompileOptions " " targetCompileOptions)
	message("Target Compile Flags: ${targetCompileOptions}")
endFunction()

function(writePkgConfigFiles target)
	getJoinedProp(pkgRequires ${target} PKG_CONFIG_TARGET_DEPS)
	getJoinedProp(pkgLibs ${target} PKG_CONFIG_TARGET_LIBS)	
	foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
		set(targetExt "${TARGET_EXT_${config}}")
		getJoinedProp(pkgRequiresForConfig ${target} PKG_CONFIG_TARGET_DEPS_${config})
		set(PKG_CONFIG_REQUIRES "${pkgRequires} ${pkgRequiresForConfig}")
		set(PKG_CONFIG_LIBS "-l${target}${targetExt} ${pkgLibs}")
		configure_file(
			"${IMAGINE_PATH}/cmake/pkgconfig.pc.in"
			"${CMAKE_BINARY_DIR}/${target}${targetExt}.pc"
			@ONLY
		)
	endforeach()
	# directly copy the .pc file else ${pcfiledir} will incorrectly point to the build directory when using CMAKE_INSTALL_MODE with symlinks
	install(CODE "
		file(COPY \"${CMAKE_BINARY_DIR}/${target}${GEN_TARGET_EXT}.pc\" DESTINATION \"${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/\")
	")
endFunction()

function(addPkgAlsa target)
	addConfigEnable(${target} CONFIG_PACKAGE_ALSA)
	addPkgConfigDeps(${target} alsa)
endFunction()

function(addPkgBluez target)
	if(NOT ENV STREQUAL android)
		addPkgConfigDeps(${target} bluez)
	endif()
endFunction()

function(addPkgBtStack target)
	addPkgConfigLibs(${target} -lBTstack)
endFunction()

function(addPkgEgl target)
	if(ENV STREQUAL android OR SUBENV STREQUAL pandora)
		addPkgConfigLibs(${target} -lEGL)
	elseif(ENV STREQUAL linux)
		addPkgConfigDeps(${target} egl)
	endif()
endfunction()

function(addPkgFlac target)
	addPkgConfigDeps(${target} flac)
endFunction()

function(addPkgFontconfig target)
	addConfigEnable(${target} CONFIG_PACKAGE_FONTCONFIG)
	addPkgConfigDeps(${target} fontconfig)
endFunction()

function(addPkgFreetype target)
	addPkgConfigDeps(${target} freetype2)
endFunction()

function(addPkgGio target)
	addConfigEnable(${target} CONFIG_PACKAGE_DBUS)
	addPkgConfigDeps(${target} gio-2.0)
endFunction()

function(addPkgGlib target)
	addPkgConfigDeps(${target} glib-2.0)
endFunction()

function(addPkgZlib target)
	set(envWithLib android;ios)
	if(ENV IN_LIST envWithLib)
		addPkgConfigLibs(${target} -lz)
	else()
		addPkgConfigDeps(${target} zlib)
	endif()
endFunction()

function(addPkgLibarchive target)
	addPkgZlib(${target})
	if(ENV STREQUAL linux)
		addPkgConfigDeps(${target} "liblzma")
		addPkgConfigLibs(${target} "${IMAGINE_SDK_PLATFORM_PATH}/lib/libarchive.a")
	else()
		addPkgConfigDeps(${target} "libarchive liblzma")
	endif()
endFunction()

function(addPkgLibdrm target)
	addConfigEnable(${target} CONFIG_PACKAGE_LIBDRM)
	addPkgConfigDeps(${target} libdrm)
endFunction()

function(addPkgLibpng target)
	addPkgConfigDeps(${target} libpng)
endFunction()

function(addPkgLibvorbis target)
	addConfigEnable(${target} CONFIG_PACKAGE_LIBVORBIS)
	addPkgConfigDeps(${target} vorbisfile)
endFunction()

function(addPkgOpenGL target)
	if(SUBENV STREQUAL pandora)
		addPkgConfigLibs(${target} "-lGLESv2 -lm")
	elseif(ENV STREQUAL linux)
		if(openGLAPI STREQUAL gles)
			addPkgConfigDeps(${target} glesv2)
		else()
			addPkgConfigDeps(${target} gl)
		endif()
	elseif(ENV STREQUAL android)
		addPkgConfigLibs(${target} -lGLESv2)
	elseif(ENV STREQUAL ios)
		addPkgConfigLibs(${target} "-framework OpenGLES")
	elseif(ENV STREQUAL macosx)
		addPkgConfigLibs(${target} "-framework OpenGL" "-framework CoreVideo")
	endif()
endFunction()

function(addPkgPulseAudio target)
	addConfigEnable(${target} CONFIG_PACKAGE_PULSEAUDIO)
	addPkgConfigDeps(${target} libpulse)
endfunction()

function(addPkgX11 target)
	addConfigEnable(${target} CONFIG_PACKAGE_X11)
	addPkgConfigDeps(${target} "xcb xcb-xfixes xcb-xinput xcb-icccm xkbcommon xkbcommon-x11")
endfunction()

function(addPkgXRandr target)
	addPkgConfigDeps(${target} xcb-randr)
endfunction()

# set imagine path

set(IMAGINE_PATH "$ENV{IMAGINE_PATH}")
setFromEnv(IMAGINE_SDK_PATH "$ENV{HOME}/imagine-sdk")

if(NOT IS_DIRECTORY "${IMAGINE_PATH}/cmake")
	message(FATAL_ERROR "Invalid Imagine path:${IMAGINE_PATH}, please set IMAGINE_PATH to the root path of the Imagine distribution")
endif()

# init toolchain variables

set(CFLAGS)
set(CXXFLAGS)
set(CFLAGS_COMMON) # common C/C++ flags that don't affect generated code
set(CFLAGS_COMMON_RELEASE)
set(CFLAGS_CODEGEN) # common C/C++ flags that do affect generated code
set(OBJCFLAGS)
set(LDFLAGS)
set(LDFLAGS_STRIP)
set(TARGET_EXT_Debug -debug)
set(TARGET_EXT_RelWithDebInfo -rdebug)
set(GEN_TARGET_EXT)

foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
	string(APPEND GEN_TARGET_EXT "$<$<CONFIG:${config}>:${TARGET_EXT_${config}}>")
endforeach()
