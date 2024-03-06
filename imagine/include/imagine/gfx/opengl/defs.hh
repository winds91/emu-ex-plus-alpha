#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/util/memory/UniqueResource.hh>
#include <imagine/util/used.hh>
#include <variant>
#include <span>

namespace Config
{
	namespace Gfx
	{
	#ifndef CONFIG_GFX_OPENGL_ES
		#if defined CONFIG_OS_IOS || defined __ANDROID__ || defined CONFIG_MACHINE_PANDORA
		#define CONFIG_GFX_OPENGL_ES 2
		#endif
	#endif

	#ifdef CONFIG_GFX_OPENGL_ES
	static constexpr int OPENGL_ES = CONFIG_GFX_OPENGL_ES;
	#else
	static constexpr int OPENGL_ES = 0;
	#endif

	#ifdef CONFIG_GFX_ANDROID_SURFACE_TEXTURE
	#define CONFIG_GFX_OPENGL_TEXTURE_TARGET_EXTERNAL
	static constexpr bool OPENGL_TEXTURE_TARGET_EXTERNAL = true;
	#else
	static constexpr bool OPENGL_TEXTURE_TARGET_EXTERNAL = false;
	#endif

	#if !defined NDEBUG && !defined __APPLE__
	#define CONFIG_GFX_OPENGL_DEBUG_CONTEXT
	static constexpr bool OPENGL_DEBUG_CONTEXT = true;
	#else
	static constexpr bool OPENGL_DEBUG_CONTEXT = false;
	#endif

	#if defined CONFIG_OS_IOS
	static constexpr bool GLDRAWABLE_NEEDS_FRAMEBUFFER = true;
	#else
	static constexpr bool GLDRAWABLE_NEEDS_FRAMEBUFFER = false;
	#endif
	}
}

// Header Locations For Platform

#if defined CONFIG_OS_IOS
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#elif defined CONFIG_BASE_MACOSX
#import <OpenGL/gl3.h>
#import <OpenGL/gl3ext.h>
#elif defined CONFIG_BASE_WIN32
#include <util/windows/windows.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/wglew.h>
#elif defined CONFIG_GFX_OPENGL_ES // Generic OpenGL ES headers
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#undef GL_GLEXT_PROTOTYPES
#else // Generic OpenGL headers
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_PROTOTYPES
#endif

// Symbol Re-mapping

#if defined CONFIG_GFX_OPENGL_ES
	#ifndef GL_BGRA
	#define GL_BGRA GL_BGRA_EXT
	#endif
	#ifndef GL_ALPHA8
	#define GL_ALPHA8 0x803C
	#endif
	#ifndef GL_LUMINANCE8
	#define GL_LUMINANCE8 0x8040
	#endif
	#ifndef GL_LUMINANCE8_ALPHA8
	#define GL_LUMINANCE8_ALPHA8 0x8045
	#endif
	#ifndef GL_SYNC_FLUSH_COMMANDS_BIT
	#define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
	#endif
#endif

#ifndef GL_APIENTRY
#define GL_APIENTRY GLAPIENTRY
#endif

namespace IG::Gfx
{

class RendererTask;

using TextureRef = GLuint;

using VertexIndexSpan = std::variant<std::span<const uint8_t>, std::span<const uint16_t>>;

static constexpr int TRIANGLE_IMPL = GL_TRIANGLES;
static constexpr int TRIANGLE_STRIP_IMPL = GL_TRIANGLE_STRIP;

static constexpr int ZERO_IMPL = GL_ZERO;
static constexpr int ONE_IMPL = GL_ONE;
static constexpr int SRC_COLOR_IMPL = GL_SRC_COLOR;
static constexpr int ONE_MINUS_SRC_COLOR_IMPL = GL_ONE_MINUS_SRC_COLOR;
static constexpr int DST_COLOR_IMPL = GL_DST_COLOR;
static constexpr int ONE_MINUS_DST_COLOR_IMPL = GL_ONE_MINUS_DST_COLOR;
static constexpr int SRC_ALPHA_IMPL = GL_SRC_ALPHA;
static constexpr int ONE_MINUS_SRC_ALPHA_IMPL = GL_ONE_MINUS_SRC_ALPHA;
static constexpr int DST_ALPHA_IMPL = GL_DST_ALPHA;
static constexpr int ONE_MINUS_DST_ALPHA_IMPL = GL_ONE_MINUS_DST_ALPHA;
static constexpr int CONSTANT_COLOR_IMPL = GL_CONSTANT_COLOR;
static constexpr int ONE_MINUS_CONSTANT_COLOR_IMPL = GL_ONE_MINUS_CONSTANT_COLOR;
static constexpr int CONSTANT_ALPHA_IMPL = GL_CONSTANT_ALPHA;
static constexpr int ONE_MINUS_CONSTANT_ALPHA_IMPL = GL_ONE_MINUS_CONSTANT_ALPHA;

static constexpr int SYNC_FLUSH_COMMANDS_BIT = GL_SYNC_FLUSH_COMMANDS_BIT;

using ClipRect = WRect;
using Drawable = NativeGLDrawable;

enum class ShaderType : uint16_t
{
	VERTEX = GL_VERTEX_SHADER,
	FRAGMENT = GL_FRAGMENT_SHADER
};

enum class ColorSpace : uint8_t
{
	LINEAR = (uint8_t)GLColorSpace::LINEAR,
	SRGB = (uint8_t)GLColorSpace::SRGB,
};

using NativeBuffer = GLuint;

void destroyGLBuffer(RendererTask &, NativeBuffer);

struct GLBufferDeleter
{
	RendererTask *rTask{};

	void operator()(NativeBuffer s) const
	{
		destroyGLBuffer(*rTask, s);
	}
};
using UniqueGLBuffer = UniqueResource<NativeBuffer, GLBufferDeleter>;

struct TextureBinding
{
	TextureRef name{};
	GLenum target{};
};

struct TextureSizeSupport
{
	int maxXSize{}, maxYSize{};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, bool, true> nonPow2CanMipmap{};
	ConditionalMemberOr<(bool)Config::Gfx::OPENGL_ES, bool, true> nonPow2CanRepeat{};

	constexpr bool supportsMipmaps(int x, int y) const
	{
		return x && y && (nonPow2CanMipmap || (isPowerOf2(x) && isPowerOf2(y)));
	}
};

}
