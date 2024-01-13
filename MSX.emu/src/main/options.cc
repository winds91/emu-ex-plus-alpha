/*  This file is part of MSX.emu.

	MSX.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MSX.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MSX.emu.  If not, see <http://www.gnu.org/licenses/> */

#include "MainSystem.hh"
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/util/format.hh>

namespace EmuEx
{

constexpr SystemLogger log{"MSXOpt"};

enum
{
	CFGKEY_DEFAULT_MACHINE_NAME = 256, CFGKEY_SKIP_FDC_ACCESS = 257,
	CFGKEY_MACHINE_FILE_PATH = 258, CFGKEY_SESSION_MACHINE_NAME = 259,
	CFGKEY_MIXER_PSG_VOLUME = 260, CFGKEY_MIXER_PSG_PAN = 261,
	CFGKEY_MIXER_SCC_VOLUME = 262, CFGKEY_MIXER_SCC_PAN = 263,
	CFGKEY_MIXER_MSX_MUSIC_VOLUME = 264, CFGKEY_MIXER_MSX_MUSIC_PAN = 265,
	CFGKEY_MIXER_MSX_AUDIO_VOLUME = 266, CFGKEY_MIXER_MSX_AUDIO_PAN = 267,
	CFGKEY_MIXER_MOON_SOUND_VOLUME = 268, CFGKEY_MIXER_MOON_SOUND_PAN = 269,
	CFGKEY_MIXER_YAMAHA_SFG_VOLUME = 270, CFGKEY_MIXER_YAMAHA_SFG_PAN = 271,
	CFGKEY_MIXER_KEYBOARD_VOLUME = 272, CFGKEY_MIXER_KEYBOARD_PAN = 273,
	CFGKEY_MIXER_PCM_VOLUME = 274, CFGKEY_MIXER_PCM_PAN = 275,
	CFGKEY_MIXER_IO_VOLUME = 276, CFGKEY_MIXER_IO_PAN = 277,
	CFGKEY_MIXER_MIDI_VOLUME = 278, CFGKEY_MIXER_MIDI_PAN = 279,
	CFGKEY_DEFAULT_COLECO_MACHINE_NAME = 280
};

// volume options use top bit as enable switch, lower 7 bits as volume value
static constexpr uint8_t MIXER_ENABLE_BIT = IG::bit(7);
static constexpr uint8_t MIXER_VOLUME_MASK = 0x7F;

static bool volumeOptionIsValid(uint8_t val)
{
	val &= MIXER_VOLUME_MASK;
	return val <= 100;
}

static bool panOptionIsValid(uint8_t val)
{
	return val <= 100;
}

const char *EmuSystem::configFilename = "MsxEmu.config";
int EmuSystem::forcedSoundRate = 44100;
Byte1Option optionSkipFdcAccess{CFGKEY_SKIP_FDC_ACCESS, 1};

Byte1Option optionMixerPSGVolume{CFGKEY_MIXER_PSG_VOLUME, 100 | MIXER_ENABLE_BIT, false, volumeOptionIsValid};
Byte1Option optionMixerSCCVolume{CFGKEY_MIXER_SCC_VOLUME, 100 | MIXER_ENABLE_BIT, false, volumeOptionIsValid};
Byte1Option optionMixerMSXMUSICVolume{CFGKEY_MIXER_MSX_MUSIC_VOLUME, 80 | MIXER_ENABLE_BIT, false, volumeOptionIsValid};
Byte1Option optionMixerMSXAUDIOVolume{CFGKEY_MIXER_MSX_AUDIO_VOLUME, 80 | MIXER_ENABLE_BIT, false, volumeOptionIsValid};
Byte1Option optionMixerMoonSoundVolume{CFGKEY_MIXER_MOON_SOUND_VOLUME, 80 | MIXER_ENABLE_BIT, false, volumeOptionIsValid};
Byte1Option optionMixerYamahaSFGVolume{CFGKEY_MIXER_YAMAHA_SFG_VOLUME, 80 | MIXER_ENABLE_BIT, false, volumeOptionIsValid};
Byte1Option optionMixerPCMVolume{CFGKEY_MIXER_PCM_VOLUME, 100 | MIXER_ENABLE_BIT, false, volumeOptionIsValid};

Byte1Option optionMixerPSGPan{CFGKEY_MIXER_PSG_PAN, 50, false, panOptionIsValid};
Byte1Option optionMixerSCCPan{CFGKEY_MIXER_SCC_PAN, 50, false, panOptionIsValid};
Byte1Option optionMixerMSXMUSICPan{CFGKEY_MIXER_MSX_MUSIC_PAN, 50, false, panOptionIsValid};
Byte1Option optionMixerMSXAUDIOPan{CFGKEY_MIXER_MSX_AUDIO_PAN, 50, false, panOptionIsValid};
Byte1Option optionMixerMoonSoundPan{CFGKEY_MIXER_MOON_SOUND_PAN, 50, false, panOptionIsValid};
Byte1Option optionMixerYamahaSFGPan{CFGKEY_MIXER_YAMAHA_SFG_PAN, 50, false, panOptionIsValid};
Byte1Option optionMixerPCMPan{CFGKEY_MIXER_PCM_PAN, 50, false, panOptionIsValid};

std::span<const AspectRatioInfo> MsxSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"4:3 (Original)", {4, 3}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

static Byte1Option &optionMixerVolume(MixerAudioType type)
{
	switch(type)
	{
		default: [[fallthrough]];
		case MIXER_CHANNEL_PSG: return optionMixerPSGVolume;
		case MIXER_CHANNEL_SCC: return optionMixerSCCVolume;
		case MIXER_CHANNEL_MSXMUSIC: return optionMixerMSXMUSICVolume;
		case MIXER_CHANNEL_MSXAUDIO: return optionMixerMSXAUDIOVolume;
		case MIXER_CHANNEL_MOONSOUND: return optionMixerMoonSoundVolume;
		case MIXER_CHANNEL_YAMAHA_SFG: return optionMixerYamahaSFGVolume;
		case MIXER_CHANNEL_PCM: return optionMixerPCMVolume;
	}
}

bool mixerEnableOption(MixerAudioType type)
{
	return optionMixerVolume(type).val & MIXER_ENABLE_BIT;
}

void setMixerEnableOption(MixerAudioType type, bool on)
{
	auto &option = optionMixerVolume(type);
	option.val = IG::setOrClearBits(option.val, MIXER_ENABLE_BIT, on);
	mixerEnableChannelType(mixer, type, on);
}

uint8_t mixerVolumeOption(MixerAudioType type)
{
	return optionMixerVolume(type).val & MIXER_VOLUME_MASK;
}

uint8_t setMixerVolumeOption(MixerAudioType type, int volume)
{
	auto &option = optionMixerVolume(type);
	if(volume == -1)
	{
		volume = option.defaultVal & MIXER_VOLUME_MASK;
	}
	option.val = IG::updateBits(option.val, (uint8_t)volume, MIXER_VOLUME_MASK);
	mixerSetChannelTypeVolume(mixer, type, volume);
	return volume;
}

static Byte1Option &optionMixerPan(MixerAudioType type)
{
	switch(type)
	{
		default: [[fallthrough]];
		case MIXER_CHANNEL_PSG: return optionMixerPSGPan;
		case MIXER_CHANNEL_SCC: return optionMixerSCCPan;
		case MIXER_CHANNEL_MSXMUSIC: return optionMixerMSXMUSICPan;
		case MIXER_CHANNEL_MSXAUDIO: return optionMixerMSXAUDIOPan;
		case MIXER_CHANNEL_MOONSOUND: return optionMixerMoonSoundPan;
		case MIXER_CHANNEL_YAMAHA_SFG: return optionMixerYamahaSFGPan;
		case MIXER_CHANNEL_PCM: return optionMixerPCMPan;
	}
}

uint8_t mixerPanOption(MixerAudioType type)
{
	return optionMixerPan(type);
}

uint8_t setMixerPanOption(MixerAudioType type, int pan)
{
	auto &option = optionMixerPan(type);
	if(pan == -1)
	{
		pan = option.reset();
	}
	else
	{
		option = pan;
	}
	mixerSetChannelTypePan(mixer, type, pan);
	return pan;
}

bool MsxSystem::resetSessionOptions(EmuApp &)
{
	optionSessionMachineNameStr.clear();
	return true;
}

bool MsxSystem::readConfig(ConfigType type, MapIO &io, unsigned key, size_t readSize)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_DEFAULT_MACHINE_NAME: return readStringOptionValue(io, readSize, optionDefaultMachineNameStr);
			case CFGKEY_DEFAULT_COLECO_MACHINE_NAME: return readStringOptionValue(io, readSize, optionDefaultColecoMachineNameStr);
			case CFGKEY_SKIP_FDC_ACCESS: return optionSkipFdcAccess.readFromIO(io, readSize);
			case CFGKEY_MACHINE_FILE_PATH: return readStringOptionValue<FS::PathString>(io, readSize, [&](auto &&path){firmwarePath_ = IG_forward(path);});
			case CFGKEY_MIXER_PSG_VOLUME: return optionMixerPSGVolume.readFromIO(io, readSize);
			case CFGKEY_MIXER_SCC_VOLUME: return optionMixerSCCVolume.readFromIO(io, readSize);
			case CFGKEY_MIXER_MSX_MUSIC_VOLUME: return optionMixerMSXMUSICVolume.readFromIO(io, readSize);
			case CFGKEY_MIXER_MSX_AUDIO_VOLUME: return optionMixerMSXAUDIOVolume.readFromIO(io, readSize);
			case CFGKEY_MIXER_MOON_SOUND_VOLUME: return optionMixerMoonSoundVolume.readFromIO(io, readSize);
			case CFGKEY_MIXER_YAMAHA_SFG_VOLUME: return optionMixerYamahaSFGVolume.readFromIO(io, readSize);
			case CFGKEY_MIXER_PCM_VOLUME: return optionMixerPCMVolume.readFromIO(io, readSize);
			case CFGKEY_MIXER_PSG_PAN: return optionMixerPSGPan.readFromIO(io, readSize);
			case CFGKEY_MIXER_SCC_PAN: return optionMixerSCCPan.readFromIO(io, readSize);
			case CFGKEY_MIXER_MSX_MUSIC_PAN: return optionMixerMSXMUSICPan.readFromIO(io, readSize);
			case CFGKEY_MIXER_MSX_AUDIO_PAN: return optionMixerMSXAUDIOPan.readFromIO(io, readSize);
			case CFGKEY_MIXER_MOON_SOUND_PAN: return optionMixerMoonSoundPan.readFromIO(io, readSize);
			case CFGKEY_MIXER_YAMAHA_SFG_PAN: return optionMixerYamahaSFGPan.readFromIO(io, readSize);
			case CFGKEY_MIXER_PCM_PAN: return optionMixerPCMPan.readFromIO(io, readSize);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_SESSION_MACHINE_NAME: return readStringOptionValue(io, readSize, optionSessionMachineNameStr);
		}
	}
	return false;
}

