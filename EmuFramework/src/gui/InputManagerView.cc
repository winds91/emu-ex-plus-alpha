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

#include <emuframework/InputManagerView.hh>
#include <emuframework/ButtonConfigView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuViewController.hh>
#include <emuframework/AppKeyCode.hh>
#include "../EmuOptions.hh"
#include "../InputDeviceData.hh"
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/bluetooth/sys.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/format.hh>
#include <imagine/util/variant.hh>
#include <imagine/util/bit.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"InputManagerView"};
static const char *confirmDeleteDeviceSettingsStr = "Delete device settings from the configuration file? Any key profiles in use are kept";
static const char *confirmDeleteProfileStr = "Delete profile from the configuration file? Devices using it will revert to their default profile";

IdentInputDeviceView::IdentInputDeviceView(ViewAttachParams attach):
	View(attach),
	text{attach.rendererTask, "Push a key on any input device enter its configuration menu", &defaultFace()},
	quads{attach.rendererTask, {.size = 1}} {}

void IdentInputDeviceView::place()
{
	quads.write(0, {.bounds = displayRect().as<int16_t>()});
	text.compile({.maxLineSize = int(viewRect().xSize() * 0.95f)});
}

bool IdentInputDeviceView::inputEvent(const Input::Event &e)
{
	return visit(overloaded
	{
		[&](const Input::MotionEvent &e)
		{
			if(e.released())
			{
				dismiss();
				return true;
			}
			return false;
		},
		[&](const Input::KeyEvent &e)
		{
			if(e.pushed())
			{
				auto del = onIdentInput;
				dismiss();
				del(e);
				return true;
			}
			return false;
		}
	}, e);
}

void IdentInputDeviceView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	using namespace IG::Gfx;
	auto &basicEffect = cmds.basicEffect();
	cmds.set(BlendMode::OFF);
	basicEffect.disableTexture(cmds);
	cmds.setColor({.4, .4, .4});
	cmds.drawQuad(quads, 0);
	basicEffect.enableAlphaTexture(cmds);
	text.draw(cmds, viewRect().center(), C2DO, ColorName::WHITE);
}

