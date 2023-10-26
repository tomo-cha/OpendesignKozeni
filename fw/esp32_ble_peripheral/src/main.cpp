#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <ESP32Servo.h>

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// グローバル変数の追加
int globalValue = 0;

// servo pin
int servoPin = 32;

// https://www.uuidgenerator.net/
#define SERVICE_UUID "199a9fa8-94f8-46bc-8228-ce67c9e807e6"
#define CHARACTERISTIC_UUID "208c149e-8266-4686-8918-981e90546c2a"

const int _LED_PIN = 2;
const String BLEName = "XIAO_ESP32BLE";

Servo myservo;   

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    uint8_t data_buff[2]; // test用2byte'AB'
    data_buff[0] = 'A';
    data_buff[1] = 'B';
    Serial.printf("*** NOTIFY: %d, %d ***\n", data_buff[0], data_buff[1]);
    pCharacteristic->setValue(data_buff, 2);
  };

  void onDisconnect(BLEServer *pServer)
  {
    Serial.printf("*** disconnected. ***\n");
    deviceConnected = false;
  }
};

//右にスイープして戻す
void moveServoRight()
{
  for (int i = 40; i < 120; i+=2)
  {
    myservo.write(i);  // サーボを0度の位置に動かす
    delay(30);        // 0.5秒待つ
  }

  for (int i = 120; i > 40; i-=2)
  {
    myservo.write(i);  // サーボを0度の位置に動かす
    delay(50);        // 0.5秒待つ
  }
}

void setup()
{
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
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

  pinMode(_LED_PIN, OUTPUT);

  myservo.attach(servoPin);
  myservo.write(40); 

}

void loop()
{
  if (deviceConnected)
  {
    Serial.print(".");
    uint8_t *pData = pCharacteristic->getData();
    if (pData[0] == 'A')
    {
      digitalWrite(_LED_PIN, HIGH);
    }
    else if (pData[0] == 'B')
    {
      // servoを右に動かす
      moveServoRight();
      globalValue += 1;
      digitalWrite(_LED_PIN, LOW);
    }
    else if (pData[0] == 'C')
    {
      // servoを左に動かす
      globalValue -= 1;
      digitalWrite(_LED_PIN, LOW);
    }
    else
    {
      digitalWrite(_LED_PIN, LOW);
    }
    pCharacteristic->notify();
  }
  delay(100);
}
