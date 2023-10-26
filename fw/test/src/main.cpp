#include <Arduino.h>
#include <ESP32Servo.h>
// #include <Servo.h> 
// サーボモータの制御のためのライブラリをインクルード

// Variables
int PulseSensorPurplePin = 27; // Pulse Sensor PURPLE WIRE connected to ANALOG
int buttonPin = 14;             // 任意のボタンピンを選択。変更が必要な場合はこちらを変更してください
int servoPin = 32;              // サーボを接続するピン。適宜変更してください

Servo myservo;                 // サーボモータオブジェクトのインスタンスを作成

int Signal;                    // holds the incoming raw data. Signal value can range from 0-1024
int Threshold = 3100;          // Determine which Signal to "count as a beat", and which to ingore.
volatile int BPM;              // BPMを保存する変数
volatile int beatLastTime = 0; // 前回のビートを検知した時刻を保存するための変数

void moveServo()
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
  myservo.attach(servoPin);         // サーボピンを指定
  pinMode(buttonPin, INPUT_PULLUP); // ボタンのピンを入力としてセット、内部プルアップを有効に
  Serial.begin(9600);               // Set's up Serial Communication at certain speed.
  myservo.write(40); 
}

void loop()
{
  Signal = analogRead(PulseSensorPurplePin); // Read the PulseSensor's value.

  if (Signal > Threshold && (millis() - beatLastTime) > 250)
  {                                          // 250msのデバウンス時間
    BPM = 60000 / (millis() - beatLastTime); // BPMを計算
    beatLastTime = millis();                 // 最後にビートを検出した時間を更新
    // Serial.print("BPM: ");
    // Serial.println(BPM);
  }
  // else
  // {
    Serial.println(Signal); // Signal valueをシリアルに送信
  // }

  // ボタンが押されたか確認
  if (digitalRead(buttonPin) == LOW)
  {
    moveServo();
  }

  delay(10);
}
