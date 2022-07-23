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
#include <imagine/util/rectangle2.h>
#include <imagine/util/used.hh>
#include <imagine/util/2DOrigin.h>
#include <compare>

namespace IG
{

class Viewport
{
public:
	constexpr Viewport() = default;
	constexpr Viewport(WRect originRect, WRect rect, Orientation softOrientation = VIEW_ROTATE_0):
		originRect{originRect}, rect{rect}, softOrientation_{softOrientation} {}
	constexpr Viewport(WRect rect, Orientation softOrientation = VIEW_ROTATE_0):
		Viewport{rect, rect, softOrientation} {}
	constexpr Viewport(WP size):
		Viewport{{{}, size}} {}
	constexpr WRect realBounds() const { return orientationIsSideways(softOrientation_) ? bounds().makeInverted() : bounds(); }
	constexpr WRect bounds() const { return rect; }
	constexpr WRect originBounds() const { return originRect; }
	constexpr int realWidth() const { return orientationIsSideways(softOrientation_) ? height() : width(); }
	constexpr int realHeight() const { return orientationIsSideways(softOrientation_) ? width() : height(); }
	constexpr int width() const { return rect.xSize(); }
	constexpr int height() const { return rect.ySize(); }
	constexpr float aspectRatio() const { return (float)width() / (float)height(); }
	constexpr float realAspectRatio() const { return (float)realWidth() / (float)realHeight(); }
	constexpr bool isPortrait() const { return width() < height(); }
	constexpr bool operator==(Viewport const &) const = default;
	WRect relRect(WP pos, WP size, _2DOrigin posOrigin, _2DOrigin screenOrigin) const;
	WRect relRectBestFit(WP pos, float aspectRatio, _2DOrigin posOrigin, _2DOrigin screenOrigin) const;

private:
	WRect originRect{};
	WRect rect{};
	IG_UseMemberIfOrConstant(!Config::SYSTEM_ROTATES_WINDOWS, Orientation,
		VIEW_ROTATE_0, softOrientation_){VIEW_ROTATE_0};
};

}