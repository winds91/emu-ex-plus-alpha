set(imagineModuleSrcs
	imagine.ccm
	imagine.audio.ccm
	imagine.bluetooth.ccm
	imagine.data.ccm
	imagine.gfx.ccm
	imagine.gui.ccm
)

target_sources(imagine PRIVATE FILE_SET CXX_MODULES FILES ${imagineModuleSrcs})

install(FILES ${imagineModuleSrcs} DESTINATION share/imagine)

add_subdirectory(base)
add_subdirectory(audio)
add_subdirectory(input)
add_subdirectory(gfx)
add_subdirectory(fs)
add_subdirectory(io)
add_subdirectory(bluetooth)
add_subdirectory(gui)
add_subdirectory(font)
add_subdirectory(data-type/image)
add_subdirectory(vmem)
add_subdirectory(logger/stdio)
add_subdirectory(util)