ifndef inc_mdfn_common
inc_mdfn_common := 1

ifndef RELEASE
 mdfnLibExt := -debug
endif

LDLIBS += -lmednafen_common$(mdfnLibExt)

MDFN_COMMON_CPPFLAGS = -DHAVE_CONFIG_H \
 -I$(EMUFRAMEWORK_PATH)/include/shared/mednafen \
 -I$(EMUFRAMEWORK_PATH)/src/shared

MDFN_CDROM_CPPFLAGS = -I$(EMUFRAMEWORK_PATH)/include/shared/lzma \
 -I$(EMUFRAMEWORK_PATH)/include/shared
endif