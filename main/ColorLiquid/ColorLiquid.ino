#include <M5Stack.h>//M5Stackの機能を読み込み

// Color =======================================
#include "Adafruit_TCS34725.h"//ライブラリ読み込み
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_16X);//カラーセンサーのオブジェクトを生成

float captureColor[3] = {0, 0, 0}; //保存した色
float white[3] = {275, 395, 490};//白とする基準の値
float black[3] = {92, 123, 144};//黒とする基準の値

// Accel =======================================
#include <ADXL345.h>//ライブラリ読み込み
ADXL345 accel(ADXL345_ALT);//加速度センサーのオブジェクト生成
int acceleration[3] = {0,0,0};//加速度センサーの値

// Pressure =======================================
int PRESSURE_PIN = 36;//センサーを接続するピン
int pressure = 0;//圧力センサーの値
int pressureThreshold = 3000;//センサーが反応するしきい値

// Wifi =========================================
#include <WiFi.h>//ライブラリの読み込み
#include <WiFiUDP.h>
WiFiUDP wifiUdp;//UDP通信のオブジェクトを生成

//ネットワーク情報
const char ssid[] = "Buffalo-G-068D"; //WiFIのSSIDを入力
const char pass[] = "oPfTxJ39sX5Fd"; // WiFiのパスワードを入力
const char *pc_addr = "192.168.11.43";//送信先のIPアドレス
const int pc_port = 50001;//送信先のポート
const int my_port = 50000;  //自身のポート

// Other =========================================
bool isDebug = false;//デバッグモードのフラグ


void setup() {
  // M5Stack =======================================
  M5.begin();
  M5.Power.begin();
  M5.Speaker.begin();//ノイズ対策
  M5.Speaker.mute();//ノイズ対策
  M5.lcd.setTextSize(2);
  pinMode(PRESSURE_PIN, ANALOG);

  // Color =======================================
  while (!tcs.begin()) {
    Serial.println("No TCS34725 found ... check your connections");
    M5.Lcd.drawString("No Found sensor.", 50, 100, 4);
    delay(1000);
  }
  tcs.setIntegrationTime(TCS34725_INTEGRATIONTIME_154MS);
  tcs.setGain(TCS34725_GAIN_4X);

  // Accel =======================================
  byte deviceID = accel.readDeviceID();
  if (deviceID != 0) {
    Serial.print("0x");
    Serial.print(deviceID, HEX);
    Serial.println("");
  } else {
    Serial.println("read device id: failed");
    while (1) {
      delay(100);
    }
  }

  if (!accel.writeRate(ADXL345_RATE_200HZ)) {
    Serial.println("write rate: failed");
    while (1) {
      delay(100);
    }
  }

  if (!accel.writeRange(ADXL345_RANGE_16G)) {
    Serial.println("write range: failed");
    while (1) {
      delay(100);
    }
  }

  if (!accel.start()) {
    Serial.println("start: failed");
    while (1) {
      delay(100);
    }
  }

  // UDP --------------------------------------------
  WiFi.begin(ssid, pass);
  while ( WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }
  wifiUdp.begin(my_port);

}


void loop() {

  //Color ============================================
  uint16_t red, green, blue, clear;
  tcs.getRawData(&red, &green, &blue, &clear);//カラーセンサーから値を取得する

  M5.update();
  if ( M5.BtnA.wasPressed() ) {//左ボタンが押されたら、現在の色を白の基準とする
    white[0] = red;
    white[1] = green;
    white[2] = blue;
  }

  if ( M5.BtnB.wasPressed() ) {//中ボタンが押されたら、現在の色を黒の基準とする
    black[0] = red;
    black[1] = green;
    black[2] = blue;
  }

  float adjustColor[3];
  adjustColor[0] = (red - black[0]) / (white[0] - black[0]) * 255; //白と黒を基準に色を調整する
  adjustColor[1] = (green - black[1]) / (white[1] - black[1]) * 255;
  adjustColor[2] = (blue - black[2]) / (white[2] - black[2]) * 255;
  adjustColor[0] = constrain( adjustColor[0], 0, 255 );//0~255に納める
  adjustColor[1] = constrain( adjustColor[1], 0, 255 );
  adjustColor[2] = constrain( adjustColor[2], 0, 255 );

  // Pressure ==========================================
  pressure = analogRead(PRESSURE_PIN);

  //Accel ===============================================
  if (accel.update()) {
    acceleration[0] = (int)(1000 * accel.getX());
    acceleration[1] = (int)(1000 * accel.getY());
    acceleration[2] = (int)(1000 * accel.getZ());
  } else {
    Serial.println("update failed");
    while (1) {
      delay(100);
    }
  }

  // Processing  ==========================================
  if ( pressure > pressureThreshold ) {//圧力センサーの値が閾値を超えた時、
    captureColor[0] = adjustColor[0];//現在の色を保持
    captureColor[1] = adjustColor[1];
    captureColor[2] = adjustColor[2];
  }

  //取り込んだ色とZ軸の加速度の値をUDPで送信する
  sendUDP( String( int(captureColor[0]) ) + "," + 
           String( int(captureColor[1]) ) + "," + 
           String( int(captureColor[2]) ) + "," + 
           String( acceleration[2] ) ,
           pc_port );

  // Display ===================================================
  uint16_t capColor = M5.Lcd.color565( captureColor[0], captureColor[1], captureColor[2] );//uint16に変換
  M5.Lcd.clear(capColor);

  uint16_t adjColor = M5.Lcd.color565( adjustColor[0], adjustColor[1], adjustColor[2] );//uint16に変換
  int radius = int( ((float)pressure / pressureThreshold) * 120);
  M5.Lcd.fillCircle(160, 120, radius, adjColor);

  // Debug =====================================================
  if ( M5.BtnC.wasPressed() ) {//右ボタンが押されたら、数値の表示を切り替える
    isDebug = !isDebug;
  }
  
  if ( isDebug ) {
    M5.Lcd.setCursor(0, 10);

    M5.Lcd.print("IP address:");
    M5.Lcd.println(WiFi.localIP());

    M5.Lcd.print( "pressure:" );
    M5.Lcd.println( pressure );

    M5.Lcd.print( "raw R:" );
    M5.Lcd.println( red );
    M5.Lcd.print( "raw G:" );
    M5.Lcd.println( green );
    M5.Lcd.print( "raw B:" );
    M5.Lcd.println( blue );

    M5.Lcd.print( "adj R:" );
    M5.Lcd.println( adjustColor[0] );
    M5.Lcd.print( "adj G:" );
    M5.Lcd.println( adjustColor[1] );
    M5.Lcd.print( "adj B:" );
    M5.Lcd.println( adjustColor[2] );

    M5.Lcd.print( "accX:" );
    M5.Lcd.println( acceleration[0] );
    M5.Lcd.print( "accY:" );
    M5.Lcd.println( acceleration[1] );
    M5.Lcd.print( "accX:" );
    M5.Lcd.println( acceleration[2] );
  }

  delay(1);        // delay in between reads for stability
}


void sendUDP(String _msg, int _port) {
  int len = _msg.length() + 1;//文字数
  char charArray[len];//Char型の配列を用意
  _msg.toCharArray( charArray, len );//StringからChar型配列に変換

  uint8_t message[len];//Uint8型の配列を用意
  for (int i = 0; i < len; i++) {
    message[i] = uint8_t(charArray[i]);//一つずつキャストしながら代入
  }

  wifiUdp.beginPacket(pc_addr, _port);//パケット開始
  wifiUdp.write(message, sizeof(message));
  wifiUdp.endPacket();//パケット終了
}
