/*!
 * @file RGA.c
 *
 * @mainpage SRS RGA Library
 *
 * @section intro_sec Introduction
 *
 * This is a library for the Stanford Research Systems RGA 100.
 *
 * @section author Author
 *
 * Written by Brett Longworth
 * Original code by Jeff Coogan
 *
 * @section license License
 *
 * BSD license, all text above must be included in any redistribution
 */

#include <Arduino.h>
#include "rga.h"

// work on passing stream object as reference
//RGA::RGA(HardwareSerial& _Serial_port, uint8_t rtsPin) {
//	_rtsPin = rtsPin;
//	Serial_port = &_Serial_port;
//}

RGA::RGA() {
  newData = false;
  packetLength = 0;
}

bool RGA::begin() {
    // start com port
    // Assign RTS pin
    // need 28800,8,N,1 with CTS RTS for SRS RGA
    RGA_SERIAL.attachRts(RGA_RTS);
    RGA_SERIAL.begin(28800, SERIAL_8N1);

    //Check connection
    //if (!get_device_id()) {
    //    return false;
    //}

    //Check filament off
    if (!turnOffFilament()) {
        return false;
    }
  return true;
}

// this doesn't work. Timing issue?
float RGA::status(char *x, int a, int b) {
  RGA_SERIAL.write(x);
  const int BUFFER_SIZE = 30;
  char Var1[BUFFER_SIZE];
  char VarOut[6];
  int rlen = RGA_SERIAL.readBytesUntil(13, Var1, BUFFER_SIZE);
  for ( int i = a; i < b; ++i ) {
    VarOut[i - a] = Var1[ i ];
  }
  float VarNum = atof(VarOut);
  //  String s = String(Var1);
  //  int VarNum = roundf(s.toFloat());
  return VarNum; // return the value
}

bool RGA::turnOffFilament() {
  RGA_SERIAL.write("FL0\r");
  delay(1000);
  return true; //should check turned off here
  flushReadBuffer();
}

bool RGA::turnOnFilament() {
  RGA_SERIAL.write("FL1\r");
  delay(1000);
  return true; //should check turned on here
  flushReadBuffer();
}

bool RGA::calibrateAll() {
  RGA_SERIAL.write("CA\r");
  delay(1000);
  //should check for clean status byte to make sure OK
  flushReadBuffer();
  return true;
}

bool RGA::setNoiseFloor(int noiseFloor) {
  // set noise floor
  char s[10];
  sprintf (s, "NF%d\r", noiseFloor);
  RGA_SERIAL.write(s);
  //delay or check that noise floor set
  //should read status byte and return
  //should query noise floor and check set correctly
  delay(1000);
  flushReadBuffer();
  return true;
}

bool RGA::scanMass(int mass) {
  // don't run if receive is in progress or previous data has not been parsed
  if (packetLength || newData) return false; 
  //flush any garbage from serial read buffer
  flushReadBuffer();
  //start mass scan
  char s[10];
  sprintf(s, "MR%d\r", mass);
  RGA_SERIAL.write(s);
  //check started?
  //return false if not started correctly
  //start serial acq
  packetLength = 4;
  return true;
}

// read data from RGA
long RGA::readMass() {
  readSerial();
  if (newData) {
    newData = false;
    return bytesToCurrent(packet);
  }
  return -1; // not ready yet
}

// Get total pressure
bool RGA::totalPressure() {
  RGA_SERIAL.write("TP?\r");
  return true;
}

void RGA::flushReadBuffer() {
    while (RGA_SERIAL.available() > 0) {
        RGA_SERIAL.read();
    }
}

void RGA::sendCommand(char* command, char* parameter) {
  char s[10];
  sprintf(s, "%s%s\r", command, parameter);
  RGA_SERIAL.write(s);
}

// run constantly in loop()
// set class variable packetLength when packet is expected
// check for newData
// set newData = false once data is processed.
// packet should also be cleared after being processed (or may be OK due to known #bytes)
void RGA::readSerial() {
  static byte ndx = 0;
  byte rc;
  while (RGA_SERIAL.available() > 0 && packetLength && !newData) {
    rc = RGA_SERIAL.read();
    if (ndx == packetLength - 1) { // whole packet received
      packet[ndx] = rc;
      ndx++;
      packet[ndx] = '\0';
      ndx = 0;
      newData = true;
      packetLength = 0;
    } else {
      packet[ndx] = rc;
      ndx++;
    }
  }
}

long RGA::bytesToCurrent(byte cb[4]) {
  long num = cb[0] + cb[1] * 256 + cb[2] * 65536 + cb[3] * 16777216;
  return num;
}