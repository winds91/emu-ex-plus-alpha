/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "LibPNG"

#include <imagine/data-type/image/LibPNG.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/string.h>
#include <assert.h>

// this must be in the range 1 to 8
#define INITIAL_HEADER_READ_BYTES 8

bool Png::supportUncommonConv = 0;

using namespace IG;

#ifndef PNG_ERROR_TEXT_SUPPORTED

CLINK void PNGAPI png_error(png_const_structrp png_ptr, png_const_charp error_message) PNG_NORETURN;
CLINK void PNGAPI png_error(png_const_structrp png_ptr, png_const_charp error_message)
{
	// TODO: print out more verbose error
	logErr("fatal libpng error");
	Base::abort();
}

CLINK void PNGAPI png_chunk_error(png_const_structrp png_ptr, png_const_charp error_message) PNG_NORETURN;
CLINK void PNGAPI png_chunk_error(png_const_structrp png_ptr, png_const_charp error_message)
{
	png_error(png_ptr, error_message);
}

#endif

#ifndef PNG_WARNINGS_SUPPORTED

CLINK void PNGAPI png_warning(png_const_structrp png_ptr, png_const_charp warning_message)
{
	// TODO: print out more verbose warning
	logWarn("libpng warning");
}

CLINK void PNGAPI png_chunk_warning(png_const_structrp png_ptr, png_const_charp warning_message)
{
	png_warning(png_ptr, warning_message);
}

#endif

static void png_ioReader(png_structp pngPtr, png_bytep data, png_size_t length)
{
	auto &io = *(IO*)png_get_io_ptr(pngPtr);
	if(io.read(data, length) != (ssize_t)length)
	{
		logErr("error reading png file");
		png_error(pngPtr, "Read Error");
	}
}

uint32_t Png::width()
{
	return png_get_image_width(png, info);
}

uint32_t Png::height()
{
	return png_get_image_height(png, info);
}

bool Png::isGrayscale()
{
	return !(png_get_color_type(png, info) & PNG_COLOR_MASK_COLOR);
}

PixelFormat Png::pixelFormat()
{
	bool grayscale = isGrayscale();
	bool alpha = hasAlphaChannel();
	if(grayscale)
		return alpha ? PIXEL_FMT_IA88 : PIXEL_FMT_I8;
	else
		return alpha ? PIXEL_FMT_RGBA8888 : PIXEL_FMT_RGB888;
}

static png_voidp png_memAlloc(png_structp png_ptr, png_size_t size)
{
	//log_mPrintf(LOG_MSG, "about to allocate %d bytes", size);
	return new uint8_t[size];
}

static void png_memFree(png_structp png_ptr, png_voidp ptr)
{
	delete[] (uint8_t*)ptr;
}

std::error_code Png::readHeader(GenericIO io)
{
	//logMsg("reading header from file handle @ %p",stream);
	
	//logMsg("initial reading to start at offset 0x%X", (int)stream->ftell);
	
	//log_mPrintf(LOG_MSG, "%d items %d size, %d", 10, 500, PNG_UINT_32_MAX/500);
	uint8_t header[INITIAL_HEADER_READ_BYTES];
	if(io.read(&header, INITIAL_HEADER_READ_BYTES) != INITIAL_HEADER_READ_BYTES)
		return {EIO, std::system_category()};

	int isPng = !png_sig_cmp(header, 0, INITIAL_HEADER_READ_BYTES);
	if (!isPng)
	{
		logErr("error - not a png file");
		return {EINVAL, std::system_category()};
	}

	png = png_create_read_struct_2 (PNG_LIBPNG_VER_STRING,
		/*(png_voidp)user_error_ptr*/NULL, /*user_error_fn*/NULL, /*user_warning_fn*/NULL,
		0, png_memAlloc, png_memFree);
	if (png == NULL)
	{
		logErr("error allocating png struct");
		return {ENOMEM, std::system_category()};
	}

	//log_mPrintf(LOG_MSG,"creating info struct");
	info = png_create_info_struct(png);
	if (info == NULL)
	{
		logErr("error allocating png info");
		png_structpp pngStructpAddr = &png;
		png_destroy_read_struct(pngStructpAddr, (png_infopp)NULL, (png_infopp)NULL);
		return {ENOMEM, std::system_category()};
	}

	/*log_mPrintf(LOG_MSG,"creating end info struct");
	end = png_create_info_struct(png);
	if (!end)
	{
		log_mPrintf(LOG_ERR,"error allocating png end info");
		void * pngStructpAddr = &png;
		void * pngInfopAddr = &info;
		png_destroy_read_struct(pngStructpAddr, pngInfopAddr, (png_infopp)NULL);
		return (OUT_OF_MEMORY);
	}*/

	if (setjmp(png_jmpbuf(png)))
	{
		logErr("error occured, jumped to setjmp");
		png_structpp pngStructpAddr = &png;
		png_infopp pngInfopAddr = &info;
		//void * pngEndInfopAddr = &data->end;
		png_destroy_read_struct(pngStructpAddr, pngInfopAddr, (png_infopp)NULL/*pngEndInfopAddr*/);
		return {EIO, std::system_category()};
	}
	
	//init custom libpng io
	png_set_read_fn(png, io.release(), png_ioReader);
	//log_mPrintf(LOG_MSG,"set custom png read function %p", png_ioReader);
	
	png_set_sig_bytes(png, INITIAL_HEADER_READ_BYTES);
	//log_mPrintf(LOG_MSG,"let libpng know we read %d bytes already", INITIAL_HEADER_READ_BYTES);
	
	png_read_info(png, info);
	
	//log_mPrintf(LOG_MSG,"finished reading header");
	return {};
}

