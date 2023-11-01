#include <Arduino.h>
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
const int numSamples = 5; // 平均を取るサンプル数
int samples[numSamples];
int samples_index = 0;
int samples_total = 0;
const float alpha = 0.8; // RCフィルタの定数 (0 < alpha < 1)
int heart_filteredValue = 0;
const int heart_threshold = 3500;
long time_goal;
int bpm = 0;
const int bpm_threshold = 155; // https://clinic.zenplace.co.jp/335/

// サーボ

void setup()
{
  Serial.begin(9600);
  // GSRセンサ
  // 心拍センサ
  // サーボ
}

void loop()
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

  /*
  心拍センサ
  未装着時2900~3100
  装着時ピーク3500以上
  脈の収縮時(アナログ値が高いとき)を閾値で検出する(クロススレッショルドピーク検出法)
  平均値を出力する
  BPMを計算する
  */
  heart_sensorValue = analogRead(HEART);
  /*移動平均フィルタ*/
  // samples[samples_index] = sensorValue; // サンプルを配列に格納し、totalに加算
  // samples_total += sensorValue;
  // samples_index = (samples_index + 1) % numSamples; // 次のインデックスを計算
  // if (samples_index == 0)                   // 平均を計算してシリアルモニタに出力
  // {
  //   heart_filteredValue = samples_total / numSamples;
  //   Serial.print("heart:");
  //   Serial.print(heart_filteredValue);
  //   Serial.print(",");
  //   samples_total = 0; // totalをリセット
  // }

  /*RCフィルタ*/
  heart_filteredValue = alpha * heart_sensorValue + (1 - alpha) * heart_filteredValue;
  Serial.print("heart:");
  Serial.print(analogRead(HEART));
  Serial.print(",");
  Serial.print("heart_filtered:");
  Serial.print(heart_filteredValue);
  Serial.print(",");

  if (heart_filteredValue > heart_threshold)
  {
    long time_start = millis();
    int beat_time = time_goal - time_start; // 1拍の時間(ms)
    bpm = 60000 / beat_time;            // beat per minute
    Serial.print("bpm:");
    Serial.print(bpm);
    Serial.print(",");
    time_goal = time_start;
  }

  /*
  緊張の判定
  GSR: 数秒前の値と比べて、下がっていたら緊張している
  BPM: 一定値以上なら緊張している
  */
  if (gsr_average < old_gsr_average && bpm > bpm_threshold)
  {
    // 緊張している
    Serial.print("Nervous");
  }
  else if (gsr_average < old_gsr_average && bpm < bpm_threshold)
  {
    // 緊張から回復している
    Serial.print("Recovering");
  }
  else if (gsr_average > old_gsr_average && bpm > bpm_threshold)
  {
    // 緊張ではなく運動している
    Serial.print("Exercising");
  }
  else if (gsr_average > old_gsr_average && bpm < bpm_threshold)
  {
    // 緊張していない
    Serial.print("Not Nervous");
  }

  /*
  サーボ
  BLEで動かす
  amaneさんのと合体
  */

  Serial.print("\n");
}
