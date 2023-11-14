#include <Arduino.h>
#include <ESP32Servo.h>
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
int readings[numReadings]; // the readings from the analog input
int readIndex = 0;         // the index of the current reading
int total = 0;             // the running total
int bpm_filtered = 0;      // the average
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
const int SERVO_HORIZON = 21;  // サーボを接続するピン。適宜変更してください
Servo servo_vertical;          // 上下動きのサーボ
const int SERVO_VERTICAL = 19; // サーボを接続するピン。適宜変更してください
int DelayTime;

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
  { // ビート検出条件
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

    // BPM値をBLE経由で通知（関数は既に定義されていると仮定）
    // notifyBPM(bpm);
  }
  Serial.print("bpm:");
  Serial.print(bpm);
  Serial.print(",");
  Serial.print("bpm_filtered:");
  Serial.print(bpm_filtered);
  Serial.print(",");
}
void moveServo1()
{ // 低い緊張緩和 ちょっと上に上げてちょっと左右に動く
  // 上げる
  for (int i = 40; i > 30; i -= 2)
  {
    servo_vertical.write(i); // サーボを0度の位置に動かす
    delay(30);               // 0.5秒待つ
  }
  //
  for (int i = 90; i < 180; i += 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
  for (int i = 180; i > 0; i -= 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
  for (int i = 0; i < 90; i += 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
  // 下げる
  for (int i = 30; i < 40; i += 2)
  {
    servo_vertical.write(i);
    delay(30);
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
  //
  for (int i = 90; i < 180; i += 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
  for (int i = 180; i > 0; i -= 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
  for (int i = 0; i < 90; i += 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
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
  // 左右
  for (int i = 90; i < 180; i += 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
  for (int i = 180; i > 0; i -= 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
  for (int i = 0; i < 90; i += 2)
  {
    servo_horizon.write(i);
    delay(30);
  }
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
  //
  for (int i = 90; i < 180; i += 2)
  {
    servo_horizon.write(i);
    delay(10);
  }
  for (int i = 180; i > 0; i -= 2)
  {
    servo_horizon.write(i);
    delay(10);
  }
  for (int i = 0; i < 90; i += 2)
  {
    servo_horizon.write(i);
    delay(10);
  }
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
  //
  for (int i = 90; i < 180; i += 2)
  {
    servo_horizon.write(i);
    delay(10);
  }
  for (int i = 180; i > 0; i -= 2)
  {
    servo_horizon.write(i);
    delay(10);
  }
  for (int i = 0; i < 90; i += 2)
  {
    servo_horizon.write(i);
    delay(10);
  }
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
  //
  for (int i = 90; i < 180; i += 2)
  {
    servo_horizon.write(i);
    delay(10);
  }
  for (int i = 180; i > 0; i -= 2)
  {
    servo_horizon.write(i);
    delay(10);
  }
  for (int i = 0; i < 90; i += 2)
  {
    servo_horizon.write(i);
    delay(10);
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
  getGsrData();
  calculateBPM();
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
