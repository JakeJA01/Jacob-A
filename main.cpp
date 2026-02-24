#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include "SparkFun_Bio_Sensor_Hub_Library.h"
#include "Wire.h"

const int RESET_PIN   = 17;
const int MFIO_PIN    = 18;
const int I2C_ADDRESS = 0x55;

SparkFun_Bio_Sensor_Hub bioHub(RESET_PIN, MFIO_PIN, I2C_ADDRESS);

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
  Serial.begin(115200);
  Serial.println("PPG BLE Logger");

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

  // Sensor setup
  Wire1.begin();
  Wire1.setClock(400000);

  int result = bioHub.begin(Wire1, RESET_PIN, MFIO_PIN);
  Serial.println(result == 0 ? "Sensor initialized!" : "Sensor init failed!");

  bioHub.configSensor();
  bioHub.setSampleRate(100);
  bioHub.setPulseWidth(69);
}

void loop() {
  bioData body = bioHub.readSensor();

  if (deviceConnected) {
    String msg = String(body.irLed) + "," + String(body.redLed);
    pCharacteristic->setValue(msg.c_str());
    pCharacteristic->notify();

    Serial.print("IR: ");
    Serial.print(body.irLed);
    Serial.print(" | Red: ");
    Serial.println(body.redLed);
  }

  delay(10); // ~100 Hz
}