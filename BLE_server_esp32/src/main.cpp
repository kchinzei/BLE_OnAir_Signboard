/*
  Original code from https://zenn.dev/luup/articles/iot-yamaguchi-20221204
  titled 「ESP32でBLEデバイスを作る」 by Luup Developers Blog, on Dec 2022
*/
/*
 BLE On Air signboard server.
 ESP32 Arduino BLE server. Compiled by PlatformIO on Visual Studio Code.
 This code is to turn on/off LED light by receiving characteristic signals. 
 */
#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

//---------------------------------------------------------
// Constants
//---------------------------------------------------------
#define SERVICE_UUID        "55725ac1-066c-48b5-8700-2d9fb3603c5e"
#define CHARACTERISTIC_UUID "69ddb59c-d601-4ea4-ba83-44f679a670ba"
#define BLE_DEVICE_NAME     "MyBLEDevice"
#define LED_PIN             16
#define INPUT_PIN           17

//---------------------------------------------------------
// Variables
//---------------------------------------------------------
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
std::string rxValue;
std::string txValue;
bool bleOn = false;
bool inputPinState = false;

//---------------------------------------------------------
// Callbacks
//---------------------------------------------------------
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("onConnect");
  };
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    bleOn = false;
    Serial.println("onDisconnect");
  }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("onWrite");
    std::string rxValue = pCharacteristic->getValue();
    if( rxValue.length() > 0 ){
      bleOn = rxValue[0]!=0;
      Serial.print("Received Value: ");
      for(int i=0; i<rxValue.length(); i++ ){
        Serial.print(rxValue[i],HEX);
      }
      Serial.println();
    }
  }
};

//---------------------------------------------------------
void setup() {
  //
  pinMode(LED_PIN, OUTPUT);
  pinMode(INPUT_PIN, INPUT);
  Serial.begin(115200);
  //
  BLEDevice::init(BLE_DEVICE_NAME);
  // Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Characteristic
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE  |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  //
  pService->start();
  // Advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  Serial.println("startAdvertising");
}

//---------------------------------------------------------
void loop() {
  // disconnecting
  if(!deviceConnected && oldDeviceConnected){
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();
    Serial.println("restartAdvertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if(deviceConnected && !oldDeviceConnected){
    oldDeviceConnected = deviceConnected;
  }
  // LED
  digitalWrite(LED_PIN,bleOn?HIGH:LOW);
  // INPUT
  if(digitalRead(INPUT_PIN) != inputPinState){
    inputPinState = !inputPinState;
    String str = String(inputPinState? 1:0);
    
    Serial.println(str);
    // Notify
    if( deviceConnected ){
      txValue = str.c_str();
      pCharacteristic->setValue(txValue);
      pCharacteristic->notify();
    }
  }
  delay(100);
}
