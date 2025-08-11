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
#include <emuframework/EmuTiming.hh>
#include <imagine/gui/View.hh>
#include <imagine/time/Time.hh>
#include <imagine/gfx/GfxText.hh>
#include <imagine/gfx/Quads.hh>

namespace EmuEx
{

using namespace IG;
class EmuInputView;
class EmuVideoLayer;
class EmuSystem;
struct FrameTimingStats;
struct FrameRateConfig;
struct AudioStats;

struct FrameTimingViewStats
{
	FrameTimingStats stats;
	SteadyClockTimePoint lastFrameTime;
	FrameRate inputRate, outputRate;
};

class EmuView : public View
{
public:
	EmuView(ViewAttachParams, EmuVideoLayer *, EmuSystem &);
	void place() final;
	void prepareDraw() final;
	void draw(Gfx::RendererCommands &__restrict__, ViewDrawParams p = {}) const final;
	void drawStatsText(Gfx::RendererCommands& __restrict__);
	bool hasLayer() const { return layer; }
	void setLayoutInputView(EmuInputView *view) { inputView = view; }
	void setShowFrameTimingStats(bool);
	void setShowAudioStats(bool);
	bool showingFrameTimingStats() const { return showFrameTimingStats; }
	bool showingAudioStats() const { return showAudioStats; }
	void setFrameTimingStats(FrameTimingViewStats);
	void setAudioStats(AudioStats);
	EmuVideoLayer *videoLayer() const { return layer; }
	auto& system(this auto&& self) { return *self.sysPtr; }

private:
	struct StatsDisplay
	{
		Gfx::Text text;
		Gfx::IQuads bgQuads;
		WRect rect{};
	};

	EmuVideoLayer *layer{};
	EmuInputView *inputView{};
	EmuSystem *sysPtr{};
	std::string frameTimingStatsStr;
	std::string audioStatsStr;
	StatsDisplay statsDisplay;
	bool showStats{};
	bool showFrameTimingStats{};
	bool showAudioStats{};

	void updateShowStats() { showStats = showFrameTimingStats || showAudioStats; }
	void updateStatsDisplay();
	void placeStats();
};

}
