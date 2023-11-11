#include <Arduino.h>
#include "rga.h"

RGA rga;
long current;

void setup() {
  Serial.begin(9600);
  delay(3000);
  Serial.println("Teensy ready");
  Serial.println("Starting RGA...");
  rga.begin();
  rga.setNoiseFloor(0);
  rga.calibrateAll();
}

void loop() {
  rga.scanMass(28);
  current = rga.readMass();
  if (current >= 0) {
    Serial.print("Mass 28 current: ");
    Serial.println(current);
  }
}
