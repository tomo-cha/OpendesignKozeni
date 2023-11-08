#include <Arduino.h>
#include <ESP32Servo.h>
// GSRセンサ
const int GSR = 34; // 接続するanalog pin
int gsr_sensorValue = 0;
int gsr_average = 0;
int old_gsr_average;
unsigned long previousMillis = 0;
const long interval = 3000;

// 心拍センサ
const int HEART = 35; // 接続するanalog pin
int heart_sensorValue = 0;
const int heart_threshold = 3100;
volatile int bpm = 0;
volatile int beatLastTime = 0; // 前回のビートを検知した時刻を保存するための変数
const int bpm_threshold = 90;  // https://clinic.zenplace.co.jp/335/

//緊張の判定
int nervous_combo_count = 0;
int last_nervous_time = 0;

// サーボ
Servo myservo;     // サーボモータオブジェクトのインスタンスを作成
int servoPin = 32; // サーボを接続するピン。適宜変更してください

// ボタン
int buttonPin = 14; // 任意のボタンピンを選択。変更が必要な場合はこちらを変更してください
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
    delay(5);
  }
  gsr_average = sum / 10;
  Serial.print("GSR:");
  Serial.print(analogRead(GSR));
  Serial.print(",");
  Serial.print("GSR_filtered:");
  Serial.print(gsr_average);
  Serial.print(",");
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    old_gsr_average = gsr_average;
  }
}
void getHeartData()
{
  /*
  心拍センサ
  未装着時2900~3100
  装着時ピーク3500以上
  脈の収縮時(アナログ値が高いとき)を閾値で検出する(クロススレッショルドピーク検出法)
  平均値を出力する
  BPMを計算する
  */
  heart_sensorValue = analogRead(HEART); // Read the PulseSensor's value.

  if (heart_sensorValue > heart_threshold && (millis() - beatLastTime) > 250)
  {                                          // 250msのデバウンス時間
    bpm = 60000 / (millis() - beatLastTime); // BPMを計算
    beatLastTime = millis();                 // 最後にビートを検出した時間を更新
  }
  Serial.println(heart_sensorValue); // Signal valueをシリアルに送信
  Serial.print("bpm:");
  Serial.print(bpm);
  Serial.print(",");
}
void moveServo1()
{ // 低い緊張緩和
  for (int i = 40; i < 120; i += 2)
  {
    myservo.write(i); // サーボを0度の位置に動かす
    delay(30);        // 0.5秒待つ
  }

  for (int i = 120; i > 0; i -= 2)
  {
    myservo.write(i); // サーボを0度の位置に動かす
    delay(50);        // 0.5秒待つ
  }

  for (int i = 0; i < 40; i += 2)
  {
    myservo.write(i); // サーボを0度の位置に動かす
    delay(30);        // 0.5秒待つ
  }
}
void moveServo2()
{ // 中の緊張緩和
}
void moveServo3()
{ // 高い緊張緩和
}

void setup()
{
  Serial.begin(9600);//シリアル通信

  // GSRセンサ

  // 心拍センサ

  // サーボ
  myservo.attach(servoPin); // サーボピンを指定
  myservo.write(40);

  // ボタン
  pinMode(buttonPin, INPUT_PULLUP); // ボタンのピンを入力としてセット、内部プルアップを有効に
}

void loop()
{
  getGsrData();
  getHeartData();
  /*
  緊張の判定
  GSR: 一つ前の値と比べて、下がっていたら緊張している
  BPM: 一定値以上なら緊張している
  */
  if (old_gsr_average - gsr_average > 100 && bpm > bpm_threshold)// 緊張している
  {
    Serial.print("Nervous");
    if (nervous_combo_count < 5)//どれだけmode2で撫でても緊張しているようだったら、mode3に移行する
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
  else//緊張していない
  {
    if (millis() - last_nervous_time > 30000)//30秒以上緊張していなかったら、
    {
      nervous_combo_count = 0;
    }
    //ここに30分前の通知をするコード
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
    buttonFlag == 0;
  }
  if (buttonFlag == 0 && digitalRead(buttonPin) == HIGH)
  {
    buttonFlag == 1;
  }

  Serial.print("\n");
}
