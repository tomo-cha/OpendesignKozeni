#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ESP32Servo.h>

TaskHandle_t thp[1]; // マルチスレッドのタスクハンドル格納用

// GSRセンサ
const int GSR_PIN = 33; // 接続するanalog pin
int gsr_sensorValue = 0;
int gsr_average = 0;
int old_gsr_average;
unsigned long previousMillis = 0;
const long interval = 250;

// 心拍センサ
const int HEART_PIN = 32; // 接続するanalog pin
int heart_sensorValue;
float heart_filteredValue;
int bpm;
const int bpm_threshold = 90;
const int numReadings = 10;
int readings[numReadings];     // the readings from the analog input
int readIndex = 0;             // the index of the current reading
int total = 0;                 // the running total
volatile int bpm_filtered = 0; // the average
// 心拍センサーデータの処理のための変数
float lpfHeart = 0;                // ローパスフィルタの出力
float hpfHeart = 0;                // ハイパスフィルタの出力
float previousBpfHeart = 0;        // 前のバンドパスフィルタの出力
float bpfHeart = 0;                // バンドパスフィルタの出力
const float filterAlpha = 0.8;     // フィルタのアルファ（LPF用）
const float filterAlphaBand = 0.2; // フィルタのアルファ（BPF用）
// BPMを計算するための変数
volatile unsigned long lastBeatTime = 0; // 最後のビートが検出された時間
const int heart_threshold = 300;         // バンドパスフィルタ後の値の閾値

// 緊張の判定
int nervous_combo_count = 0;
int last_nervous_time = 0;

// サーボ
Servo servo_horizon;           // 左右動きのサーボ
const int SERVO_HORIZON = 21;  // サーボを接続するピン。適宜変更してください
Servo servo_vertical;          // 上下動きのサーボ
const int SERVO_VERTICAL = 19; // サーボを接続するピン。適宜変更してください

// ボタン
const int buttonPin = 14; // 任意のボタンピンを選択。変更が必要な場合はこちらを変更してください
int buttonFlag = 0;
int buttonCount = 1;

