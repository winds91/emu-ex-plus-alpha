#pragma once

#include <imagine/input/Input.hh>
#include <imagine/bluetooth/sys.hh>

namespace Bluetooth
{

bool scanForDevices(BluetoothAdapter &bta, BluetoothAdapter::OnStatusDelegate onScanStatus);
bool listenForDevices(BluetoothAdapter &bta, const BluetoothAdapter::OnStatusDelegate &onScanStatus);
void closeDevices(BluetoothAdapter *bta);
void closeBT(BluetoothAdapter *&bta);
uint32_t devsConnected();
uint32_t pendingDevs();
void connectPendingDevs(BluetoothAdapter *bta);

}
