#include <Wire.h>
#include "Adafruit_VL53L1X.h"
#include <BH1745.h>

// --- VL53L1X Distance Sensor ---
#define IRQ_PIN 2
#define XSHUT_PIN 3
Adafruit_VL53L1X vl53 = Adafruit_VL53L1X(XSHUT_PIN, IRQ_PIN);

// --- BH1745 Color Sensor ---
BH1745 bh;

// --- Timing variables ---
unsigned long previousDistanceMillis = 0;
const unsigned long distanceInterval = 50; // VL53L1X timing budget

unsigned long previousColorMillis = 0;
const unsigned long colorInterval = 50;    // color read interval (more frequent than print)

unsigned long previousPrintMillis = 0;
const unsigned long printInterval = 1000;  // Serial print every 1 second

// --- Variables to store sensor readings ---
int16_t distance = -1;
long r, g, b, c;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Wire.begin();

  // --- Initialize VL53L1X ---
  Serial.println(F("Initializing VL53L1X..."));
  if (!vl53.begin(0x29, &Wire)) {
    Serial.print(F("VL53L1X init error: "));
    Serial.println(vl53.vl_status);
    while (1) delay(10);
  }
  Serial.println(F("VL53L1X OK!"));
  if (!vl53.startRanging()) {
    Serial.print(F("Couldn't start ranging: "));
    Serial.println(vl53.vl_status);
    while (1) delay(10);
  }
  vl53.setTimingBudget(distanceInterval);l

  // --- Initialize BH1745 ---
  Serial.println(F("Initializing BH1745..."));
  if (!bh.begin()) {
    Serial.println(F("BH1745 Device Error!"));
    while (1);
  }
  bh.setGain(bh.GAIN_16X);
  bh.setRgbcMode(bh.RGBC_16_BIT);
}

void loop() {
  unsigned long currentMillis = millis();

  // --- VL53L1X Distance Reading ---
  if (currentMillis - previousDistanceMillis >= distanceInterval) {
    previousDistanceMillis = currentMillis;
    if (vl53.dataReady()) {
      distance = vl53.distance();
      vl53.clearInterrupt();
    }
  }

  // --- BH1745 Color Reading ---
  if (currentMillis - previousColorMillis >= colorInterval) {
    previousColorMillis = currentMillis;
    bh.read();
    r = bh.red;
    g = bh.green;
    b = bh.blue;
    c = bh.clear;
  }

  // --- Serial Print every 1 second ---
  if (currentMillis - previousPrintMillis >= printInterval) {
    previousPrintMillis = currentMillis;

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    Serial.print("R: "); Serial.print(r);
    Serial.print(" G: "); Serial.print(g);
    Serial.print(" B: "); Serial.print(b);
    Serial.print(" C: "); Serial.println(c);

    const long REFLECTION_THRESHOLD = 3000;
    bool reflectionDetected = (c > REFLECTION_THRESHOLD);
    Serial.print("Reflection detected: "); Serial.println(reflectionDetected);

    const long BLACK_THRESHOLD = 1000;
    bool isBlack = (r < BLACK_THRESHOLD && g < BLACK_THRESHOLD && b < BLACK_THRESHOLD && c < BLACK_THRESHOLD);
    Serial.print("Is BLACK: "); Serial.println(isBlack);

    bool isRed = (r > g * 1.3) && (r > b * 1.3) && (c >= BLACK_THRESHOLD);
    Serial.print("Is RED: "); Serial.println(isRed);

    bool isBlue = (b > r * 1.3) && (b > g * 1.3) && (c >= BLACK_THRESHOLD);
    Serial.print("Is BLUE: "); Serial.println(isBlue);

    Serial.println("-------------------------");
  }
}