void MsxSystem::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		if(optionDefaultMachineNameStr != optionMachineNameDefault)
		{
			writeStringOptionValue(io, CFGKEY_DEFAULT_MACHINE_NAME, optionDefaultMachineNameStr);
		}
		if(optionDefaultColecoMachineNameStr != optionColecoMachineNameDefault)
		{
			writeStringOptionValue(io, CFGKEY_DEFAULT_COLECO_MACHINE_NAME, optionDefaultColecoMachineNameStr);
		}
		optionSkipFdcAccess.writeWithKeyIfNotDefault(io);
		writeStringOptionValue(io, CFGKEY_MACHINE_FILE_PATH, firmwarePath_);

		optionMixerPSGVolume.writeWithKeyIfNotDefault(io);
		optionMixerSCCVolume.writeWithKeyIfNotDefault(io);
		optionMixerMSXMUSICVolume.writeWithKeyIfNotDefault(io);
		optionMixerMSXAUDIOVolume.writeWithKeyIfNotDefault(io);
		optionMixerMoonSoundVolume.writeWithKeyIfNotDefault(io);
		optionMixerYamahaSFGVolume.writeWithKeyIfNotDefault(io);
		optionMixerPCMVolume.writeWithKeyIfNotDefault(io);

		optionMixerPSGPan.writeWithKeyIfNotDefault(io);
		optionMixerSCCPan.writeWithKeyIfNotDefault(io);
		optionMixerMSXMUSICPan.writeWithKeyIfNotDefault(io);
		optionMixerMSXAUDIOPan.writeWithKeyIfNotDefault(io);
		optionMixerMoonSoundPan.writeWithKeyIfNotDefault(io);
		optionMixerYamahaSFGPan.writeWithKeyIfNotDefault(io);
		optionMixerPCMPan.writeWithKeyIfNotDefault(io);
	}
	else if(type == ConfigType::SESSION)
	{
		writeStringOptionValue(io, CFGKEY_SESSION_MACHINE_NAME, optionSessionMachineNameStr);
	}
}

