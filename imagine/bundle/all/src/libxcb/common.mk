ifndef CHOST
CHOST := $(shell cc -dumpmachine)
else
buildArg := --build=$(shell cc -dumpmachine)
endif

libxcbVer := 1.16
libxcbSrcDir := libxcb-$(libxcbVer)
libxcbSrcArchive := libxcb-$(libxcbVer).tar.xz

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/src/.libs/libxcb.a
installIncludeDir := $(installDir)/include/xcb

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libxcb to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(libxcbSrcDir)/src/*.h $(buildDir)/src/*.h $(installIncludeDir)/
	cp $(buildDir)/xcb.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(libxcbSrcDir)/configure : | $(libxcbSrcArchive)
	@echo "Extracting libxcb..."
	tar -mxJf $|
	cd $(libxcbSrcDir) && autoreconf -isf

$(outputLibFile) : $(makeFile)
	@echo "Building libxcb..."
	$(MAKE) -C $(<D)

$(makeFile) : $(libxcbSrcDir)/configure
	@echo "Configuring libxcb..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" $$dir/$(libxcbSrcDir)/configure --prefix=$(installDir) --disable-build-docs --disable-shared --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)