void getGsrData()
{
  /*
    GSRセンサ
    平均値を出力する
    3秒ごとに比較
  */
  long sum = 0;
  for (int i = 0; i < 10; i++) // Average the 10 measurements to remove the glitch
  {
    gsr_sensorValue = analogRead(GSR_PIN);
    sum += gsr_sensorValue;
  }
  gsr_average = sum / 10;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    old_gsr_average = gsr_average;
  }
}
/*
  心拍センサ
  脈の収縮時(アナログ値が高いとき)を閾値で検出する(クロススレッショルドピーク検出法)
  平均値を出力する
  BPMを計算する
*/
// センサデータを読み取り、バンドパスフィルタを適用する関数
float applyBandPassFilter(int heart_sensorValue)
{
  // DC成分をカットするためのローパスフィルタ
  lpfHeart = filterAlpha * lpfHeart + (1.0 - filterAlpha) * heart_sensorValue;

  // 高周波ノイズをカットするためのハイパスフィルタ
  hpfHeart = heart_sensorValue - lpfHeart;

  // バンドパスフィルタの適用
  bpfHeart = filterAlphaBand * previousBpfHeart + (1.0 - filterAlphaBand) * hpfHeart;

  previousBpfHeart = bpfHeart; // 次の計算のために現在のフィルタ出力を保存

  return bpfHeart;
}
// バンドパスフィルタ後の値からBPMを計算する関数
void calculateBPM()
{
  // バンドパスフィルタを適用した値を取得
  heart_sensorValue = analogRead(HEART_PIN);
  heart_filteredValue = applyBandPassFilter(heart_sensorValue);

  // BPMの計算と通知
  if (heart_filteredValue > heart_threshold && (millis() - lastBeatTime) > 250)
  {                                          // ビート検出条件
    bpm = 60000 / (millis() - lastBeatTime); // BPM計算
    // bpmの移動平均を計算
    total = total - readings[readIndex];
    readings[readIndex] = bpm;
    total = total + readings[readIndex];
    readIndex = readIndex + 1;
    if (readIndex >= numReadings)
    {
      readIndex = 0;
    }
    bpm_filtered = total / numReadings;
    lastBeatTime = millis(); // 最後のビート時間を更新

    notifyBPM(bpm_filtered); // BPM値をBLE経由で通知
  }
  Serial.print("bpm:");
  Serial.print(bpm);
  Serial.print(",");
  Serial.print("bpm_filtered:");
  Serial.print(bpm_filtered);
  Serial.print(",");
}
void moveServoHorizontal(int delayTime)
{
  // 左右に動く
  for (int i = 90; i < 180; i += 2)
  {
    servo_horizon.write(i);
    delay(delayTime);
  }
  for (int i = 180; i > 0; i -= 2)
  {
    servo_horizon.write(i);
    delay(delayTime);
  }
  for (int i = 0; i < 90; i += 2)
  {
    servo_horizon.write(i);
    delay(delayTime);
  }
}
void moveServo1()
{ // 低い緊張緩和 ちょっと上に上げてちょっと左右に動く
  // 上げる
  for (int i = 40; i > 25; i -= 2)
  {
    servo_vertical.write(i); // サーボを0度の位置に動かす
    delay(100);              // 0.5秒待つ
  }
  delay(2000);
  // トントン
  for (int i = 25; i < 50; i += 2)
  {
    servo_vertical.write(i);
    delay(50);
  }
  for (int i = 50; i > 25; i -= 2)
  {
    servo_vertical.write(i); // サーボを0度の位置に動かす
    delay(50);               // 0.5秒待つ
  }
  delay(1000);
  for (int i = 25; i < 50; i += 2)
  {
    servo_vertical.write(i);
    delay(50);
  }
  for (int i = 50; i > 25; i -= 2)
  {
    servo_vertical.write(i); // サーボを0度の位置に動かす
    delay(50);               // 0.5秒待つ
  }
  delay(1000);

  moveServoHorizontal(30);
  delay(1000);
  // 下げる
  for (int i = 25; i < 40; i += 2)
  {
    servo_vertical.write(i);
    delay(100);
  }
}
void moveServo2()
{ // 中の緊張緩和 ちょっと下に下げて左右に動く
  // 下げる
  for (int i = 40; i < 50; i += 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
  moveServoHorizontal(30);
  // 上げる
  for (int i = 50; i > 40; i -= 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
}
void moveServo3()
{ // 高い緊張緩和 大きく下に下げて左右に動く
  // 上げる
  for (int i = 40; i < 60; i += 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
  moveServoHorizontal(30);
  // 下げる
  for (int i = 60; i > 40; i -= 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
}
/*
  月曜日 実験用動き
  1.イージングをかけない
  2.左右の動きにイージングをかける
  だんだん遅くなる2次関数的イージング
  最大速度は5cm/sになるようにする https://journals.lww.com/neuroreport/abstract/1999/07130/psychophysical_assessment_of_the_affective.17.aspx
*/
void moveServo1_2()
{
  // 低い緊張緩和 ちょっと上に上げてちょっと左右に動く
  // 上げる
  for (int i = 40; i > 25; i -= 2)
  {
    servo_vertical.write(i); // サーボを0度の位置に動かす
    delay(30);               // 0.5秒待つ
  }
  moveServoHorizontal(10);
  // 下げる
  for (int i = 25; i < 40; i += 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
}
void moveServo2_2()
{
  // 中の緊張緩和 ちょっと下に下げて左右に動く
  // 下げる
  for (int i = 40; i < 55; i += 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
  moveServoHorizontal(10);
  // 上げる
  for (int i = 55; i > 40; i -= 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
}
void moveServo3_2()
{
  // 高い緊張緩和 大きく下に下げて左右に動く
  // 上げる
  for (int i = 40; i < 65; i += 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
  moveServoHorizontal(10);
  // 下げる
  for (int i = 65; i > 40; i -= 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
}
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// グローバル変数の追加
int globalValue = 0;
// https://www.uuidgenerator.net/
#define SERVICE_UUID "199a9fa8-94f8-46bc-8228-ce67c9e807e6"
#define CHARACTERISTIC_UUID "208c149e-8266-4686-8918-981e90546c2a"
const String BLEName = "XIAO_ESP32BLE";
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
    servo_horizon.write(i); // サーボを0度の位置に動かす
    delay(30);              // 0.5秒待つ
  }

  for (int i = 120; i > 40; i -= 2)
  {
    servo_horizon.write(i); // サーボを0度の位置に動かす
    delay(50);              // 0.5秒待つ
  }
}
// 現在時刻を保存するためのグローバル変数
int receivedHour = 0, receivedMinute = 0;
volatile int receivedSecond = 0;
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

          Serial.println("Received Command: A");
        }
        else if (cmd == 'B')
        {
          moveServoRight();
          globalValue += 1;
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
      // 遠隔のデータ（スライダ）が送信されていたら
      if (value.rfind("SLIDER:", 0) == 0)
      {                                               // valueが"SLIDER:"で始まるかチェック
        int sliderValue = std::stoi(value.substr(7)); // スライダーの値を取得（プレフィックスの後）
        // moveServoToPosition(sliderValue); // ここでサーボモータを動かすなどの処理を行う
        servo_horizon.write(sliderValue);
        Serial.println("Received Slider Value: " + String(sliderValue));
      }
    }
  }
};
hw_timer_t *timer = NULL;
void IRAM_ATTR onTimer()
{
  receivedSecond++;
}
// 　取得した時間をプリントするような処理
void timePrint()
{
  if (updateReceivedTime)
  {
    //    delay(1000); // 1秒待機

    // 秒を更新
    //    receivedSecond++;
    timerAlarmEnable(timer); // 1秒ごとに実行されるタイマー割り込みを開始
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
const int minutesBeforeAction = 5; // ここで5, 15, 30など何分前に起動させるのか設定

void checkTime()
{
  if (updateReceivedTime && updateReceivedSchedule)
  {
    // 現在時刻と予定時刻の差分を計算（分単位）
    int currentMinutes = receivedHour * 60 + receivedMinute;
    int scheduleMinutes = scheduleHour * 60 + scheduleMinute;
    int diffMinutes = scheduleMinutes - currentMinutes;

    // 予定時刻の何分前にアクションを実行するかをチェック
    if (diffMinutes == minutesBeforeAction)
    {
      // アクションを実行
      Serial.println("アクションを実行");
      moveServo1();
    }
    else if (diffMinutes < 0)
    {
      // 予定時刻を過ぎている場合は、フラグをリセット
      updateReceivedTime = false;
      updateReceivedSchedule = false;
    }
  }
} // BPM値をBLEキャラクタリスティックにセットして通知する関数
void notifyBPM(int bpmValue)
{
  if (deviceConnected)
  {
    // BPM値を文字列に変換
    char bpmStr[10];
    sprintf(bpmStr, "%d", bpmValue);

    // BPM値をBLEキャラクタリスティックにセット
    pCharacteristic->setValue((uint8_t *)bpmStr, strlen(bpmStr));

    // 通知を送信
    pCharacteristic->notify();
  }
}

void setup()
{
  Serial.begin(9600); // シリアル通信
  xTaskCreatePinnedToCore(Core0a, "Core0a", 4096, NULL, 3, &thp[0], 0);

  // GSRセンサ

  // 心拍センサ
  for (int thisReading = 0; thisReading < numReadings; thisReading++)
  {
    readings[thisReading] = 0;
  }

  // サーボ
  servo_horizon.attach(SERVO_HORIZON); // サーボピンを指定
  servo_horizon.write(60);
  servo_vertical.attach(SERVO_VERTICAL); // サーボピンを指定
  servo_vertical.write(40);

  // ボタン
  pinMode(buttonPin, INPUT_PULLUP); // ボタンのピンを入力としてセット、内部プルアップを有効に

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
}

void loop()
{
  getGsrData();
  /*
    serial plotter用
    Serial.print(文字列: 変数,)      の形式で書くことで表示ができる
  */
  Serial.print("GSR:");
  Serial.print(analogRead(GSR_PIN));
  Serial.print(",");
  Serial.print("GSR_filtered:");
  Serial.print(gsr_average);
  Serial.print(",");
  Serial.print("HEART:");
  Serial.print(analogRead(HEART_PIN));
  Serial.print(",");
  Serial.print("HEART_filtered:");
  Serial.print(heart_filteredValue);
  Serial.print(",");

  /*
    緊張の判定
    GSR: 一つ前の値と比べて、下がっていたら緊張している
    BPM: 一定値以上なら緊張している
  */
  if (old_gsr_average - gsr_average > 30 && bpm > bpm_threshold) // 緊張している
  {
    Serial.print("Nervous");
    if (nervous_combo_count < 5) // どれだけmode2で撫でても緊張しているようだったら、mode3に移行する
    {
      moveServo2();
    }
    else
    {
      moveServo3();
    }
    nervous_combo_count++;
    last_nervous_time = millis();
  }
  else // 緊張していない
  {
    Serial.print("NotNervous");
    if (millis() - last_nervous_time > 30000) // 30秒以上緊張していなかったら、
    {
      nervous_combo_count = 0;
    }
    // ここに5分前の通知をするコード
    checkTime();
  }

  /*デモ用で残しておきたい。ボタンで動きを見せられるように*/
  if (buttonFlag == 1 && digitalRead(buttonPin) == LOW)
  {
    if (buttonCount == 1)
    {
      moveServo1();
    }
    else if (buttonCount == 2)
    {
      moveServo2();
    }
    else if (buttonCount == 3)
    {
      moveServo3();
    }
    else if (buttonCount == 4)
    {
      moveServo1_2();
    }
    else if (buttonCount == 5)
    {
      moveServo2_2();
    }
    else if (buttonCount == 6)
    {
      moveServo3_2();
    }
    buttonCount++;
    if (buttonCount > 6)
    {
      buttonCount = 1;
    }
    buttonFlag = 0;
  }
  if (buttonFlag == 0 && digitalRead(buttonPin) == HIGH)
  {
    buttonFlag = 1;
  }

  Serial.print("\n");
}

void Core0a(void *args)
{ // サブCPU(Core0)で実行するプログラム
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

  while (1)
  { // ここで無限ループを作っておく
    // サブで実行するプログラムを書く
    delay(1); // 1/1000秒待つ
    // BLEと一度接続された後にその時刻を継続的に吐き出す処理
    timePrint();
    calculateBPM();
  }
}