void Png::freeImageData()
{
	if(png)
	{
		logMsg("deallocating libpng data");
		delete (IO*)png_get_io_ptr(png);
		png_structpp pngStructpAddr = &png;
		png_infopp pngInfopAddr = &info;
		//void * pngEndInfopAddr = &data->end;
		png_destroy_read_struct(pngStructpAddr, pngInfopAddr, (png_infopp)NULL/*pngEndInfopAddr*/);
	}
}

bool Png::hasAlphaChannel()
{
	return ( (png_get_color_type(png, info) & PNG_COLOR_MASK_ALPHA) ||
				( png_get_valid(png, info, PNG_INFO_tRNS) ) ) ? 1 : 0;
}

// TODO: write a filter to convert pixels to 16-bits
/*static void pngConvertBGR888ToXBGR1555(png_structp png_ptr, png_row_infop row_info, png_bytep data)
{
	
}*/

void Png::setTransforms(IG::PixelFormat outFormat, png_infop transInfo)
{
	int addingAlphaChannel = 0;
	
	if(png_get_color_type(png, info) == PNG_COLOR_TYPE_PALETTE)
	{
		//logMsg("converting paletted image to direct color");
		png_set_palette_to_rgb(png);
	}

	if(png_get_valid(png, info, PNG_INFO_tRNS))
	{
		//logMsg("coverting pallete transparency to alpha channel");
		png_set_tRNS_to_alpha(png);
		addingAlphaChannel = 1;
	}

	if(png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY && png_get_bit_depth(png, info) < 8)
	{
		logMsg("expanding gray-scale to 8-bits");
		png_set_expand_gray_1_2_4_to_8(png);
	}
	
	// convert 16-bits per channel to 8-bits per channel
	#ifndef PNG_NO_READ_16_TO_8
	if(png_get_bit_depth(png, info) == 16)
	{
		logMsg("converting 16-bits per channel to 8-bits per channel");
		png_set_strip_16(png);
	}
	#endif
	
	if(supportUncommonConv)
	{
		if((png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY || png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY_ALPHA) &&
				!(outFormat.isGrayscale()))
		{
			logMsg("converting gray-scale to RGB");
			png_set_gray_to_rgb(png);
		}

		if(!outFormat.desc().aBits)
		{
			if(png_get_color_type(png, info) & PNG_COLOR_MASK_ALPHA || addingAlphaChannel)
			{
				logMsg("removing alpha channel");
				png_set_strip_alpha(png);
			}
		}
		else if((!hasAlphaChannel() && png_get_color_type(png, info) == PNG_COLOR_TYPE_RGB)
			|| (!hasAlphaChannel() && outFormat.desc().aBits ))
		{
			logMsg("adding alpha channel");
				png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
		}

		if(outFormat.desc().aBits && outFormat.desc().aShift != 0)
		{
			logMsg("swapping RGBA to ARGB");
			png_set_swap_alpha(png);
		}
	}
	
	if(outFormat.isBGROrder())
	{
		//logMsg("swaping colors to BGR order");
		png_set_bgr(png);
	}
	
	//if(outFormat == PIXEL_ABGR1555)
	//{
		//TODO: convert to 16-bits
	//}
	
	png_read_update_info(png, info);
}

