#pragma once

#include <imagine/input/Input.hh>
#include <imagine/base/Window.hh>

namespace Input
{

extern DeviceChangeDelegate onDeviceChange;

void setAllowKeyRepeats(bool on);
bool allowKeyRepeats();
bool processICadeKey(char c, uint action, const Device &dev, Base::Window &win);

}
