#include <Arduino.h>
#include <Wire.h>

#include <SoftwareSerial.h>               //ソフトウェアシリアルライブラリのインポート
SoftwareSerial mySoftwareSerial(10, 11);  //10, 11ピンをソフトウェアシリアルで使用する

#include <DFRobotDFPlayerMini.h>  //DFPlayer miniのライブラリをインポート
DFRobotDFPlayerMini myDFPlayer;   //DFPlayer miniのインスタンス

#include <KXTJ3.h>            //加速度センサモジュールのライブラリをインポート
KXTJ3 kxtj3(KXTJ3_ADDR_LOW);  //加速度センサモジュールのインスタンス

#define SYSTEM_BAUDRATE 115200    //シリアル通信の速度
#define SYSTEM_WAIT 100           // ループごとの待機時間（ms）
#define ERROR_WAIT 1000           // エラー表示の待機時間（ms）
void error_func(int32_t result);  //加速度センサモジュールのエラー表示関数

const int volume = 20;          // ボリューム。最大30
const int bufferIndexMax = 5;  //平均化するバッファサイズ
const int buttonPin = 7;       //ボタンに接続するピン
const int modeNum = 3;         //モードの数

int bufferIndex = 0;                   //バッファのインデックス
float32 accBuffer[3][bufferIndexMax];  //加速度の履歴

float32 tolerance = 0.5;       //出目判定時の許容範囲
float32 stopTolerance = 0.03;  //静止判定時の許容範囲　小さいほど、判定がシビア
float32 shakeTolerance = 0.3;  //揺れ判定時の許容範囲　大きいほど、大きく振る必要がある

bool standbyFlag = true;  //サイコロを振る準備ができているか
int mode = 0;             //選択中のゲームモード。


void setup() {
  pinMode(buttonPin, INPUT_PULLUP);  //ボタンに接続したピンをプルアップの入力として設定する
  Serial.begin(SYSTEM_BAUDRATE);     //シリアル通信を開始

  //-------------------------------------------
  //DFPlayer miniのセットアップ
  mySoftwareSerial.begin(9600);  //

  if (!myDFPlayer.begin(mySoftwareSerial)) {  //DFPlayerとの通信開始
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while (true) {
      delay(0);
    }
  }
  Serial.println(F("DFPlayer Mini online."));

  //-------------------------------------------
  //加速度センサモジュールのセットアップ
  int32_t result;
  float32 version;

  (void)Serial.println("KXTJ3-1057 Sample Code Version : 1.1");
  version = kxtj3.get_version();
  (void)Serial.print("KXTJ3-1057 Driver Version : ");
  (void)Serial.println(version, 1);
  Wire.begin();                   //I2C通信の準備
  result = kxtj3.init();          //加速度センサモジュールの初期化
  if (result == KXTJ3_COMM_OK) {  //通信が成功したら
    (void)kxtj3.start();          //加速度センサモジュールを起動
  } else {
    error_func(result);  //通信に失敗していたらエラーメッセージを表示
  }

  //-------------------------------------------
  myDFPlayer.volume(volume);  //音量（ 0～30 ）
  myDFPlayer.play(7);         //モード1のタイトル
  delay(3000);

  return;
}






