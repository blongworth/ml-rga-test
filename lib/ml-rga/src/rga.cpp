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
  sendCommand("FL", "0.0");
  return true; //should check turned off here
}

bool RGA::turnOnFilament() {
  sendCommand("FL", emissionCurrent);
  double currentRB = getEmissionCurrent();
  if (currentRB < emissionCurrent - emissionCurrentInc ||
      currentRB > emissionCurrent + emissionCurrentInc))
  {
    return false;
  }
  return true; //should check turned on here
}

bool RGA::setEmissionCurrent(double current)
{
    return false;
}

double RGA::getEmissionCurrent()
{
  sendCommand("FL", "?");
  return toFloat(readBufferLineASCII());
}

bool RGA::calibrateAll() {
  sendCommand("CA");
  flushReadBuffer();
  return true;
}

bool RGA::setNoiseFloor(int noiseFloor) {
  // set noise floor
  char s[10];
  sprintf (s, "NF%d\r", noiseFloor);
  RGA_SERIAL.write(s);
  // sendCommand("NF", noiseFloor);
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
float RGA::totalPressure() {
  sendCommand("TP", "?");
  return toFloat(readBufferLineASCII());
  //need to convert return to pressure
}

void RGA::flushReadBuffer() {
    while (RGA_SERIAL.available() > 0) {
        RGA_SERIAL.read();
    }
}

void RGA::sendCommand(String command, String parameter = "") {
  String fullCmd = command + parameter + "\r";
  Serial.print("Sending command '");
  Serial.print(fullCmd);
  Serial.println("'...");

  Serial.write(fullCmd.c_str());

  if (parameter != "?" && isStatusReportingCommand(cmd)) {
    checkStatusByte();
  }
}

bool RGA::isStatusReportingCommand(String cmd) {
  for (int i = 0; i < sizeof(statusCommands) / sizeof(statusCommands[0]); i++) {
    if (cmd == statusCommands[i]) {
      return true;
    }
  }
  return false;
}

void RGA::checkStatusByte()
{
  Serial.println("Checking status byte...");
  uint8_t statusByte = readBufferChunked(3, 10)[0];  // last two bytes are \n\r
  int binStr[8];
  
  for (int i = 0; i < 8; i++) {
    binStr[i] = (statusByte >> (7 - i)) & 1;
  }

  if (!_cdemPresent) {
    binStr[3] = 0;
  }

  for (int i = 0; i < 8; i++) {
    if (binStr[i] == 1 && _statusErrorBits[i]) {
      // Error reported in status echo
      char errorMsg[50];  // Adjust the size according to your needs
      snprintf(errorMsg, sizeof(errorMsg), "Error %s reported in status echo!", _statusErrorBits[i]);
      Serial.println(errorMsg);
      // Handle the exception as needed
    }
  }
}

#define CHUNK_SIZE 64

uint8_t* RGA::readBufferChunked(int lengthBytes, int attempts = 10) {
  int attemptSleep = 500;
  int recv = 0;
  int recvTotal = 0;
  int attempt = 0;
  Serial.println("Waiting for serial buffer to fill up...");
  uint8_t* dataRecv = nullptr;

  if (lengthBytes < CHUNK_SIZE) {
    CHUNK_SIZE = lengthBytes;
  }

  while (recvTotal < lengthBytes) {
    while ((recv < CHUNK_SIZE) && (recv + recvTotal != lengthBytes)) {
      attempt++;
      recv = Serial.available();
      delay(attemptSleep);
      if (attempt > attempts) {
        // Failed to receive buffer
        Serial.println("Failed to receive buffer");
        // Handle the exception as needed
      }
      //Serial.print(recv);
      //Serial.print(" bytes buffered, ");
      //Serial.print(recvTotal);
      //Serial.print(" bytes received, out of ");
      //Serial.println(lengthBytes);
    }

    dataRecv = new uint8_t[recv];
    Serial.readBytes(dataRecv, recv);
    recvTotal += recv;
    delay(attemptSleep);
    recv = Serial.available();
  }

  uint8_t* bufferBytes = new uint8_t[lengthBytes];
  memcpy(bufferBytes, dataRecv, lengthBytes);
  delete[] dataRecv;

  Serial.print("Serial buffer is received: ");
  for (int i = 0; i < lengthBytes; i++) {
    Serial.print(bufferBytes[i]);
    Serial.print(" ");
  }
  Serial.println();

  return bufferBytes;
}

String RGA::readBufferLineASCII()
{
  //Serial.println("Reading a line from serial port...");
  String bufferAscii;

    while (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        break; // Stop reading when newline or carriage return is encountered
      }
      bufferAscii += c;
    }

    // Read the extra byte required due to \n\r line termination
    while (Serial.available() > 0) {
      Serial.read();
    }

    bufferAscii.trim(); // Remove leading and trailing whitespaces

  //Serial.print("Received line from serial port: '");
  //Serial.print(bufferAscii);
  //Serial.println("'");

  return bufferAscii;
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
