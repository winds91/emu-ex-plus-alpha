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

#define LOGTAG "GLRenderer"
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Window.hh>
#include <imagine/util/ranges.hh>
#include "internalDefs.hh"
#include "utils.hh"
#ifdef __ANDROID__
#include "../../base/android/android.hh"
#include <imagine/gfx/opengl/android/egl.hh>
#endif
#include <string>
#include <cassert>
#include <cctype>
#include <format>

namespace IG::Gfx
{

[[gnu::weak]] const bool Renderer::enableSamplerObjects = false;

float rotationRadians(Rotation r)
{
	switch(r)
	{
		case Rotation::ANY:
		case Rotation::UP: return radians(0.);
		case Rotation::RIGHT: return radians(90.);
		case Rotation::DOWN: return radians(-180.);
		case Rotation::LEFT: return radians(-90.);
	}
	bug_unreachable("Rotation == %d", std::to_underlying(r));
}

static void printFeatures(DrawContextSupport support)
{
	if(!Config::DEBUG_BUILD)
		return;
	std::string featuresStr{};
	featuresStr.reserve(256);

	featuresStr.append(" [Texture Size:");
	featuresStr.append(std::format("{}", support.textureSizeSupport.maxXSize));
	featuresStr.append("]");
	if(support.textureSizeSupport.nonPow2CanRepeat)
		featuresStr.append("[NPOT Textures w/ Mipmap+Repeat]");
	else if(support.textureSizeSupport.nonPow2CanMipmap)
		featuresStr.append("[NPOT Textures w/ Mipmap]");
	#ifdef CONFIG_GFX_OPENGL_ES
	if(support.hasBGRPixels)
	{
		featuresStr.append(" [BGRA Format]");
	}
	#endif
	if(Config::Gfx::OPENGL_ES && support.hasTextureSwizzle)
	{
		featuresStr.append(" [Texture Swizzle]");
	}
	if(support.hasImmutableTexStorage)
	{
		featuresStr.append(" [Immutable Texture Storage]");
	}
	if(support.hasImmutableBufferStorage())
	{
		featuresStr.append(" [Immutable Buffer Storage]");
	}
	if(support.hasMemoryBarriers())
	{
		featuresStr.append(" [Memory Barriers]");
	}
	if(Config::Gfx::OPENGL_ES && support.hasUnpackRowLength)
	{
		featuresStr.append(" [Unpack Sub-Images]");
	}
	if(Config::Gfx::OPENGL_ES && support.hasSamplerObjects)
	{
		featuresStr.append(" [Sampler Objects]");
	}
	if(Config::Gfx::OPENGL_ES && support.hasVAOFuncs())
	{
		featuresStr.append(" [VAOs]");
	}
	if(Config::Gfx::OPENGL_ES && support.hasPBOFuncs)
	{
		featuresStr.append(" [PBOs]");
	}
	if(!Config::Gfx::OPENGL_ES || (Config::Gfx::OPENGL_ES && support.glMapBufferRange))
	{
		featuresStr.append(" [Map Buffer Range]");
	}
	if(support.hasSyncFences())
	{
		if(Config::GL_PLATFORM_EGL)
		{
			if(support.hasServerWaitSync())
				featuresStr.append(" [EGL Sync Fences + Server Sync]");
			else
				featuresStr.append(" [EGL Sync Fences]");
		}
		else
		{
			featuresStr.append(" [Sync Fences]");
		}
	}
	if(Config::Gfx::OPENGL_ES && support.hasSrgbWriteControl)
	{
		featuresStr.append(" [sRGB FB Write Control]");
	}
	featuresStr.append(" [GLSL:");
	featuresStr.append((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
	featuresStr.append("]");

	logMsg("features:%s", featuresStr.c_str());
}

#ifdef __ANDROID__
EGLImageKHR makeAndroidNativeBufferEGLImage(EGLDisplay dpy, EGLClientBuffer clientBuff, bool srgb)
{
	EGLint eglImgAttrs[]
	{
		EGL_IMAGE_PRESERVED_KHR,
		EGL_TRUE,
		srgb ? EGL_GL_COLORSPACE : EGL_NONE,
		srgb ? EGL_GL_COLORSPACE_SRGB : EGL_NONE,
		EGL_NONE
	};
	return eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
		clientBuff, eglImgAttrs);
}
#endif

void GLRenderer::setupNonPow2MipmapRepeatTextures()
{
	support.textureSizeSupport.nonPow2CanMipmap = true;
	support.textureSizeSupport.nonPow2CanRepeat = true;
}

void GLRenderer::setupImmutableTexStorage(bool extSuffix)
{
	if(support.hasImmutableTexStorage)
		return;
	support.hasImmutableTexStorage = true;
	#ifdef CONFIG_GFX_OPENGL_ES
	const char *procName = extSuffix ? "glTexStorage2DEXT" : "glTexStorage2D";
	support.glTexStorage2D = (typeof(support.glTexStorage2D))glManager.procAddress(procName);
	#endif
}

void GLRenderer::setupRGFormats()
{
	support.luminanceFormat = GL_RED;
	support.luminanceInternalFormat = GL_R8;
	support.luminanceAlphaFormat = GL_RG;
	support.luminanceAlphaInternalFormat = GL_RG8;
	support.alphaFormat = GL_RED;
	support.alphaInternalFormat = GL_R8;
}

void GLRenderer::setupSamplerObjects()
{
	if(!Renderer::enableSamplerObjects || support.hasSamplerObjects)
		return;
	support.hasSamplerObjects = true;
	#ifdef CONFIG_GFX_OPENGL_ES
	support.glGenSamplers = (typeof(support.glGenSamplers))glManager.procAddress("glGenSamplers");
	support.glDeleteSamplers = (typeof(support.glDeleteSamplers))glManager.procAddress("glDeleteSamplers");
	support.glBindSampler = (typeof(support.glBindSampler))glManager.procAddress("glBindSampler");
	support.glSamplerParameteri = (typeof(support.glSamplerParameteri))glManager.procAddress("glSamplerParameteri");
	#endif
}

void GLRenderer::setupSpecifyDrawReadBuffers()
{
	#ifdef CONFIG_GFX_OPENGL_ES
	//support.glDrawBuffers = (typeof(support.glDrawBuffers))glManager.procAddress("glDrawBuffers");
	//support.glReadBuffer = (typeof(support.glReadBuffer))glManager.procAddress("glReadBuffer");
	#endif
}

bool DrawContextSupport::hasDrawReadBuffers() const
{
	#ifdef CONFIG_GFX_OPENGL_ES
	return false; //glDrawBuffers;
	#else
	return true;
	#endif
}

#ifdef __ANDROID__
bool DrawContextSupport::hasEGLTextureStorage() const
{
	return glEGLImageTargetTexStorageEXT;
}
#endif

bool DrawContextSupport::hasImmutableBufferStorage() const
{
	#ifdef CONFIG_GFX_OPENGL_ES
	return glBufferStorage;
	#else
	return hasBufferStorage;
	#endif
}

bool DrawContextSupport::hasMemoryBarriers() const
{
	return false;
	/*#ifdef CONFIG_GFX_OPENGL_ES
	return glMemoryBarrier;
	#else
	return hasMemoryBarrier;
	#endif*/
}

bool DrawContextSupport::hasVAOFuncs() const
{
	if constexpr(Config::DEBUG_BUILD && forceNoVAOs)
		return false;
	#ifdef CONFIG_GFX_OPENGL_ES
	return glBindVertexArray;
	#else
	return true;
	#endif
}

void GLRenderer::setupUnmapBufferFunc()
{
	#ifdef CONFIG_GFX_OPENGL_ES
	if(!support.glUnmapBuffer)
	{
		if constexpr(Config::envIsAndroid || Config::envIsIOS)
		{
			support.glUnmapBuffer = (DrawContextSupport::UnmapBufferProto)glUnmapBufferOES;
		}
		else
		{
			if constexpr((bool)Config::Gfx::OPENGL_ES)
			{
				support.glUnmapBuffer = (typeof(support.glUnmapBuffer))glManager.procAddress("glUnmapBufferOES");
			}
			else
			{
				support.glUnmapBuffer = (typeof(support.glUnmapBuffer))glManager.procAddress("glUnmapBuffer");
			}
		}
	}
	#endif
}

void GLRenderer::setupImmutableBufferStorage()
{
	if(support.hasImmutableBufferStorage())
		return;
	#ifdef CONFIG_GFX_OPENGL_ES
	support.glBufferStorage = (typeof(support.glBufferStorage))glManager.procAddress("glBufferStorageEXT");
	#else
	support.hasBufferStorage = true;
	#endif
}

void GLRenderer::setupMemoryBarrier()
{
	/*if(support.hasMemoryBarriers())
		return;
	#ifdef CONFIG_GFX_OPENGL_ES
	support.glMemoryBarrier = (typeof(support.glMemoryBarrier))glManager.procAddress("glMemoryBarrier");
	#else
	support.hasMemoryBarrier = true;
	#endif*/
}

void GLRenderer::setupVAOFuncs(bool oes)
{
	#ifdef CONFIG_GFX_OPENGL_ES
	if(support.glBindVertexArray)
		return;
	if(oes)
	{
		support.glBindVertexArray = (typeof(support.glBindVertexArray))glManager.procAddress("glBindVertexArrayOES");
		support.glGenVertexArrays = (typeof(support.glGenVertexArrays))glManager.procAddress("glGenVertexArraysOES");
		support.glDeleteVertexArrays = (typeof(support.glDeleteVertexArrays))glManager.procAddress("glDeleteVertexArraysOES");
	}
	else
	{
		support.glBindVertexArray = (typeof(support.glBindVertexArray))glManager.procAddress("glBindVertexArray");
		support.glGenVertexArrays = (typeof(support.glGenVertexArrays))glManager.procAddress("glGenVertexArrays");
		support.glDeleteVertexArrays = (typeof(support.glDeleteVertexArrays))glManager.procAddress("glDeleteVertexArrays");
	}
	#endif
}

void GLRenderer::checkExtensionString(std::string_view extStr)
{
	//logMsg("checking %s", extStr);
	if(Config::DEBUG_BUILD && Config::Gfx::OPENGL_DEBUG_CONTEXT && extStr == "GL_KHR_debug")
	{
		support.hasDebugOutput = true;
		#ifdef __ANDROID__
		// older GPU drivers like Tegra 3 can crash when using debug output,
		// only enable on recent Android version to be safe
		if(mainTask.appContext().androidSDK() < 23)
		{
			support.hasDebugOutput = false;
		}
		#endif
	}
	#ifdef CONFIG_GFX_OPENGL_ES
	else if(extStr == "GL_OES_texture_npot")
	{
		// allows mipmaps and repeat modes
		setupNonPow2MipmapRepeatTextures();
	}
	else if(!Config::envIsIOS && extStr == "GL_NV_texture_npot_2D_mipmap")
	{
		// no repeat modes
		support.textureSizeSupport.nonPow2CanMipmap = true;
	}
	else if(extStr == "GL_EXT_unpack_subimage")
	{
		support.hasUnpackRowLength = true;
	}
	else if((!Config::envIsIOS && extStr == "GL_EXT_texture_format_BGRA8888")
			|| (Config::envIsIOS && extStr == "GL_APPLE_texture_format_BGRA8888"))
	{
		support.hasBGRPixels = true;
	}
	else if(extStr == "GL_EXT_texture_storage")
	{
		setupImmutableTexStorage(true);
	}
	else if(!Config::GL_PLATFORM_EGL && Config::envIsIOS && extStr == "GL_APPLE_sync")
	{
		setupAppleFenceSync();
	}
	else if(Config::envIsAndroid && extStr == "GL_OES_EGL_image")
	{
		support.hasEGLImages = true;
	}
	else if(Config::Gfx::OPENGL_TEXTURE_TARGET_EXTERNAL &&
		extStr == "GL_OES_EGL_image_external")
	{
		support.hasExternalEGLImages = true;
	}
	#ifdef __ANDROID__
	else if(extStr == "GL_EXT_EGL_image_storage")
	{
		support.glEGLImageTargetTexStorageEXT = (typeof(support.glEGLImageTargetTexStorageEXT))glManager.procAddress("glEGLImageTargetTexStorageEXT");
	}
	#endif
	else if(extStr == "GL_NV_pixel_buffer_object")
	{
		support.hasPBOFuncs = true;
	}
	else if(extStr == "GL_NV_map_buffer_range")
	{
		if(!support.glMapBufferRange)
			support.glMapBufferRange = (typeof(support.glMapBufferRange))glManager.procAddress("glMapBufferRangeNV");
		setupUnmapBufferFunc();
	}
	else if(extStr == "GL_EXT_map_buffer_range")
	{
		if(!support.glMapBufferRange)
			support.glMapBufferRange = (typeof(support.glMapBufferRange))glManager.procAddress("glMapBufferRangeEXT");
		// Only using ES 3.0 version currently
		//if(!support.glFlushMappedBufferRange)
		//	support.glFlushMappedBufferRange = (typeof(support.glFlushMappedBufferRange))glManager.procAddress("glFlushMappedBufferRangeEXT");
		setupUnmapBufferFunc();
	}
	else if(extStr == "GL_EXT_buffer_storage")
	{
		setupImmutableBufferStorage();
	}
	/*else if(string_equal(extStr, "GL_OES_mapbuffer"))
	{
		// handled in *_map_buffer_range currently
	}*/
	else if(extStr == "GL_EXT_sRGB_write_control")
	{
		support.hasSrgbWriteControl = true;
	}
	else if(extStr == "GL_OES_vertex_array_object")
	{
		setupVAOFuncs(true);
	}
	#endif
	#ifndef CONFIG_GFX_OPENGL_ES
	/*else if(string_equal(extStr, "GL_EXT_texture_filter_anisotropic"))
	{
		setupAnisotropicFiltering();
	}
	else if(string_equal(extStr, "GL_ARB_multisample"))
	{
		setupMultisample();
	}
	else if(string_equal(extStr, "GL_NV_multisample_filter_hint"))
	{
		setupMultisampleHints();
	}*/
	else if(extStr == "GL_ARB_texture_storage")
	{
		setupImmutableTexStorage(false);
	}
	else if(extStr == "GL_ARB_buffer_storage")
	{
		setupImmutableBufferStorage();
	}
	else if(extStr == "GL_ARB_shader_image_load_store")
	{
		setupMemoryBarrier();
	}
	#endif
}

void GLRenderer::checkFullExtensionString(const char *fullExtStr)
{
	std::string fullExtStrTemp{fullExtStr};
	char *savePtr;
	auto extStr = strtok_r(fullExtStrTemp.data(), " ", &savePtr);
	while(extStr)
	{
		checkExtensionString(extStr);
		extStr = strtok_r(nullptr, " ", &savePtr);
	}
}

static int glVersionFromStr(const char *versionStr)
{
	// skip to version number
	while(!isdigit(*versionStr) && *versionStr != '\0')
		versionStr++;
	int major = 1, minor = 0;
	if(sscanf(versionStr, "%d.%d", &major, &minor) != 2)
	{
		logErr("unable to parse GL version string");
	}
	return 10 * major + minor;
}

void Renderer::configureRenderer()
{
	if(Config::DEBUG_BUILD && defaultToFullErrorChecks)
	{
		setCorrectnessChecks(true);
	}
	task().runSync(
		[this](GLTask::TaskContext ctx)
		{
			auto version = (const char*)glGetString(GL_VERSION);
			assert(version);
			auto rendererName = (const char*)glGetString(GL_RENDERER);
			logMsg("version: %s (%s)", version, rendererName);

			int glVer = glVersionFromStr(version);

			#ifdef CONFIG_BASE_GL_PLATFORM_EGL
			if constexpr((bool)Config::Gfx::OPENGL_ES)
			{
				auto extStr = ctx.glDisplay.queryExtensions();
				setupEglFenceSync(extStr);
			}
			#endif

			#ifndef CONFIG_GFX_OPENGL_ES
			assert(glVer >= 33);
			// extension functionality
			GLint numExtensions;
			glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
			if(Config::DEBUG_BUILD)
			{
				logMsgNoBreak("extensions: ");
				for(auto i : iotaCount(numExtensions))
				{
					logger_printf(LOG_M, "%s ", (const char*)glGetStringi(GL_EXTENSIONS, i));
				}
				logger_printf(LOG_M, "\n");
			}
			for(auto i : iotaCount(numExtensions))
			{
				checkExtensionString((const char*)glGetStringi(GL_EXTENSIONS, i));
			}
			#else
			// core functionality
			assumeExpr(glVer >= 20);
			if(glVer >= 30)
				setupNonPow2MipmapRepeatTextures();
			if(glVer >= 30)
			{
				support.glMapBufferRange = (typeof(support.glMapBufferRange))glManager.procAddress("glMapBufferRange");
				support.glUnmapBuffer = (typeof(support.glUnmapBuffer))glManager.procAddress("glUnmapBuffer");
				support.glFlushMappedBufferRange = (typeof(support.glFlushMappedBufferRange))glManager.procAddress("glFlushMappedBufferRange");
				setupImmutableTexStorage(false);
				support.hasTextureSwizzle = true;
				setupRGFormats();
				setupSamplerObjects();
				support.hasPBOFuncs = true;
				setupVAOFuncs();
				if(!Config::GL_PLATFORM_EGL)
					setupFenceSync();
				if(!Config::envIsIOS)
					setupSpecifyDrawReadBuffers();
				support.hasUnpackRowLength = true;
				support.useLegacyGLSL = false;
			}
			if(glVer >= 31)
			{
				setupMemoryBarrier();
			}

			// extension functionality
			auto extensions = (const char*)glGetString(GL_EXTENSIONS);
			assert(extensions);
			logMsg("extensions: %s", extensions);
			checkFullExtensionString(extensions);
			#endif // CONFIG_GFX_OPENGL_ES

			GLint texSize;
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
			support.textureSizeSupport.maxXSize = support.textureSizeSupport.maxYSize = texSize;
			assert(support.textureSizeSupport.maxXSize > 0 && support.textureSizeSupport.maxYSize > 0);

			printFeatures(support);
			task().runInitialCommandsInGL(ctx, support);
		});
	support.isConfigured = true;
}

bool Renderer::isConfigured() const
{
	return support.isConfigured;
}

const RendererTask &Renderer::task() const
{
	return mainTask;
}

RendererTask &Renderer::task()
{
	return mainTask;
}

void Renderer::setWindowValidOrientations(Window &win, Orientations validO)
{
	if(!win.isMainWindow())
		return;
	auto oldWinO = win.softOrientation();
	if(win.setValidOrientations(validO) && !Config::SYSTEM_ROTATES_WINDOWS)
	{
		animateWindowRotation(win, rotationRadians(oldWinO), rotationRadians(win.softOrientation()));
	}
}

void GLRenderer::addEventHandlers(ApplicationContext ctx, RendererTask &task)
{
	releaseShaderCompilerEvent.attach([&task, ctx]()
	{
		if(!ctx.isRunning())
			return;
		logMsg("automatically releasing shader compiler");
		task.releaseShaderCompiler();
	});
	if constexpr(Config::envIsIOS)
		task.setIOSDrawableDelegates();
}

std::optional<GLBufferConfig> GLRenderer::makeGLBufferConfig(ApplicationContext ctx, IG::PixelFormat pixelFormat, const Window *winPtr)
{
	if(!pixelFormat)
	{
		if(winPtr)
			pixelFormat = winPtr->pixelFormat();
		else
			pixelFormat = ctx.defaultWindowPixelFormat();
	}
	GLBufferConfigAttributes glBuffAttr{.pixelFormat = pixelFormat};
	if constexpr(Config::Gfx::OPENGL_ES)
	{
		if(auto config = glManager.makeBufferConfig(ctx, glBuffAttr, glAPI, 3);
			config)
		{
			return config;
		}
		// fall back to OpenGL ES 2.0
		return glManager.makeBufferConfig(ctx, glBuffAttr, glAPI, 2);
	}
	else
	{
		// full OpenGL
		return glManager.makeBufferConfig(ctx, glBuffAttr, glAPI);
	}
}

}