void loop() {
  //モードの切替え -------------------------------------
  if (digitalRead(buttonPin) == LOW) {  //ボタンが押された場合
    mode++;
    if (mode >= modeNum) mode = 0;
    Serial.print("MODE : ");
    Serial.println(mode);
    myDFPlayer.play((mode + 1) * 7);  //タイトル音声を再生
    delay(3000);
  }

  // 姿勢の検出 -----------------------------------------
  int32_t result;
  float32 acc[KXTJ3_AXIS_SIZE];
  result = kxtj3.get_val(acc);  //加速度センサの値を取得

  float32 accAve[3] = { 0.0, 0.0, 0.0 };   //バッファの平均
  float32 accDiff[3] = { 0.0, 0.0, 0.0 };  //平均と現在の差


  if (result == KXTJ3_COMM_OK) {  //加速度センサとの通信に成功していたら

    //インデックスを増やし、上限を超えたら0にする
    bufferIndex++;
    if (bufferIndex >= bufferIndexMax) bufferIndex = 0;

    //加速度をバッファに入れる
    accBuffer[0][bufferIndex] = acc[0];
    accBuffer[1][bufferIndex] = acc[1];
    accBuffer[2][bufferIndex] = acc[2];

    //平均する
    for (int i = 0; i < bufferIndexMax; i++) {
      accAve[0] += accBuffer[0][i] / bufferIndexMax;
      accAve[1] += accBuffer[1][i] / bufferIndexMax;
      accAve[2] += accBuffer[2][i] / bufferIndexMax;
    }

    //平均と現在の差
    accDiff[0] = abs(accAve[0] - acc[0]);
    accDiff[1] = abs(accAve[1] - acc[1]);
    accDiff[2] = abs(accAve[2] - acc[2]);

    //平均との差が小さくなる、つまり揺れが収まったら
    if (accDiff[0] < stopTolerance && accDiff[1] < stopTolerance && accDiff[2] < stopTolerance) {
      Serial.println("静止");

      if (standbyFlag == true) {
        Serial.println("再生");
        standbyFlag = false;

        //出目に対応した音声を再生
        if ((-tolerance < acc[0] && acc[0] < tolerance) && (-tolerance < acc[1] && acc[1] < tolerance) && (1 - tolerance < acc[2] && acc[2] < 1 + tolerance)) {
          Serial.println("出目1");
          myDFPlayer.play(1 + mode * 7);
        }else if ((-1 - tolerance < acc[0] && acc[0] < -1 + tolerance) && (-tolerance < acc[1] && acc[1] < tolerance) && (-tolerance < acc[2] && acc[2] < tolerance)) {
          Serial.println("出目2");
          myDFPlayer.play(2 + mode * 7);
        }else if ((-tolerance < acc[0] && acc[0] < tolerance) && (1 - tolerance < acc[1] && acc[1] < 1 + tolerance) && (-tolerance < acc[2] && acc[2] < tolerance)) {
          Serial.println("出目3");
          myDFPlayer.play(3 + mode * 7);
        }else if ((-tolerance < acc[0] && acc[0] < tolerance) && (-1 - tolerance < acc[1] && acc[1] < -1 + tolerance) && (-tolerance < acc[2] && acc[2] < tolerance)) {
          Serial.println("出目4");
          myDFPlayer.play(4 + mode * 7);
        }else if ((1 - tolerance < acc[0] && acc[0] < 1 + tolerance) && (-tolerance < acc[1] && acc[1] < tolerance) && (-tolerance < acc[2] && acc[2] < tolerance)) {
          Serial.println("出目5");
          myDFPlayer.play(5 + mode * 7);
        }else if ((-tolerance < acc[0] && acc[0] < tolerance) && (-tolerance < acc[1] && acc[1] < tolerance) && (-1 - tolerance < acc[2] && acc[2] < -1 + tolerance)) {
          Serial.println("出目6");
          myDFPlayer.play(6 + mode * 7);
        }

        delay(3000);
      }
    }

    //大きく揺れた場合
    if (shakeTolerance < accDiff[0] || shakeTolerance < accDiff[1] || shakeTolerance < accDiff[2]) {
      Serial.println("揺れ");
      standbyFlag = true;  //準備状態に戻す
    }

  } else {
    //加速度センサとの通信に失敗していたら
    (void)Serial.println("Error : Can't get sensor data!");
    error_func(result);
  }

  delay(SYSTEM_WAIT);
}


//加速度センサのエラーメッセージ
void error_func(int32_t result) {
  uint8_t cnt;

  switch (result) {
    case KXTJ3_COMM_ERROR:
      (void)Serial.println("Communication Error.");
      break;

    case KXTJ3_WAI_ERROR:
      (void)Serial.println("ID Error.");
      break;

    case KXTJ3_PARAM_ERROR:
      (void)Serial.println("Parameter Error.");
      break;

    default:
      (void)Serial.println("Unknown Error.");
      break;
  }

  (void)Serial.println("KXTJ3-1057 Check System and Driver Parameter.");
  cnt = 0;
  while (1) {
    (void)Serial.print(".");
    if (cnt < 30) {
      cnt++;
    } else {
      cnt = 0;
      (void)Serial.println();
    }
    delay(ERROR_WAIT);
  }

  return;
}
