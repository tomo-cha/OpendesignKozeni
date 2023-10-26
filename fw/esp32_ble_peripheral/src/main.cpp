#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// https://www.uuidgenerator.net/
# define    SERVICE_UUID    "199a9fa8-94f8-46bc-8228-ce67c9e807e6" 
# define    CHARACTERISTIC_UUID "208c149e-8266-4686-8918-981e90546c2a"

const int _LED_PIN = 2;
const String BLEName = "XIAO_ESP32BLE";

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
        uint8_t data_buff[2]; // test用2byte'AB'
        data_buff[0] = 'A';
        data_buff[1] = 'B'; 
        Serial.printf("*** NOTIFY: %d, %d ***\n", data_buff[0], data_buff[1]);
        pCharacteristic->setValue(data_buff, 2);
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.printf("*** disconnected. ***\n");
      deviceConnected = false;
    }
};



void setup() {
  Serial.begin(9600);

  // Create the BLE Device
  BLEDevice::init(BLEName.c_str());

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

  pinMode(_LED_PIN, OUTPUT);
}

void loop() {
  if(deviceConnected){
        Serial.print(".");
        uint8_t* pData = pCharacteristic->getData();
        digitalWrite(_LED_PIN, (pData[0]!='A')?LOW:HIGH); // 1byte目が'A'でなくなったら消灯
        pCharacteristic->notify();
  }
  delay(1000);
}

