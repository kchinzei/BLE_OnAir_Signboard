#
#  The MIT License (MIT)
#  Copyright (c) Kiyo Chinzei (kchinzei@gmail.com)
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#  THE SOFTWARE.
"""
BLE On Air signboard client.
This script is to remotely turn on/off LED light. The light is connected to an ESP32
board acted as the BLE server.
"""
import asyncio
from logging import getLogger
from bleak import BleakScanner, BleakClient
from bleak.backends.characteristic import BleakGATTCharacteristic
from bleak.exc import BleakError

DEVICE_NAME = 'MyBLEDevice'
SERVICE_UUID = "55725ac1-066c-48b5-8700-2d9fb3603c5e"
CHARACTERISTIC_UUID = '69ddb59c-d601-4ea4-ba83-44f679a670ba'

logger = getLogger(__name__)

async def print_client_status(client):
    print(f'connected to {client.address=}')
    print(f'{client.mtu_size=}')
    for service in client.services:
        print(f'{service=}')
        for char in service.characteristics:
            if "read" in char.properties:
                try:
                    value = await client.read_gatt_char(char.uuid)
                    print(f'  [Characteristic] {char=} ({char.properties=}), Value: {value=}')
                except Exception as e:
                    print(f'  [Characteristic] Error: {char.properties=} {e=}')
            else:
                print(f'  [Characteristic] {char=} ({char.properties=})')


def notification_handler(characteristic: BleakGATTCharacteristic, data: bytearray):
    """Simple notification handler which prints the data received."""
    logger.info('%s: %s', characteristic.description, data.decode())
    print(f'notification received: {data.decode()}')


async def main():
    while True:
        device = None
        while device is None:
            device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10)
            await asyncio.sleep(10)
    
        print(f'Found(UUID)!\t{device.name=}')
        print(f'\t{device.address=}')
        print(f'\t{device.details=}')

        try:
            dummy: bool = True
            async with BleakClient(device, services=[SERVICE_UUID], timeout=10) as client:
                await asyncio.wait_for(client.start_notify(CHARACTERISTIC_UUID, notification_handler), timeout=10)
                # print_client_status(client)
                while client.is_connected:
                    b = b'\x01' if dummy else b'\x00'
                    dummy = not dummy

                    await asyncio.wait_for(client.write_gatt_char(CHARACTERISTIC_UUID, b, response=True), timeout=10)
                    await asyncio.sleep(2.0)
        except (asyncio.TimeoutError, BleakError) as e:
            logger.info(f'Exception {e=}')
        except Exception as e:
            logger.info(f'Other exception {e=}')

        await client.disconnect()
        print(f'disconnected. wait for connecting again')
        device = None
        # Go to top


asyncio.run(main())