InputManagerView::InputManagerView(ViewAttachParams attach,
	InputManager &inputManager_):
	TableView{"Key/Gamepad Input Setup", attach, item},
	inputManager{inputManager_},
	deleteDeviceConfig
	{
		"Delete Saved Device Settings", attach,
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			auto &savedInputDevs = inputManager.savedInputDevs;
			if(!savedInputDevs.size())
			{
				app().postMessage("No saved device settings");
				return;
			}
			auto multiChoiceView = makeViewWithName<TextTableView>(item, savedInputDevs.size());
			for(auto &ePtr : savedInputDevs)
			{
				multiChoiceView->appendItem(InputDeviceData::makeDisplayName(ePtr->name, ePtr->enumId),
					[this, deleteDeviceConfigPtr = ePtr.get()](const Input::Event &e)
					{
						pushAndShowModal(makeView<YesNoAlertView>(confirmDeleteDeviceSettingsStr,
							YesNoAlertView::Delegates
							{
								.onYes = [this, deleteDeviceConfigPtr]
								{
									log.info("deleting device settings for:{},{}",
										deleteDeviceConfigPtr->name, deleteDeviceConfigPtr->enumId);
									auto ctx = appContext();
									for(auto &devPtr : ctx.inputDevices())
									{
										auto &inputDevConf = inputDevData(*devPtr).devConf;
										if(inputDevConf.hasSavedConf(*deleteDeviceConfigPtr))
										{
											log.info("removing from active device");
											inputDevConf.setSavedConf(inputManager, nullptr);
											break;
										}
									}
									std::erase_if(inputManager.savedInputDevs, [&](auto &ptr){ return ptr.get() == deleteDeviceConfigPtr; });
									dismissPrevious();
								}
							}), e);
					});
			}
			pushAndShow(std::move(multiChoiceView), e);
		}
	},
	deleteProfile
	{
		"Delete Saved Key Profile", attach,
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			auto &customKeyConfigs = inputManager.customKeyConfigs;
			if(!customKeyConfigs.size())
			{
				app().postMessage("No saved profiles");
				return;
			}
			auto multiChoiceView = makeViewWithName<TextTableView>(item, customKeyConfigs.size());
			for(auto &ePtr : customKeyConfigs)
			{
				multiChoiceView->appendItem(ePtr->name,
					[this, deleteProfilePtr = ePtr.get()](const Input::Event &e)
					{
						pushAndShowModal(makeView<YesNoAlertView>(confirmDeleteProfileStr,
							YesNoAlertView::Delegates
							{
								.onYes = [this, deleteProfilePtr]
								{
									log.info("deleting profile:{}", deleteProfilePtr->name);
									inputManager.deleteKeyProfile(appContext(), deleteProfilePtr);
									dismissPrevious();
								}
							}), e);
					});
			}
			pushAndShow(std::move(multiChoiceView), e);
		}
	},
	rescanOSDevices
	{
		"Re-scan OS Input Devices", attach,
		[this](const Input::Event &e)
		{
			appContext().enumInputDevices();
			int devices = 0;
			auto ctx = appContext();
			for(auto &e : ctx.inputDevices())
			{
				if(e->map() == Input::Map::SYSTEM)
					devices++;
			}
			app().postMessage(2, false, std::format("{} OS devices present", devices));
		}
	},
	identDevice
	{
		"Auto-detect Device To Setup", attach,
		[this](const Input::Event &e)
		{
			auto identView = makeView<IdentInputDeviceView>();
			identView->onIdentInput =
				[this](const Input::Event &e)
				{
					auto dev = e.device();
					if(dev)
					{
						pushAndShowDeviceView(*dev, e);
					}
				};
			pushAndShowModal(std::move(identView), e);
		}
	},
	generalOptions
	{
		"General Options", attach,
		[this](const Input::Event &e)
		{
			pushAndShow(makeView<InputManagerOptionsView>(&app().viewController().inputView), e);
		}
	},
	deviceListHeading
	{
		"Individual Device Settings", attach,
	}
{
	inputManager.onUpdateDevices = [this]()
	{
		popTo(*this);
		auto selectedCell = selected;
		loadItems();
		highlightCell(selectedCell);
		place();
		show();
	};
	deleteDeviceConfig.setActive(inputManager.savedInputDevs.size());
	deleteProfile.setActive(inputManager.customKeyConfigs.size());
	loadItems();
}

InputManagerView::~InputManagerView()
{
	inputManager.onUpdateDevices = {};
}

void InputManagerView::loadItems()
{
	auto ctx = appContext();
	item.clear();
	item.reserve(16);
	item.emplace_back(&identDevice);
	item.emplace_back(&generalOptions);
	item.emplace_back(&deleteDeviceConfig);
	item.emplace_back(&deleteProfile);
	doIfUsed(rescanOSDevices, [&](auto &mItem)
	{
		if(ctx.androidSDK() >= 12 && ctx.androidSDK() < 16)
			item.emplace_back(&mItem);
	});
	item.emplace_back(&deviceListHeading);
	inputDevName.clear();
	inputDevName.reserve(ctx.inputDevices().size());
	for(auto &devPtr : ctx.inputDevices())
	{
		auto &devItem = inputDevName.emplace_back(inputDevData(*devPtr).displayName, attachParams(),
			[this, &dev = *devPtr](const Input::Event &e)
			{
				pushAndShowDeviceView(dev, e);
			});
		if(devPtr->hasKeys() && !devPtr->isPowerButton())
		{
			item.emplace_back(&devItem);
		}
		else
		{
			log.info("not adding device:{} to list", devPtr->name());
		}
	}
}

void InputManagerView::onShow()
{
	TableView::onShow();
	deleteDeviceConfig.setActive(inputManager.savedInputDevs.size());
	deleteProfile.setActive(inputManager.customKeyConfigs.size());
}

