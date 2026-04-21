#include <Arduino.h>

#include <Wire.h>
#include "MAX30105.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

MAX30105 particleSensor;

#define SERVICE_UUID        "12345678-1234-1234-1234-123456789012"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-210987654321"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("Device connected!");
  }
  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("Device disconnected!");
    delay(500);
    BLEDevice::startAdvertising();
  }
};

void setup() {
    // Initialize serial at the same rate set in platformio.ini
    Serial.begin(9600); 

    // Wait for Serial to be ready
  while (!Serial) { delay(10); }
  
  // A tiny extra delay helps the PC side "catch up"
  delay(5000); 
    
    Serial.println("ESP32 Serial Test Started");

     // Initialize sensor
  if (particleSensor.begin() == false)
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }

    particleSensor.setup(); //Configure sensor. Use 6.4mA for LED drive


   // BLE setup
  BLEDevice::init("QT_Py_ESP32S3");
  BLEDevice::setMTU(512);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setValue("0,0");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  Serial.println("BLE advertising...");


}

void loop() {

  if (deviceConnected) {
    String msg = String(particleSensor.getGreen()) + "," + String(particleSensor.getRed()) + "," + String(particleSensor.getIR());
    pCharacteristic->setValue(msg.c_str());
    pCharacteristic->notify();

    Serial.print(" | Green: ");
    Serial.println(particleSensor.getGreen());
    Serial.print("IR: ");
    Serial.print(particleSensor.getIR());
    Serial.print(" | Red: ");
    Serial.println(particleSensor.getRed());
  }

  delay(10); // ~100 Hz
  
}
