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

const String RGA::statusCommands[] = {"EE", "FL", "IE", "VF", "CA", "HV"};

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

bool RGA::turnOffFilament() {
  sendCommand("FL", "0.0");
  float currentRB = getEmissionCurrent();
  if (currentRB < 0.0 - emissionCurrentInc ||
      currentRB > 0.0 + emissionCurrentInc)
  {
    return false;
  }
  return true; 
}

bool RGA::turnOnFilament() {
  sendCommand("FL", emissionCurrent);
  float currentRB = getEmissionCurrent();
  if (currentRB < emissionCurrent - emissionCurrentInc ||
      currentRB > emissionCurrent + emissionCurrentInc)
  {
    return false;
  }
  return true; 
}

bool RGA::setEmissionCurrent(double current)
{
    return false;
}

float RGA::getEmissionCurrent()
{
  sendCommand("FL", "?");
  return readBufferLineASCII().toFloat();
}

bool RGA::calibrateAll() {
  sendCommand("CA");
  flushReadBuffer();
  return true;
}

bool RGA::setNoiseFloor(int noiseFloor) {
  // set noise floor
  sendCommand("NF", String(noiseFloor));
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
  sendCommand("MR", String(mass));
  packetLength = 4;
  return true;
}

// read data from RGA
long RGA::readMass() {
  readSerial();
  // check if packet is available
  if (newData) {
    newData = false;
    return bytesToCurrent(packet);
  }
  return -1; // not ready yet
}

// Get total pressure
float RGA::totalPressure() {
  sendCommand("TP", "?");
  return readBufferLineASCII().toFloat();
  //need to convert return to pressure
}

void RGA::flushReadBuffer() {
    while (RGA_SERIAL.available() > 0) {
        RGA_SERIAL.read();
    }
}

void RGA::sendCommand(String command) {
  String fullCmd = command + "\r";
  Serial.print("Sending command '");
  Serial.print(fullCmd);
  Serial.println("'...");

  Serial.write(fullCmd.c_str());

  if (isStatusReportingCmd(command)) {
    checkStatusByte();
  }
}

void RGA::sendCommand(String command, String parameter = "") {
  String fullCmd = command + parameter + "\r";
  //Serial.print("Sending command '");
  //Serial.print(fullCmd);
  //Serial.println("'...");

  RGA_SERIAL.write(fullCmd.c_str());

  if (parameter != "?" && isStatusReportingCmd(command)) {
    checkStatusByte();
  }
}

bool RGA::isStatusReportingCmd(String command) {
  for (unsigned int i = 0; i < sizeof(statusCommands) / sizeof(statusCommands[0]); i++) {
    if (command == statusCommands[i]) {
      return true;
    }
  }
  return false;
}

void RGA::checkStatusByte()
{
  //Serial.println("Checking status byte...");
  uint8_t statusByte = readBufferChunked(3, 10)[0];  // last two bytes are \n\r
  int binStr[8];
  
  for (int i = 0; i < 8; i++) {
    binStr[i] = (statusByte >> (7 - i)) & 1;
  }

  if (!cdemPresent) {
    binStr[3] = 0;
  }

  for (int i = 0; i < 8; i++) {
    if (binStr[i] == 1) {
      // Error reported in status echo
      Serial.print("Error: Status bit ");
      Serial.print(binStr[i]);
      Serial.println(" is on!");
      // Handle the exception as needed
    }
  }
}


uint8_t* RGA::readBufferChunked(byte lengthBytes, byte attempts = 10) {
  int attemptSleep = 500;
  int recv = 0;
  int recvTotal = 0;
  int attempt = 0;
  int chunkSize = CHUNK_SIZE;
  //Serial.println("Waiting for serial buffer to fill up...");
  char* dataRecv = nullptr;

  if (lengthBytes < CHUNK_SIZE) {
    chunkSize = lengthBytes;
  }

  while (recvTotal < lengthBytes) {
    while ((recv < chunkSize) && (recv + recvTotal != lengthBytes)) {
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

    dataRecv = new char[recv];
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
