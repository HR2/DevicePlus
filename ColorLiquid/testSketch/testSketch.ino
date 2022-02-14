#include <M5Stack.h>

// Color =======================================
#include "Adafruit_TCS34725.h"
byte gammatable[256]; // our RGB -> eye-recognized gamma color
static uint16_t color16(uint16_t r, uint16_t g, uint16_t b) {
  uint16_t _color;
  _color = (uint16_t)(r & 0xF8) << 8;
  _color |= (uint16_t)(g & 0xFC) << 3;
  _color |= (uint16_t)(b & 0xF8) >> 3;
  return _color;
}
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);




// Accel =======================================
#include <ADXL345.h>
ADXL345 accel(ADXL345_ALT);




// Pressure =======================================
int PRESSURE_PIN = 36;




// Wifi =========================================
#include <WiFi.h>
#include <WiFiUDP.h>
WiFiUDP wifiUdp;

// 家のルーター
const char ssid[] = "Buffalo-G-068D"; //WiFIのSSIDを入力
const char pass[] = "oPfTxJ39sX5Fd"; // WiFiのパスワードを入力
const char *pc_addr = "192.168.11.43";//送信先のIPアドレス
const int pc_port = 50001;//送信先のポート
const int my_port = 50000;  //自身のポート










void setup() {
  // M5Stack =======================================
  M5.begin();
  M5.Power.begin();
  M5.Speaker.begin();//ノイズ対策
  M5.Speaker.mute();//ノイズ対策
  M5.lcd.setTextSize(2);
  Serial.begin(9600);
  pinMode(PRESSURE_PIN, ANALOG);



  // Color =======================================
  while (!tcs.begin()) { //如果color unit未能初始化
    Serial.println("No TCS34725 found ... check your connections");
    M5.Lcd.drawString("No Found sensor.", 50, 100, 4);
    delay(1000);
  }
  tcs.setIntegrationTime(TCS34725_INTEGRATIONTIME_154MS); //Sets the integration time for the TC34725.  设置TC34725的集成时间
  tcs.setGain(TCS34725_GAIN_4X);  //Adjusts the gain on the TCS34725.  调整TCS34725上的增益



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
  M5.Lcd.print("IP address = ");
  M5.Lcd.println(WiFi.localIP());
  wifiUdp.begin(my_port);

}








void loop() {

  //Color ============================================
  uint16_t clear, red, green, blue;
  tcs.getRawData(&red, &green, &blue, &clear);  //Reads the raw red, green, blue and clear channel values.  读取原始的红、绿、蓝和清晰的通道值

  uint32_t sum = clear;
  float r, g, b;
  r = red;
  g = green;
  b = blue;

  r /= sum;
  g /= sum;
  b /= sum;

  r *= 256;
  g *= 256;
  b *= 256;

  uint16_t _color = color16((int)r, (int)g, (int)b);


  //Accel ===============================================
  if (accel.update()) {
    M5.Lcd.setCursor(35,  130); M5.Lcd.print((int)(1000 * accel.getX()));
    M5.Lcd.setCursor(135, 130); M5.Lcd.print((int)(1000 * accel.getY()));
    M5.Lcd.setCursor(235, 130); M5.Lcd.print((int)(1000 * accel.getZ()));
  } else {
    Serial.println("update failed");
    while (1) {
      delay(100);
    }
  }


  // Pressure ==========================================
  int pressure = analogRead(PRESSURE_PIN);
  Serial.print( pressure );
  Serial.println( "" );


  // Display ===================================================
  M5.Lcd.fillRect(0, 0, 320, 120, _color);

  M5.Lcd.setCursor(35, 130); M5.Lcd.print( pressure );

  M5.Lcd.setCursor(35,  150); M5.Lcd.print((int)(1000 * accel.getX()));
  M5.Lcd.setCursor(135, 150); M5.Lcd.print((int)(1000 * accel.getY()));
  M5.Lcd.setCursor(235, 150); M5.Lcd.print((int)(1000 * accel.getZ()));

  

// UDP =====================================================
  sendUDP( String((int)r) + "," + String((int)g) + "," + String((int)b), pc_port );

  
  
  
  
  
  
  delay(1);        // delay in between reads for stability
}










void sendUDP(String _msg, int _port) {
  // UDPを送る ---------------------------------

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