void MsxSystem::onOptionsLoaded()
{
	mixerEnableChannelType(mixer, MIXER_CHANNEL_PSG, mixerEnableOption(MIXER_CHANNEL_PSG));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_PSG, mixerVolumeOption(MIXER_CHANNEL_PSG));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_PSG, optionMixerPSGPan);

	mixerEnableChannelType(mixer, MIXER_CHANNEL_SCC, mixerEnableOption(MIXER_CHANNEL_SCC));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_SCC, mixerVolumeOption(MIXER_CHANNEL_SCC));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_SCC, optionMixerSCCPan);

	mixerEnableChannelType(mixer, MIXER_CHANNEL_MSXMUSIC, mixerEnableOption(MIXER_CHANNEL_MSXMUSIC));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_MSXMUSIC, mixerVolumeOption(MIXER_CHANNEL_MSXMUSIC));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_MSXMUSIC, optionMixerMSXMUSICPan);

	mixerEnableChannelType(mixer, MIXER_CHANNEL_MSXAUDIO, mixerEnableOption(MIXER_CHANNEL_MSXAUDIO));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_MSXAUDIO, mixerVolumeOption(MIXER_CHANNEL_MSXAUDIO));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_MSXAUDIO, optionMixerMSXAUDIOPan);

	mixerEnableChannelType(mixer, MIXER_CHANNEL_MOONSOUND, mixerEnableOption(MIXER_CHANNEL_MOONSOUND));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_MOONSOUND, mixerVolumeOption(MIXER_CHANNEL_MOONSOUND));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_MOONSOUND, optionMixerMoonSoundPan);

	mixerEnableChannelType(mixer, MIXER_CHANNEL_YAMAHA_SFG, mixerEnableOption(MIXER_CHANNEL_YAMAHA_SFG));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_YAMAHA_SFG, mixerVolumeOption(MIXER_CHANNEL_YAMAHA_SFG));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_YAMAHA_SFG, optionMixerYamahaSFGPan);

	mixerEnableChannelType(mixer, MIXER_CHANNEL_PCM, mixerEnableOption(MIXER_CHANNEL_PCM));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_PCM, mixerVolumeOption(MIXER_CHANNEL_PCM));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_PCM, optionMixerPCMPan);
}

