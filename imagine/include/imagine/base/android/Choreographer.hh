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
#include <imagine/base/baseDefs.hh>
#include <imagine/util/jni.hh>
#include <variant>

struct AChoreographer;

namespace IG
{

class AndroidApplication;

class NativeChoreographer
{
public:
	constexpr NativeChoreographer() = default;
	NativeChoreographer(AndroidApplication &);
	void scheduleVSync();
	explicit constexpr operator bool() const { return choreographer; }

protected:
	using AChoreographerFrameCallback = void (*)(long frameTimeNanos, void* data);
	using PostFrameCallbackFunc = void (*)(AChoreographer*, AChoreographerFrameCallback, void* data);

	AndroidApplication *appPtr{};
	AChoreographer *choreographer{};
	PostFrameCallbackFunc postFrameCallback{};
	bool requested{};
};

class JavaChoreographer
{
public:
	constexpr JavaChoreographer() = default;
	JavaChoreographer(AndroidApplication &, JNIEnv *, jobject baseActivity, jclass baseActivityClass);
	void scheduleVSync();
	explicit constexpr operator bool() const { return frameHelper; }

protected:
	AndroidApplication *appPtr{};
	JNI::UniqueGlobalRef frameHelper{};
	JNI::InstMethod<void()> jPostFrame{};
	bool requested{};
};

template <class ChoreographerBase>
class ChoreographerFrameTimer final
{
public:
	constexpr ChoreographerFrameTimer() = default;
	ChoreographerFrameTimer(ChoreographerBase &choreographer):
		choreographerPtr{&choreographer} {}
	void scheduleVSync() { choreographerPtr->scheduleVSync(); }
	void cancel() {}
	void setFrameRate(FrameRate) {}

protected:
	ChoreographerBase *choreographerPtr{};
};

using Choreographer = std::variant<NativeChoreographer, JavaChoreographer>;
using NativeChoreographerFrameTimer = ChoreographerFrameTimer<NativeChoreographer>;
using JavaChoreographerFrameTimer = ChoreographerFrameTimer<JavaChoreographer>;

}
