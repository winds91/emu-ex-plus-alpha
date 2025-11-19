target_sources(swanemu PRIVATE
	main/Main.cc
	main/options.cc
	main/input.cc
	main/EmuMenuViews.cc
	wswan/comm.cpp
	wswan/gfx.cpp
	wswan/memory.cpp
	wswan/tcache.cpp
	wswan/interrupt.cpp
	wswan/rtc.cpp
	wswan/v30mz.cpp
	wswan/eeprom.cpp
	wswan/main.cpp
	wswan/sound.cpp
)