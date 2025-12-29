/*  This file is part of NGP.emu.

	NGP.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NGP.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NGP.emu.  If not, see <http://www.gnu.org/licenses/> */

#include "MainSystem.hh"
#include <mednafen-emuex/MDFNUtils.hh>
#include <mednafen/general.h>
import emuex;

namespace EmuEx
{

const char *EmuSystem::configFilename = "NgpEmu.config";

std::span<const AspectRatioInfo> NgpSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"20:19 (Original)", {20, 19}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

bool NgpSystem::readConfig(ConfigType type, MapIO &io, unsigned key)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_NGPKEY_LANGUAGE: return readOptionValue(io, optionNGPLanguage);
			case CFGKEY_NO_MD5_FILENAMES: return readOptionValue(io, noMD5InFilenames);
		}
	}
	return false;
}

void NgpSystem::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		writeOptionValueIfNotDefault(io, optionNGPLanguage);
		writeOptionValueIfNotDefault(io, CFGKEY_NO_MD5_FILENAMES, noMD5InFilenames, false);
	}
}

}

namespace Mednafen
{

#define EMU_MODULE "ngp"
constexpr IG::SystemLogger log{"NGP.emu"};

using namespace EmuEx;

uint64 MDFN_GetSettingUI(const char *name)
{
	log.error("unhandled settingUI {}", name);
	unreachable();
}

int64 MDFN_GetSettingI(const char *name_)
{
	std::string_view name{name_};
	if("filesys.state_comp_level" == name)
		return 6;
	log.error("unhandled settingI {}", name_);
	unreachable();
}

double MDFN_GetSettingF(const char *name)
{
	log.error("unhandled settingF {}", name);
	unreachable();
}

bool MDFN_GetSettingB(const char *name_)
{
	std::string_view name{name_};
	if("cheats" == name)
		return 0;
	if(EMU_MODULE".language" == name)
		return static_cast<NgpSystem&>(gSystem()).optionNGPLanguage;
	if("filesys.untrusted_fip_check" == name)
		return 0;
	log.error("unhandled settingB {}", name_);
	unreachable();
}

std::string MDFN_GetSettingS(const char *name)
{
	log.error("unhandled settingS {}", name);
	unreachable();
}

std::string MDFN_MakeFName(MakeFName_Type type, int id1, const char *cd1)
{
	switch(type)
	{
		case MDFNMKF_STATE:
		case MDFNMKF_SAV:
		case MDFNMKF_SAVBACK:
			return savePathMDFN(id1, cd1);
		default: unreachable();
	}
}

}
