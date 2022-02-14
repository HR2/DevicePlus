// M5Stack =======================================
#include <M5Stack.h>

// Color =======================================
#include "Adafruit_TCS34725.h"
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
static uint16_t color16(uint16_t r, uint16_t g, uint16_t b) {
  uint16_t _color;
  _color = (uint16_t)(r & 0xF8) << 8;
  _color |= (uint16_t)(g & 0xFC) << 3;
  _color |= (uint16_t)(b & 0xF8) >> 3;
  return _color;
}

// Accel =======================================
#include <ADXL345.h>
ADXL345 accel(ADXL345_ALT);

// Pressure =======================================
int PRESSURE_PIN = 36;


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
}


void loop() {
  //Color ============================================
  uint16_t clear, red, green, blue;
  tcs.getRawData(&red, &green, &blue, &clear);

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

  uint16_t _color = color16( (int)r, (int)g, (int)b );

  //Accel ===============================================
  int accX, accY, accZ;
  if (accel.update()) {
    accX = (int)(1000 * accel.getX() );
    accY = (int)(1000 * accel.getY() );
    accZ = (int)(1000 * accel.getZ() );
  } else {
    Serial.println("update failed");
    while (1) {
      delay(100);
    }
  }

  // Pressure ==========================================
  int pressure = analogRead(PRESSURE_PIN);


  // Display ===================================================
  M5.Lcd.clear(_color);

  M5.Lcd.setCursor(0, 10);
  M5.Lcd.print( "pressure:" );
  M5.Lcd.println( pressure );

  M5.Lcd.print( "r:" );
  M5.Lcd.println( r );
  M5.Lcd.print( "g:" );
  M5.Lcd.println( g );
  M5.Lcd.print( "b:" );
  M5.Lcd.println( b );

  M5.Lcd.print( "accX:" );
  M5.Lcd.println( accX );
  M5.Lcd.print( "accY:" );
  M5.Lcd.println( accY );
  M5.Lcd.print( "accZ:" );
  M5.Lcd.println( accZ );


  delay(1);
}
