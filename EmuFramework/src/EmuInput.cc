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

#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuViewController.hh>
#include <emuframework/AppKeyCode.hh>
#include "InputDeviceData.hh"
#include <emuframework/EmuOptions.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"InputManager"};

void InputManager::updateInputDevices(ApplicationContext ctx)
{
	for(auto &devPtr : ctx.inputDevices())
	{
		log.info("input device:{}, id:{}, map:{}", devPtr->name(), devPtr->enumId(), (int)devPtr->map());
		auto &appData = devPtr->makeAppData<InputDeviceData>(*this, *devPtr);
	}
	vController.setPhysicalControlsPresent(ctx.keyInputIsPresent());
	onUpdateDevices.callCopySafe();
}

KeyConfig *InputManager::customKeyConfig(std::string_view name, const Input::Device &dev) const
{
	if(auto it = std::ranges::find_if(customKeyConfigs,
		[&](auto &ptr){ return ptr->name == name && ptr->desc().map == dev.map(); });
		it != customKeyConfigs.end())
	{
		return it->get();
	}
	return nullptr;
}

KeyConfigDesc InputManager::keyConfig(std::string_view name, const Input::Device &dev) const
{
	auto conf = customKeyConfig(name, dev);
	if(conf)
		return conf->desc();
	for(auto &conf : EmuApp::defaultKeyConfigs())
	{
		if(conf.name == name)
			return conf;
	}
	return {};
}

static void removeKeyConfFromAllDevices(InputManager &inputManager, std::string_view conf, ApplicationContext ctx)
{
	log.info("removing saved key config {} from all devices", conf);
	for(auto &ePtr : inputManager.savedInputDevs)
	{
		auto &e = *ePtr;
		if(e.keyConfName == conf)
		{
			log.info("used by saved device config {},{}", e.name, e.enumId);
			e.keyConfName.clear();
		}
		auto devIt = std::ranges::find_if(ctx.inputDevices(), [&](auto &devPtr){ return e.matchesDevice(*devPtr); });
		if(devIt != ctx.inputDevices().end())
		{
			inputDevData(*devIt->get()).buildKeyMap(inputManager, *devIt->get());
		}
	}
}

void InputManager::deleteKeyProfile(ApplicationContext ctx, KeyConfig *profile)
{
	removeKeyConfFromAllDevices(*this, profile->name, ctx);
	std::erase_if(customKeyConfigs, [&](auto &confPtr){ return confPtr.get() == profile; });
}

void InputManager::writeCustomKeyConfigs(FileIO &io) const
{
	for(const auto &c : customKeyConfigs)
	{
		c->writeConfig(io);
	}
}

void InputManager::writeSavedInputDevices(ApplicationContext ctx, FileIO &io) const
{
	if(!savedInputDevs.size())
		return;
	// compute total size
	ssize_t bytes = 2; // config key size
	bytes += 1; // number of configs
	for(auto &ePtr : savedInputDevs)
	{
		auto &e = *ePtr;
		bytes += 1; // device id
		bytes += 1; // enabled
		bytes += 1; // player
		bytes += 1; // mapJoystickAxis1ToDpad
		if(hasICadeInput)
			bytes += 1; // iCade mode
		bytes += 1; // name string length
		uint8_t nameSize = e.name.size();
		bytes += nameSize; // name string
		bytes += 1; // key config map
		bytes += 1; // name of key config string length
		uint8_t keyConfNameSize = e.keyConfName.size();
		bytes += keyConfNameSize; // name of key config string
	}
	if(bytes > 0xFFFF)
	{
		bug_unreachable("excessive input device config size, should not happen");
	}
	// write to config file
	log.info("saving {} input device configs, {} bytes", savedInputDevs.size(), bytes);
	io.put(uint16_t(bytes));
	io.put(uint16_t(CFGKEY_INPUT_DEVICE_CONFIGS));
	io.put(uint8_t(savedInputDevs.size()));
	for(auto &ePtr : savedInputDevs)
	{
		auto &e = *ePtr;
		log.info("writing config {}, id {}", e.name, e.enumId);
		uint8_t enumIdWithFlags = e.enumId;
		if(e.handleUnboundEvents)
			enumIdWithFlags |= e.HANDLE_UNBOUND_EVENTS_FLAG;
		io.put(uint8_t(enumIdWithFlags));
		io.put(uint8_t(e.enabled));
		io.put(uint8_t(e.player));
		io.put(std::bit_cast<uint8_t>(e.joystickAxisAsDpadFlags));
		if(hasICadeInput)
			io.put(uint8_t(e.iCadeMode));
		uint8_t nameLen = e.name.size();
		assert(nameLen);
		io.put(nameLen);
		io.write(e.name.data(), nameLen);
		auto devPtr = ctx.inputDevice(e.name, e.enumId);
		uint8_t keyConfMap = devPtr ? (uint8_t)devPtr->map() : 0;
		io.put(keyConfMap);
		uint8_t keyConfNameLen = e.keyConfName.size();
		io.put(keyConfNameLen);
		if(keyConfNameLen)
		{
			log.info("has key conf {}, map {}", e.keyConfName, keyConfMap);
			io.write(e.keyConfName.data(), keyConfNameLen);
		}
	}
}

