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

#ifdef IMAGINE_CONFIG_H
#define IMAGINE_CONFIG_H_INCLUDE <IMAGINE_CONFIG_H>
#include IMAGINE_CONFIG_H_INCLUDE
#undef IMAGINE_CONFIG_H_INCLUDE
#else
	#if __has_include (<imagine-config.h>)
	#include <imagine-config.h>
	#endif
#endif

#if defined __APPLE__
#include <TargetConditionals.h>
	#if TARGET_OS_IPHONE
	#define CONFIG_OS_IOS
	#endif
#endif
#ifdef __ANDROID__
#include <android/api-level.h>
#endif
#include <cstdint>

namespace Config
{

// TODO: have to use ANDROID_ for now since ANDROID is needed as a macro in some system headers not yet using __ANDROID__
enum { UNKNOWN, ANDROID_, IOS, MACOSX, LINUX, WIN32_ };
inline constexpr uint32_t ENV =
	#if defined __ANDROID__
	ANDROID_;
	#elif defined __APPLE__ && TARGET_OS_IPHONE
	IOS;
	#elif defined __APPLE__ && TARGET_OS_MAC
	MACOSX;
	#elif defined __linux__
	LINUX;
	#elif defined _WIN32
	WIN32_;
	#else
	#warning "Unknown ENV type"
	UNKNOWN;
	#endif

inline constexpr bool envIsAndroid = ENV == ANDROID_;
inline constexpr bool envIsIOS = ENV == IOS;
inline constexpr bool envIsMacOSX = ENV == MACOSX;
inline constexpr bool envIsLinux = ENV == LINUX;
inline constexpr bool is64Bit = sizeof(void*) == 8;

#if defined __ANDROID__
	#ifdef ANDROID_COMPAT_API
	#define ANDROID_MIN_API ANDROID_COMPAT_API
	#else
	#define ANDROID_MIN_API __ANDROID_API__
	#endif

inline constexpr int ENV_ANDROID_MIN_SDK = ANDROID_MIN_API;
#define ENV_NOTE "Android"
#else
inline constexpr int ENV_ANDROID_MIN_SDK = 0;
#endif

#ifndef NDEBUG
inline constexpr bool DEBUG_BUILD = true;
#else
inline constexpr bool DEBUG_BUILD = false;
#endif

// Platform architecture & machine

enum Machine
{
	GENERIC,
	GENERIC_X86,
	GENERIC_X86_64,
	GENERIC_ARM,
	GENERIC_ARMV5,
	GENERIC_ARMV6,
	GENERIC_ARMV7,
	GENERIC_ARMV7S,
	GENERIC_AARCH64,
	GENERIC_PPC,
	GENERIC_MIPS,
	PANDORA,
};

#if defined __x86_64__
inline constexpr Machine MACHINE = GENERIC_X86_64;
#define CONFIG_ARCH_STR "x86_64"
#elif defined __i386__
inline constexpr Machine MACHINE = GENERIC_X86;
#define CONFIG_ARCH_STR "x86"
#elif defined __powerpc__
inline constexpr Machine MACHINE = GENERIC_PPC;
#define CONFIG_ARCH_STR "ppc"
#elif defined __mips__
inline constexpr Machine MACHINE = GENERIC_MIPS;
#define CONFIG_ARCH_STR "mips"
#elif defined __aarch64__
inline constexpr Machine MACHINE = GENERIC_AARCH64;
#define CONFIG_ARCH_STR "aarch64"
#elif __arm__
	#if defined __ARM_ARCH_7S__
	inline constexpr Machine MACHINE = GENERIC_ARMV7S;
	#define CONFIG_ARCH_STR "armv7s"
	#elif defined __ARM_ARCH_5TE__
	// default Android "ARM" profile
	inline constexpr Machine MACHINE = GENERIC_ARMV5;
	#define CONFIG_ARCH_STR "armv5te"
	#elif __ARM_ARCH == 7
	// default Android & iOS ARMv7 profile -> __ARM_ARCH_7A__
		#if defined CONFIG_MACHINE_PANDORA
		inline constexpr Machine MACHINE = PANDORA;
		#else
		inline constexpr Machine MACHINE = GENERIC_ARMV7;
		#endif
	#define CONFIG_ARCH_STR "armv7"
	#elif __ARM_ARCH == 6
	// default iOS ARMv6 profile -> __ARM_ARCH_6K__
	inline constexpr Machine MACHINE = GENERIC_ARMV6;
	#define CONFIG_ARCH_STR "armv6"
	#else
	inline constexpr Machine MACHINE = GENERIC_ARM;
	#define CONFIG_ARCH_STR "arm"
	#endif
#else
inline constexpr Machine MACHINE = GENERIC;
#warning Compiling on unknown architecture
#endif

#ifdef __ARM_ARCH
inline constexpr int ARM_ARCH = __ARM_ARCH;
#else
inline constexpr int ARM_ARCH = 0;
#endif

inline constexpr bool MACHINE_IS_GENERIC_X86 = MACHINE == GENERIC_X86;
inline constexpr bool MACHINE_IS_GENERIC_ARMV6 = MACHINE == GENERIC_ARMV6;
inline constexpr bool MACHINE_IS_GENERIC_ARMV7 = MACHINE == GENERIC_ARMV7;
inline constexpr bool MACHINE_IS_GENERIC_AARCH64 = MACHINE == GENERIC_AARCH64;
inline constexpr bool MACHINE_IS_PANDORA = MACHINE == PANDORA;

