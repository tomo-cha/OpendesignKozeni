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
int servoPin = 21;

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

// 右にスイープして戻す
void moveServoRight()
{
  for (int i = 40; i < 120; i += 2)
  {
    myservo.write(i); // サーボを0度の位置に動かす
    delay(30);        // 0.5秒待つ
  }

  for (int i = 120; i > 40; i -= 2)
  {
    myservo.write(i); // サーボを0度の位置に動かす
    delay(50);        // 0.5秒待つ
  }
}

// 現在時刻を保存するためのグローバル変数
int receivedHour = 0, receivedMinute = 0, receivedSecond = 0;
bool updateReceivedTime = false;

// 予定された時刻を保存するためのグローバル変数
int scheduleHour = 0, scheduleMinute = 0, scheduleSecond = 0;
bool updateReceivedSchedule = false;

// 現在時刻を受け付けるためのコールバック
// および予約時間を受け付けるための変数を追加
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0)
    {
      // コマンドの場合の処理
      if (value.substr(0, 4) == "CMD:")
      {
        char cmd = value[4]; // 5番目の文字を取得
        if (cmd == 'A')
        {
          // ここでグローバル変数に書き込めばそのモードに変更可能
          digitalWrite(_LED_PIN, HIGH);

          Serial.println("Received Command: A");
        }
        else if (cmd == 'B')
        {
          moveServoRight();
          globalValue += 1;
          digitalWrite(_LED_PIN, LOW);
          Serial.println("Received Command: B");
        }
        // 他のコマンドも同様に追加
      }
      // 時刻の場合の処理
      else if (value.substr(0, 5) == "TIME:")
      {
        sscanf(value.substr(5).c_str(), "%02d:%02d:%02d", &receivedHour, &receivedMinute, &receivedSecond);
        updateReceivedTime = true;
        Serial.println("Received Time: " + String(receivedHour) + ":" + String(receivedMinute) + ":" + String(receivedSecond));
      }
      // 他のデータ形式に対する処理も追加可能
      if (value.substr(0, 8) == "MEETING:")
      {
        sscanf(value.substr(8).c_str(), "%02d:%02d:%02d", &scheduleHour, &scheduleMinute, &scheduleSecond);
        updateReceivedSchedule = true;
        Serial.println("Received Meeting Time: " + String(scheduleHour) + ":" + String(scheduleMinute) + ":" + String(scheduleSecond));
      }
    }
  }
};

// 　取得した時間をプリントするような処理
void timePrint()
{
  if (updateReceivedTime)
  {
    delay(1000); // 1秒待機

    // 秒を更新
    receivedSecond++;
    if (receivedSecond >= 60)
    {
      receivedSecond = 0;
      receivedMinute++;
      if (receivedMinute >= 60)
      {
        receivedMinute = 0;
        receivedHour++;
        if (receivedHour >= 24)
        {
          receivedHour = 0;
        }
      }
    }

    // 更新された時刻をシリアルに出力
    Serial.printf("%02d:%02d:%02d\n", receivedHour, receivedMinute, receivedSecond);
  }
}

// Lチカ
void blinkLed() 
{
  for(int i = 0; i < 10; i++){
    digitalWrite(_LED_PIN,HIGH);
    delay(150);
    digitalWrite(_LED_PIN,LOW);
    delay(150);
  }
}

// 　予定時刻と現在の時刻が一致したらLちかさせる
void checkTime()
{
  if (updateReceivedTime && updateReceivedSchedule)
  {
    if (receivedHour == scheduleHour && receivedMinute == scheduleMinute && receivedSecond == scheduleSecond)
    {
      // LEDを点灯させる関数
      blinkLed();
    }
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

  // ここでコールバックをセットします
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

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
  // BLEと一度接続された後にその時刻を継続的に吐き出す処理
  timePrint();
  checkTime();
}