bool InputManager::readCustomKeyConfig(MapIO &io)
{
	auto conf = KeyConfig::readConfig(io);
	if(conf)
	{
		customKeyConfigs.emplace_back(std::make_unique<KeyConfig>(conf));
	}
	return true;
}

bool InputManager::readSavedInputDevices(MapIO &io)
{
	auto confs = io.get<uint8_t>();
	for(auto confIdx : iotaCount(confs))
	{
		InputDeviceSavedConfig devConf;
		auto enumIdWithFlags = io.get<uint8_t>();
		devConf.handleUnboundEvents = enumIdWithFlags & devConf.HANDLE_UNBOUND_EVENTS_FLAG;
		devConf.enumId = enumIdWithFlags & devConf.ENUM_ID_MASK;
		devConf.enabled = io.get<uint8_t>();
		devConf.player = io.get<uint8_t>();
		if(devConf.player != InputDeviceConfig::PLAYER_MULTI && devConf.player > EmuSystem::maxPlayers)
		{
			log.warn("player {} out of range", devConf.player);
			devConf.player = 0;
		}
		devConf.joystickAxisAsDpadFlags = std::bit_cast<AxisAsDpadFlags>(io.get<uint8_t>());
		if(hasICadeInput)
		{
			devConf.iCadeMode = io.get<uint8_t>();
		}
		auto nameLen = io.get<uint8_t>();
		if(!nameLen)
		{
			log.error("unexpected 0 length device name");
			return false;
		}
		io.readSized(devConf.name, nameLen);
		auto keyConfMap = Input::validateMap(io.get<uint8_t>());
		auto keyConfNameLen = io.get<uint8_t>();
		if(keyConfNameLen)
		{
			char keyConfName[keyConfNameLen + 1];
			if(io.read(keyConfName, keyConfNameLen) != keyConfNameLen)
				return false;
			keyConfName[keyConfNameLen] = '\0';
			devConf.keyConfName = keyConfName;
		}
		if(!containsIf(savedInputDevs, [&](const auto &confPtr){ return *confPtr == devConf;}))
		{
			log.info("read input device config:{}, id:{}", devConf.name, devConf.enumId);
			savedInputDevs.emplace_back(std::make_unique<InputDeviceSavedConfig>(devConf));
		}
		else
		{
			log.warn("ignoring duplicate input device config:{}, id:{}", devConf.name, devConf.enumId);
		}
	}
	return true;
}

KeyConfigDesc InputManager::defaultConfig(const Input::Device &dev) const
{
	KeyConfigDesc firstConfig{}, firstSubtypeConfig{};
	for(const auto &conf : EmuApp::defaultKeyConfigs())
	{
		if(dev.map() == conf.map && !firstConfig)
			firstConfig = conf;
		if(dev.subtype() == conf.devSubtype && !firstSubtypeConfig)
			firstSubtypeConfig = conf;
	}
	return firstSubtypeConfig ?: firstConfig;
}