std::errc Png::readImage(IG::Pixmap &dest)
{
	//logMsg("reading whole image to %p", buffer);
	//log_mPrintf(LOG_MSG,"buffer has %d byte pitch", pitch);
	
	int height = this->height();
	int width = this->width();
	//int rowbytes = png_get_rowbytes(png, info);
	//log_mPrintf(LOG_MSG,"width = %d, height = %d, rowbytes = %d", width, height, rowbytes);

	png_infop transInfo = png_create_info_struct(png);
	if (!transInfo)
	{
		logErr("error allocating png transform info");
		png_structpp pngStructpAddr = &png;
		png_infopp pngInfopAddr = &info;
		png_destroy_read_struct(pngStructpAddr, pngInfopAddr, (png_infopp)NULL);
		return std::errc::not_enough_memory;
	}
	setTransforms(dest.format(), transInfo);
	
	//log_mPrintf(LOG_MSG,"after transforms, rowbytes = %u", (uint32_t)png_get_rowbytes(data->png, data->info));

	assert( (uint32_t)width*dest.format().bytesPerPixel() == png_get_rowbytes(png, info) );
	
	if(png_get_interlace_type(png, info) == PNG_INTERLACE_NONE)
	{
		//logMsg("reading row at a time");
		if (setjmp(png_jmpbuf((png_structp)png)))
		{
			logErr("error reading image, jumped to setjmp");
			return std::errc::io_error;
		}

		for (int i = 0; i < height; i++)
		{
			png_read_row(png, (png_bytep)dest.pixel({0, i}), nullptr);
		}
	}
	else // read the whole image in 1 call with interlace handling, but needs array of row pointers allocated
	{
		logMsg("is interlaced");
		png_bytep rowPtr[height];
		for (int i = 0; i < height; i++)
		{
			//logr_mPrintf(LOG_MSG,row relative offset = %d", offset);
			rowPtr[i] = (png_bytep)dest.pixel({0, i});
			//log_mPrintf(LOG_MSG, "set row pointer %d to %p", i, row_pointers[i]);
		}

		if (setjmp(png_jmpbuf((png_structp)png)))
		{
			logErr("error reading image, jumped to setjmp");
			return std::errc::io_error;
		}

		png_read_image(png, rowPtr);
	}
	
	//log_mPrintf(LOG_MSG,"reading complete");
	png_read_end(png, nullptr);

	png_infopp pngInfopAddr = &transInfo;
	png_destroy_info_struct(png, pngInfopAddr);
	return {};
}

Png::operator bool() const
{
	return info;
}

PngFile::PngFile() {}

PngFile::~PngFile()
{
	deinit();
}

std::errc PngFile::write(IG::Pixmap dest)
{
	return(png.readImage(dest));
}

IG::Pixmap PngFile::lockPixmap()
{
	IG::Pixmap pix{{{(int)png.width(), (int)png.height()}, png.pixelFormat()}, nullptr};
	return pix;
}

void PngFile::unlockPixmap() {}

std::error_code PngFile::load(GenericIO io)
{
	deinit();
	if(!io)
		return {EINVAL, std::system_category()};
	auto ec = png.readHeader(std::move(io));
	if(ec)
	{
		logErr("error reading header");
		return ec;
	}
	return {};
}

std::error_code PngFile::load(const char *name)
{
	deinit();
	if(!string_hasDotExtension(name, "png"))
	{
		logErr("suffix doesn't match PNG image");
		return {EINVAL, std::system_category()};
	}
	FileIO io;
	auto ec = io.open(name, IO::AccessHint::ALL);
	if(ec)
	{
		return ec;
	}
	return load(io.makeGeneric());
}

std::error_code PngFile::loadAsset(const char *name, const char *appName)
{
	return load(FileUtils::openAppAsset(name, IO::AccessHint::ALL, appName).makeGeneric());
}

void PngFile::deinit()
{
	png.freeImageData();
}

PngFile::operator bool() const
{
	return (bool)png;
}
