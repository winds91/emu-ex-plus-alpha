#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/config.hh>
#include <emuframework/EmuAppHelper.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/util/container/ArrayList.hh>

namespace EmuEx
{

using namespace IG;
class EmuVideoLayer;
class EmuVideo;
enum class ImageEffectId : uint8_t;
enum class ImageChannel : uint8_t;
enum class VideoSystem: uint8_t;

class VideoOptionView : public TableView, public EmuAppHelper<VideoOptionView>
{
public:
	VideoOptionView(ViewAttachParams attach, bool customMenu = false);
	void place() final;
	void loadStockItems();
	void setEmuVideoLayer(EmuVideoLayer &videoLayer);

protected:
	static constexpr int MAX_ASPECT_RATIO_ITEMS = 5;
	EmuVideoLayer *videoLayer{};

	StaticArrayList<TextMenuItem, 5> textureBufferModeItem;
	MultiChoiceMenuItem textureBufferMode;
	TextMenuItem frameIntervalItem[5];
	MultiChoiceMenuItem frameInterval;
	TextMenuItem frameRateItems[4];
	VideoSystem activeVideoSystem{};
	MultiChoiceMenuItem frameRate;
	MultiChoiceMenuItem frameRatePAL;
	IG_UseMemberIf(enableFrameTimeStats, BoolMenuItem, frameTimeStats);
	StaticArrayList<TextMenuItem, MAX_ASPECT_RATIO_ITEMS> aspectRatioItem;
	MultiChoiceMenuItem aspectRatio;
	TextMenuItem zoomItem[6];
	MultiChoiceMenuItem zoom;
	TextMenuItem viewportZoomItem[4];
	MultiChoiceMenuItem viewportZoom;
	TextMenuItem contentRotationItem[5];
	MultiChoiceMenuItem contentRotation;
	TextMenuItem placeVideo;
	BoolMenuItem imgFilter;
	TextMenuItem imgEffectItem[6];
	MultiChoiceMenuItem imgEffect;
	TextMenuItem overlayEffectItem[8];
	MultiChoiceMenuItem overlayEffect;
	TextMenuItem overlayEffectLevelItem[5];
	MultiChoiceMenuItem overlayEffectLevel;
	TextMenuItem imgEffectPixelFormatItem[3];
	MultiChoiceMenuItem imgEffectPixelFormat;
	StaticArrayList<TextMenuItem, 4> windowPixelFormatItem;
	MultiChoiceMenuItem windowPixelFormat;
	IG_UseMemberIf(Config::envIsLinux && Config::BASE_MULTI_WINDOW, BoolMenuItem, secondDisplay);
	IG_UseMemberIf(Config::BASE_MULTI_SCREEN && Config::BASE_MULTI_WINDOW, BoolMenuItem, showOnSecondScreen);
	TextMenuItem imageBuffersItem[3];
	MultiChoiceMenuItem imageBuffers;
	TextMenuItem frameClockItems[3];
	MultiChoiceMenuItem frameClock;
	IG_UseMemberIf(Gfx::supportsPresentModes, TextMenuItem, presentModeItems[3]);
	IG_UseMemberIf(Gfx::supportsPresentModes, MultiChoiceMenuItem, presentMode);
	TextMenuItem renderPixelFormatItem[3];
	MultiChoiceMenuItem renderPixelFormat;
	IG_UseMemberIf(Config::multipleScreenFrameRates, std::vector<TextMenuItem>, screenFrameRateItems);
	IG_UseMemberIf(Config::multipleScreenFrameRates, MultiChoiceMenuItem, screenFrameRate);
	IG_UseMemberIf(Gfx::supportsPresentationTime, TextMenuItem, presentationTimeItems[3]);
	IG_UseMemberIf(Gfx::supportsPresentationTime, MultiChoiceMenuItem, presentationTime);
	BoolMenuItem blankFrameInsertion;
	TextMenuItem brightnessItem[2];
	TextMenuItem redItem[2];
	TextMenuItem greenItem[2];
	TextMenuItem blueItem[2];
	TextMenuItem brightness;
	MultiChoiceMenuItem red;
	MultiChoiceMenuItem green;
	MultiChoiceMenuItem blue;
	TextHeadingMenuItem visualsHeading;
	TextHeadingMenuItem screenShapeHeading;
	TextHeadingMenuItem colorLevelsHeading;
	TextHeadingMenuItem advancedHeading;
	TextHeadingMenuItem systemSpecificHeading;
	StaticArrayList<MenuItem*, 41> item;

	bool onFrameTimeChange(VideoSystem vidSys, SteadyClockTime time);
	TextMenuItem::SelectDelegate setVideoBrightnessCustomDel(ImageChannel);
	void setAllColorLevelsSelected(MenuId);
	EmuVideo &emuVideo() const;
};

}