void InputManagerView::pushAndShowDeviceView(const Input::Device &dev, const Input::Event &e)
{
	pushAndShow(makeViewWithName<InputManagerDeviceView>(inputDevData(dev).displayName, *this, dev, inputManager), e);
}

#ifdef CONFIG_BLUETOOTH_SCAN_SECS
static void setBTScanSecs(int secs)
{
	BluetoothAdapter::scanSecs = secs;
	log.info("set bluetooth scan time {}", BluetoothAdapter::scanSecs);
}
#endif

InputManagerOptionsView::InputManagerOptionsView(ViewAttachParams attach, EmuInputView *emuInputView_):
	TableView{"General Input Options", attach, item},
	mogaInputSystem
	{
		"MOGA Controller Support", attach,
		app().mogaManagerIsActive(),
		[this](BoolMenuItem &item)
		{
			if(!app().mogaManagerIsActive() && !appContext().packageIsInstalled("com.bda.pivot.mogapgp"))
			{
				app().postMessage(8, "Install the MOGA Pivot app from Google Play to use your MOGA Pocket. "
					"For MOGA Pro or newer, set switch to mode B and pair in the Android Bluetooth settings app instead.");
				return;
			}
			app().setMogaManagerActive(item.flipBoolValue(*this), true);
		}
	},
	notifyDeviceChange
	{
		"Notify If Devices Change", attach,
		app().notifyOnInputDeviceChange,
		[this](BoolMenuItem &item)
		{
			app().notifyOnInputDeviceChange = item.flipBoolValue(*this);
		}
	},
	bluetoothHeading
	{
		"In-app Bluetooth Options", attach,
	},
	keepBtActive
	{
		"Keep Connections In Background", attach,
		app().keepBluetoothActive,
		[this](BoolMenuItem &item)
		{
			app().keepBluetoothActive = item.flipBoolValue(*this);
		}
	},
	#ifdef CONFIG_BLUETOOTH_SCAN_SECS
	btScanSecsItem
	{
		{
			"2secs", attach,
			[this]()
			{
				setBTScanSecs(2);
			}
		},
		{
			"4secs", attach,
			[this]()
			{
				setBTScanSecs(4);
			}
		},
		{
			"6secs", attach,
			[this]()
			{
				setBTScanSecs(6);
			}
		},
		{
			"8secs", attach,
			[this]()
			{
				setBTScanSecs(8);
			}
		},
		{
			"10secs", attach,
			[this]()
			{
				setBTScanSecs(10);
			}
		}
	},
	btScanSecs
	{
		"Scan Time", attach,
		[]()
		{
			switch(BluetoothAdapter::scanSecs)
			{
				default: return 0;
				case 2: return 0;
				case 4: return 1;
				case 6: return 2;
				case 8: return 3;
				case 10: return 4;
			}
		}(),
		btScanSecsItem
	},
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	btScanCache
	{
		"Cache Scan Results", attach,
		BluetoothAdapter::scanCacheUsage(),
		[this](BoolMenuItem &item)
		{
			BluetoothAdapter::setScanCacheUsage(item.flipBoolValue(*this));
		}
	},
	#endif
	altGamepadConfirm
	{
		"Swap Confirm/Cancel Keys", attach,
		app().swappedConfirmKeys(),
		[this](BoolMenuItem &item)
		{
			app().setSwappedConfirmKeys(item.flipBoolValue(*this));
		}
	},
	emuInputView{emuInputView_}
{
	if constexpr(MOGA_INPUT)
	{
		item.emplace_back(&mogaInputSystem);
	}
	item.emplace_back(&altGamepadConfirm);
	#if 0
	if(Input::hasTrackball())
	{
		item.emplace_back(&relativePointerDecel);
	}
	#endif
	if(appContext().hasInputDeviceHotSwap())
	{
		item.emplace_back(&notifyDeviceChange);
	}
	if(used(bluetoothHeading))
	{
		item.emplace_back(&bluetoothHeading);
		if(used(keepBtActive))
		{
			item.emplace_back(&keepBtActive);
		}
		#ifdef CONFIG_BLUETOOTH_SCAN_SECS
		item.emplace_back(&btScanSecs);
		#endif
		#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
		item.emplace_back(&btScanCache);
		#endif
	}
}