KeyInfo InputManager::transpose(KeyInfo c, int index) const
{
	if(index != InputDeviceConfig::PLAYER_MULTI)
		c.flags.deviceId = index;
	return c;
}

std::string InputManager::toString(KeyInfo k) const
{
	std::string s{toString(k.codes[0], k.flags)};
	for(const auto &c : k.codes | std::ranges::views::drop(1))
	{
		s += " + ";
		s += toString(c, k.flags);
	}
	if(k.flags.turbo && k.flags.toggle)
		s += " (Turbo Toggle)";
	else if(k.flags.turbo)
		s += " (Turbo)";
	else if(k.flags.toggle)
		s += " (Toggle)";
	return s;
}

std::string InputManager::toString(KeyCode c, KeyFlags flags) const
{
	if(flags.appCode)
		return std::string{EmuEx::toString(AppKeyCode(c))};
	return std::string{EmuApp::systemKeyCodeToString(c)};
}

void InputManager::updateKeyboardMapping()
{
	vController.updateKeyboardMapping();
}

void InputManager::toggleKeyboard()
{
	vController.resetHighlightedKeys();
	vController.toggleKeyboard();
}

void EmuApp::setDisabledInputKeys(std::span<const KeyCode> keys)
{
	auto &vController = inputManager.vController;
	vController.setDisabledInputKeys(keys);
	if(!vController.hasWindow())
		return;
	vController.place();
	system().clearInputBuffers(viewController().inputView);
}

void EmuApp::unsetDisabledInputKeys()
{
	setDisabledInputKeys({});
}

bool InputDeviceSavedConfig::matchesDevice(const Input::Device &dev) const
{
	//log.debug("checking against device {},{}", name, devId);
	return dev.enumId() == enumId && dev.name() == name;
}

const KeyCategory *InputManager::categoryOfKeyCode(KeyInfo key) const
{
	if(key.isAppKey())
		return &appKeyCategory;
	for(const auto &cat : EmuApp::keyCategories())
	{
		if(containsIf(cat.keys, [&](auto &k) { return k.codes == key.codes; }))
			return &cat;
	}
	return {};
}

KeyInfo InputManager::validateSystemKey(KeyInfo key, bool isUIKey) const
{
	if(!categoryOfKeyCode(key))
	{
		log.warn("resetting invalid system key:{}", key.codes[0]);
		if(isUIKey)
			return appKeyCategory.keys[0];
		else
			return EmuApp::keyCategories()[0].keys[0];
	}
	return key;
}

std::string_view toString(AppKeyCode code)
{
	switch(code)
	{
		case AppKeyCode::openContent: return "Open Content";
		case AppKeyCode::closeContent: return "Close Content";
		case AppKeyCode::openSystemActions: return "Open System Actions";
		case AppKeyCode::saveState: return "Save State";
		case AppKeyCode::loadState: return "Load State";
		case AppKeyCode::decStateSlot: return "Decrement State Slot";
		case AppKeyCode::incStateSlot: return "Increment State Slot";
		case AppKeyCode::fastForward: return "Fast-forward";
		case AppKeyCode::takeScreenshot: return "Take Screenshot";
		case AppKeyCode::openMenu: return "Open Menu";
		case AppKeyCode::toggleFastForward: return "Toggle Fast-forward";
		case AppKeyCode::turboModifier: return "Turbo Modifier";
		case AppKeyCode::exitApp: return "Exit App";
		case AppKeyCode::slowMotion: return "Slow-motion";
		case AppKeyCode::toggleSlowMotion: return "Toggle Slow-motion";
		case AppKeyCode::rewind: return "Rewind";
		case AppKeyCode::softReset: return "Soft Reset";
		case AppKeyCode::hardReset: return "Hard Reset";
		case AppKeyCode::resetMenu: return "Open Reset Menu";
	};
	return "";
}

}