bool MsxSystem::setDefaultMachineName(std::string_view name)
{
	if(name == optionDefaultMachineNameStr)
		return false;
	log.info("set default MSX machine:{}", name);
	optionDefaultMachineNameStr = name;
	return true;
}

bool MsxSystem::setDefaultColecoMachineName(std::string_view name)
{
	if(name == optionDefaultColecoMachineNameStr)
		return false;
	log.info("set default Coleco machine:{}", name);
	optionDefaultColecoMachineNameStr = name;
	return true;
}

static bool archiveHasMachinesDirectory(ApplicationContext ctx, CStringView path)
{
	return bool(FS::findDirectoryInArchive(ctx.openFileUri(path), [&](auto &entry){ return entry.name().ends_with("Machines/"); }));
}

void MsxSystem::setFirmwarePath(CStringView path, FS::file_type type)
{
	auto ctx = appContext();
	log.info("set firmware path:{}", path);
	if((type == FS::file_type::directory && !ctx.fileUriExists(FS::uriString(path, "Machines")))
		|| (FS::hasArchiveExtension(path) && !archiveHasMachinesDirectory(ctx, path)))
	{
		throw std::runtime_error{"Path is missing Machines folder"};
	}
	firmwarePath_ = path;
	firmwareArch = {};
}

FS::PathString MsxSystem::firmwarePath() const
{
	if(firmwarePath_.empty())
	{
		if constexpr(Config::envIsLinux && !Config::MACHINE_IS_PANDORA)
			return appContext().assetPath();
		else
			return appContext().storagePath();
	}
	else
	{
		return firmwarePath_;
	}
}

}