class ProfileSelectMenu : public TextTableView
{
public:
	using ProfileChangeDelegate = DelegateFunc<void (std::string_view profile)>;

	ProfileChangeDelegate onProfileChange{};

	ProfileSelectMenu(ViewAttachParams attach, Input::Device &dev, std::string_view selectedName, const InputManager &mgr):
		TextTableView
		{
			"Key Profile",
			attach,
			mgr.customKeyConfigs.size() + 8 // reserve space for built-in configs
		}
	{
		for(auto &confPtr : mgr.customKeyConfigs)
		{
			auto &conf = *confPtr;
			if(conf.desc().map == dev.map())
			{
				if(selectedName == conf.name)
				{
					activeItem = textItem.size();
				}
				textItem.emplace_back(conf.name, attach,
					[this, &conf](const Input::Event &e)
					{
						auto del = onProfileChange;
						dismiss();
						del(conf.name);
					});
			}
		}
		for(const auto &conf : EmuApp::defaultKeyConfigs())
		{
			if(dev.map() != conf.map)
				continue;
			if(selectedName == conf.name)
				activeItem = textItem.size();
			textItem.emplace_back(conf.name, attach,
				[this, &conf](const Input::Event &e)
				{
					auto del = onProfileChange;
					dismiss();
					del(conf.name);
				});
		}
	}
};

static bool customKeyConfigsContainName(auto &customKeyConfigs, std::string_view name)
{
	return std::ranges::find_if(customKeyConfigs, [&](auto &confPtr){ return confPtr->name == name; }) != customKeyConfigs.end();
}

