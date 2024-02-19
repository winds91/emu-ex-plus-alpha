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

#if defined CONFIG_PACKAGE_X11
#include <imagine/base/x11/XApplication.hh>
#elif defined __ANDROID__
#include <imagine/base/android/AndroidApplication.hh>
#elif defined CONFIG_OS_IOS
#include <imagine/base/iphone/IOSApplication.hh>
#endif

namespace IG
{

class Application : public ApplicationImpl
{
public:
	using ApplicationImpl::ApplicationImpl;
	void runOnMainThread(MainThreadMessageDelegate);
	void flushMainThreadMessages();
};

}