	namespace Input
	{
	#if (defined __APPLE__ && TARGET_OS_IPHONE) || defined __ANDROID__
	inline constexpr bool SYSTEM_COLLECTS_TEXT = true;
	#else
	inline constexpr bool SYSTEM_COLLECTS_TEXT = false;
	#endif

	// dynamic input device list from system
	#if defined CONFIG_PACKAGE_X11 || defined __ANDROID__ || defined __APPLE__
	inline constexpr bool DEVICE_HOTSWAP = true;
	#else
	inline constexpr bool DEVICE_HOTSWAP = false;
	#endif

	inline constexpr bool KEYBOARD_DEVICES = true;

	// mouse & touch
	inline constexpr bool POINTING_DEVICES = true;

	#if defined CONFIG_PACKAGE_X11 || defined __ANDROID__ || defined _WIN32
	inline constexpr bool MOUSE_DEVICES = true;
	#else
	inline constexpr bool MOUSE_DEVICES = false;
	#endif

	#if defined CONFIG_PACKAGE_X11 || defined __ANDROID__ || defined _WIN32
	#define CONFIG_INPUT_GAMEPAD_DEVICES
	inline constexpr bool GAMEPAD_DEVICES = true;
	#else
	inline constexpr bool GAMEPAD_DEVICES = false;
	#endif

	#if (defined __APPLE__ && TARGET_OS_IPHONE) || defined __ANDROID__
	inline constexpr bool TOUCH_DEVICES = true;
	#else
	inline constexpr bool TOUCH_DEVICES = false;
	#endif

	inline constexpr int MAX_POINTERS =
	#if defined CONFIG_PACKAGE_X11
	4; // arbitrary max
	#elif defined CONFIG_OS_IOS || defined __ANDROID__
	// arbitrary max
	7;
	#else
	1;
	#endif

	// relative motion/trackballs
	#ifdef __ANDROID__
	inline constexpr bool RELATIVE_MOTION_DEVICES = true;
	#else
	inline constexpr bool RELATIVE_MOTION_DEVICES = false;
	#endif

	#if defined CONFIG_PACKAGE_X11 || defined __ANDROID__ || defined __APPLE__
	inline constexpr bool BLUETOOTH = true;
	#define CONFIG_INPUT_BLUETOOTH
	#else
	inline constexpr bool BLUETOOTH = false;
	#endif
	}

}