InputManagerDeviceView::InputManagerDeviceView(UTF16String name, ViewAttachParams attach,
	InputManagerView &rootIMView_, const Input::Device &dev, InputManager &inputManager_):
	TableView{std::move(name), attach, item},
	inputManager{inputManager_},
	rootIMView{rootIMView_},
	playerItems
	{
		[&]
		{
			DynArray<TextMenuItem> items{EmuSystem::maxPlayers + 1uz};
			items[0] = {"Multiple", attach, {.id = InputDeviceConfig::PLAYER_MULTI}};
			const char *numStrings[] {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"};
			for(auto i : iotaCount(EmuSystem::maxPlayers))
			{
				items[i + 1] = {numStrings[i], attach, {.id = i}};
			}
			return items;
		}()
	},
	player
	{
		"Player", attach,
		MenuId{inputDevData(dev).devConf.player()},
		playerItems,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				auto playerVal = item.id;
				bool changingMultiplayer = (playerVal == InputDeviceConfig::PLAYER_MULTI && devConf.player() != InputDeviceConfig::PLAYER_MULTI) ||
					(playerVal != InputDeviceConfig::PLAYER_MULTI && devConf.player() == InputDeviceConfig::PLAYER_MULTI);
				devConf.setPlayer(inputManager, playerVal);
				devConf.save(inputManager);
				if(changingMultiplayer)
				{
					loadItems();
					place();
					show();
				}
				else
					onShow();
			}
		},
	},
	loadProfile
	{
		u"", attach,
		[this](const Input::Event &e)
		{
			auto profileSelectMenu = makeView<ProfileSelectMenu>(devConf.device(),
				devConf.keyConf(inputManager).name, inputManager);
			profileSelectMenu->onProfileChange =
					[this](std::string_view profile)
					{
						log.info("set key profile:{}", profile);
						devConf.setKeyConfName(inputManager, profile);
						onShow();
					};
			pushAndShow(std::move(profileSelectMenu), e);
		}
	},
	renameProfile
	{
		"Rename Profile", attach,
		[this](const Input::Event &e)
		{
			if(!devConf.mutableKeyConf(inputManager))
			{
				app().postMessage(2, "Can't rename a built-in profile");
				return;
			}
			app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input name", devConf.keyConf(inputManager).name,
				[this](EmuApp &app, auto str)
				{
					if(customKeyConfigsContainName(inputManager.customKeyConfigs, str))
					{
						app.postErrorMessage("Another profile is already using this name");
						postDraw();
						return false;
					}
					devConf.mutableKeyConf(inputManager)->name = str;
					onShow();
					postDraw();
					return true;
				});
		}
	},
	newProfile
	{
		"New Profile", attach,
		[this](const Input::Event &e)
		{
			pushAndShowModal(makeView<YesNoAlertView>(
				"Create a new profile? All keys from the current profile will be copied over.",
				YesNoAlertView::Delegates
				{
					.onYes = [this](const Input::Event &e)
					{
						app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input name", "",
							[this](EmuApp &app, auto str)
							{
								if(customKeyConfigsContainName(inputManager.customKeyConfigs, str))
								{
									app.postErrorMessage("Another profile is already using this name");
									return false;
								}
								devConf.setKeyConfCopiedFromExisting(inputManager, str);
								log.info("created new profile:{}", devConf.keyConf(inputManager).name);
								onShow();
								postDraw();
								return true;
							});
					}
				}), e);
		}
	},
	deleteProfile
	{
		"Delete Profile", attach,
		[this](const Input::Event &e)
		{
			if(!devConf.mutableKeyConf(inputManager))
			{
				app().postMessage(2, "Can't delete a built-in profile");
				return;
			}
			pushAndShowModal(makeView<YesNoAlertView>(confirmDeleteProfileStr,
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						auto conf = devConf.mutableKeyConf(inputManager);
						if(!conf)
						{
							bug_unreachable("confirmed deletion of a read-only key config, should never happen");
						}
						log.info("deleting profile:{}", conf->name);
						inputManager.deleteKeyProfile(appContext(), conf);
					}
				}), e);
		}
	},
	iCadeMode
	{
		"iCade Mode", attach,
		inputDevData(dev).devConf.iCadeMode(),
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			if constexpr(Config::envIsIOS)
			{
				confirmICadeMode();
			}
			else
			{
				if(!item.boolValue())
				{
					pushAndShowModal(makeView<YesNoAlertView>(
						"This mode allows input from an iCade-compatible Bluetooth device, don't enable if this isn't an iCade", "Enable", "Cancel",
						YesNoAlertView::Delegates{.onYes = [this]{ confirmICadeMode(); }}), e);
				}
				else
					confirmICadeMode();
			}
		}
	},
	consumeUnboundKeys
	{
		"Handle Unbound Keys", attach,
		inputDevData(dev).devConf.shouldHandleUnboundKeys,
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			devConf.shouldHandleUnboundKeys = item.flipBoolValue(*this);
			devConf.save(inputManager);
		}
	},
	joystickAxisStick1Keys
	{
		"Stick 1 as D-Pad", attach,
		inputDevData(dev).devConf.joystickAxesAsKeys(Input::AxisSetId::stick1),
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			devConf.setJoystickAxesAsKeys(Input::AxisSetId::stick1, item.flipBoolValue(*this));
			devConf.save(inputManager);
		}
	},
	joystickAxisStick2Keys
	{
		"Stick 2 as D-Pad", attach,
		inputDevData(dev).devConf.joystickAxesAsKeys(Input::AxisSetId::stick2),
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			devConf.setJoystickAxesAsKeys(Input::AxisSetId::stick2, item.flipBoolValue(*this));
			devConf.save(inputManager);
		}
	},
	joystickAxisHatKeys
	{
		"POV Hat as D-Pad", attach,
		inputDevData(dev).devConf.joystickAxesAsKeys(Input::AxisSetId::hat),
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			devConf.setJoystickAxesAsKeys(Input::AxisSetId::hat, item.flipBoolValue(*this));
			devConf.save(inputManager);
		}
	},
	joystickAxisTriggerKeys
	{
		"L/R Triggers as L2/R2", attach,
		inputDevData(dev).devConf.joystickAxesAsKeys(Input::AxisSetId::triggers),
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			devConf.setJoystickAxesAsKeys(Input::AxisSetId::triggers, item.flipBoolValue(*this));
			devConf.save(inputManager);
		}
	},
	joystickAxisPedalKeys
	{
		"Brake/Gas as L2/R2", attach,
		inputDevData(dev).devConf.joystickAxesAsKeys(Input::AxisSetId::pedals),
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			devConf.setJoystickAxesAsKeys(Input::AxisSetId::pedals, item.flipBoolValue(*this));
			devConf.save(inputManager);
		}
	},
	categories{"Action Categories", attach},
	options{"Options", attach},
	joystickSetup{"Joystick Axis Setup", attach},
	devConf{inputDevData(dev).devConf}
{
	loadProfile.setName(std::format("Profile: {}", devConf.keyConf(inputManager).name));
	renameProfile.setActive(devConf.mutableKeyConf(inputManager));
	deleteProfile.setActive(devConf.mutableKeyConf(inputManager));
	loadItems();
}

