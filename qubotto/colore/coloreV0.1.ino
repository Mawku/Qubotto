#include <Arduino.h>
#include <Wire.h>
#include <BH1745.h>

BH1745 bh;  

void setup() {
  Serial.begin(115200);
  Wire.begin();  

  bool result = bh.begin(); 
  if (!result){
    Serial.println("BH1745 Device Error!");
    while (1);  
  }

  Serial.println("BH1745 OK");

  bh.setGain(bh.GAIN_16X);         
  bh.setRgbcMode(bh.RGBC_16_BIT);  // ✅ use 16-bit mode for reliable color detection
}

void loop() {
  bh.read();

  long r = bh.red;
  long g = bh.green;
  long b = bh.blue;
  long c = bh.clear;

  // Print raw RGB and clear values
  Serial.print("R: "); Serial.print(r);
  Serial.print(" G: "); Serial.print(g);
  Serial.print(" B: "); Serial.print(b);
  Serial.print(" C: "); Serial.println(c);

  // ---- Reflection detection ----
  const long REFLECTION_THRESHOLD = 3000;  // adjust for your LED
  bool reflectionDetected = (c > REFLECTION_THRESHOLD);
  Serial.print("Reflection detected: ");
  Serial.println(reflectionDetected);

  // ---- Black detection ----
  const long BLACK_THRESHOLD = 1000;  
  bool isBlack = (r < BLACK_THRESHOLD &&
                  g < BLACK_THRESHOLD &&
                  b < BLACK_THRESHOLD &&
                  c < BLACK_THRESHOLD);
  Serial.print("Is BLACK: ");
  Serial.println(isBlack);

  // ---- Red detection ----
  bool isRed = (r > g * 1.3) && (r > b * 1.3) && (c >= BLACK_THRESHOLD);
  Serial.print("Is RED: ");
  Serial.println(isRed);

  // ---- Blue detection ----
  bool isBlue = (b > r * 1.3) && (b > g * 1.3) && (c >= BLACK_THRESHOLD);
  Serial.print("Is BLUE: ");
  Serial.println(isBlue);

  Serial.println("-------------------------");
  delay(300);
}
