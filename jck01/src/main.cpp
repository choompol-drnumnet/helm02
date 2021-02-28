#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c3319002"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b2002"

#define HELMET_SWITCH	12
#define ALCOHOL_SENSOR	39

//#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
//#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  BLEDevice::init("Jacket1");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
                                       
  pCharacteristic->setValue("-----------------");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);  
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");

  pinMode(HELMET_SWITCH, INPUT_PULLUP);
} 

int cnt = 0;
char buf[20];

void loop() {
  cnt++;
  int alg = analogRead(ALCOHOL_SENSOR);
  int sw1 = digitalRead(HELMET_SWITCH);
  sprintf(buf, "SFT %d %d %d", cnt, sw1, alg);
  Serial.println(buf);
  pCharacteristic->setValue(buf);
  pCharacteristic->notify();
  // put your main code here, to run repeatedly:
  //delay(1000);
  delay(250);
}