void InputManagerDeviceView::addCategoryItem(const KeyCategory &cat)
{
	auto &catItem = inputCategory.emplace_back(cat.name, attachParams(),
		[this, &cat](const Input::Event &e)
		{
			pushAndShow(makeView<ButtonConfigView>(rootIMView, cat, devConf), e);
		});
	item.emplace_back(&catItem);
}

void InputManagerDeviceView::loadItems()
{
	auto &dev = devConf.device();
	item.clear();
	auto categoryCount = EmuApp::keyCategories().size();
	bool hasJoystick = dev.motionAxes().size();
	auto joystickItemCount = hasJoystick ? 9 : 0;
	item.reserve(categoryCount + joystickItemCount + 12);
	inputCategory.clear();
	inputCategory.reserve(categoryCount);
	if(EmuSystem::maxPlayers > 1)
	{
		item.emplace_back(&player);
	}
	item.emplace_back(&loadProfile);
	item.emplace_back(&categories);
	addCategoryItem(appKeyCategory);
	for(auto &cat : EmuApp::keyCategories())
	{
		if(cat.multiplayerIndex && devConf.player() != InputDeviceConfig::PLAYER_MULTI)
			continue;
		addCategoryItem(cat);
	}
	item.emplace_back(&options);
	item.emplace_back(&newProfile);
	item.emplace_back(&renameProfile);
	item.emplace_back(&deleteProfile);
	if(hasICadeInput && (dev.map() == Input::Map::SYSTEM && dev.hasKeyboard()))
	{
		item.emplace_back(&iCadeMode);
	}
	if constexpr(Config::envIsAndroid)
	{
		item.emplace_back(&consumeUnboundKeys);
	}
	if(hasJoystick)
	{
		item.emplace_back(&joystickSetup);
		if(dev.motionAxis(Input::AxisId::X))
			item.emplace_back(&joystickAxisStick1Keys);
		if(dev.motionAxis(Input::AxisId::Z))
			item.emplace_back(&joystickAxisStick2Keys);
		if(dev.motionAxis(Input::AxisId::HAT0X))
			item.emplace_back(&joystickAxisHatKeys);
		if(dev.motionAxis(Input::AxisId::LTRIGGER))
			item.emplace_back(&joystickAxisTriggerKeys);
		if(dev.motionAxis(Input::AxisId::BRAKE))
			item.emplace_back(&joystickAxisPedalKeys);
	}
}

void InputManagerDeviceView::onShow()
{
	TableView::onShow();
	loadProfile.compile(std::format("Profile: {}", devConf.keyConf(inputManager).name));
	bool keyConfIsMutable = devConf.mutableKeyConf(inputManager);
	renameProfile.setActive(keyConfIsMutable);
	deleteProfile.setActive(keyConfIsMutable);
}

void InputManagerDeviceView::confirmICadeMode()
{
	devConf.setICadeMode(iCadeMode.flipBoolValue(*this));
	devConf.save(inputManager);
	onShow();
	app().defaultVController().setPhysicalControlsPresent(appContext().keyInputIsPresent());
}

}
