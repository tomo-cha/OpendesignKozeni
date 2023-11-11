#include <Arduino.h>
#include <ESP32Servo.h>
// GSRセンサ
const int GSR = 34; // 接続するanalog pin
int gsr_sensorValue = 0;
int gsr_average = 0;
int old_gsr_average;
unsigned long previousMillis = 0;
const long interval = 250;

// 心拍センサ
const int HEART = 35; // 接続するanalog pin
int heart_sensorValue = 0;
const int heart_threshold = 2100;
volatile int bpm = 0;
volatile int beatLastTime = 0; // 前回のビートを検知した時刻を保存するための変数
const int bpm_threshold = 90;
const int numReadings = 10;
int readings[numReadings]; // the readings from the analog input
int readIndex = 0;         // the index of the current reading
int total = 0;             // the running total
int bpm_filtered = 0;      // the average

// esp32のタイマー割り込み https://garretlab.web.fc2.com/arduino/esp32/examples/ESP32/Timer_RepeatTimer.html
// センサ値は一定時間ごとにタイマー割り込みでセンサ値を取得する
hw_timer_t *timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// 緊張の判定
int nervous_combo_count = 0;
int last_nervous_time = 0;

// サーボ
Servo servo_horizon;           // 左右動きのサーボ
const int SERVO_HORIZON = 32;  // サーボを接続するピン。適宜変更してください
Servo servo_vertical;          // 上下動きのサーボ
const int SERVO_VERTICAL = 33; // サーボを接続するピン。適宜変更してください

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
    gsr_sensorValue = analogRead(GSR);
    sum += gsr_sensorValue;
  }
  gsr_average = sum / 10;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    old_gsr_average = gsr_average;
  }
}
void getHeartData()
{
  /*
    心拍センサ
    脈の収縮時(アナログ値が高いとき)を閾値で検出する(クロススレッショルドピーク検出法)
    平均値を出力する
    BPMを計算する
  */
  heart_sensorValue = analogRead(HEART); // Read the PulseSensor's value.

  if (heart_sensorValue > heart_threshold && (millis() - beatLastTime) > 250)
  {                                          // 250msのデバウンス時間
    bpm = 60000 / (millis() - beatLastTime); // BPMを計算
    beatLastTime = millis();                 // 最後にビートを検出した時間を更新
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
  }
}
void ARDUINO_ISR_ATTR onTimer()
{
  /*タイマー割り込み*/
  getGsrData();
  getHeartData();
}
void moveServo1()
{ // 低い緊張緩和 ちょっと上に上げてちょっと左右に動く
  // 上げる
  for (int i = 40; i > 25; i -= 2)
  {
    servo_vertical.write(i); // サーボを0度の位置に動かす
    delay(30);               // 0.5秒待つ
  }
  //
  for (int i = 60; i < 120; i += 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
  for (int i = 120; i > 0; i -= 2)
  {
    servo_horizon.write(i);
    delay(50);
  }
  for (int i = 0; i < 60; i += 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
  // 下げる
  for (int i = 25; i < 40; i += 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
}
void moveServo2()
{ // 中の緊張緩和 ちょっと下に下げて左右に動く
  // 下げる
  for (int i = 40; i < 55; i += 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
  //
  for (int i = 60; i < 120; i += 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
  for (int i = 120; i > 0; i -= 2)
  {
    servo_horizon.write(i);
    delay(50);
  }
  for (int i = 0; i < 60; i += 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
  // 上げる
  for (int i = 55; i > 40; i -= 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
}
void moveServo3()
{ // 高い緊張緩和 大きく下に下げて左右に動く
  // 上げる
  for (int i = 40; i < 65; i += 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
  //
  for (int i = 60; i < 120; i += 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
  for (int i = 120; i > 0; i -= 2)
  {
    servo_horizon.write(i);
    delay(50);
  }
  for (int i = 0; i < 60; i += 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
  // 下げる
  for (int i = 65; i > 40; i -= 2)
  {
    servo_vertical.write(i);
    delay(30);
  }
}

void setup()
{
  Serial.begin(9600); // シリアル通信

  // GSRセンサ

  // 心拍センサ
  for (int thisReading = 0; thisReading < numReadings; thisReading++)
  {
    readings[thisReading] = 0;
  }

  // タイマー割り込み
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true); // Attach onTimer function to our timer.

  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 50000, true); // 50msごとに呼び出し

  // Start an alarm
  timerAlarmEnable(timer);

  // サーボ
  servo_horizon.attach(SERVO_HORIZON); // サーボピンを指定
  servo_horizon.write(60);
  servo_vertical.attach(SERVO_VERTICAL); // サーボピンを指定
  servo_vertical.write(40);

  // ボタン
  pinMode(buttonPin, INPUT_PULLUP); // ボタンのピンを入力としてセット、内部プルアップを有効に
}

void loop()
{
  Serial.print("GSR:");
  Serial.print(analogRead(GSR));
  Serial.print(",");
  Serial.print("GSR_filtered:");
  Serial.print(gsr_average);
  Serial.print(",");
  Serial.print("HEART:");
  Serial.print(heart_sensorValue);
  Serial.print(",");
  Serial.print("bpm:");
  Serial.print(bpm);
  Serial.print(",");
  Serial.print("bpm_filtered:");
  Serial.print(bpm_filtered);
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
    // ここに30分前の通知をするコード
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
    buttonCount++;
    if (buttonCount > 3)
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
