/*!
   * @file RGA.h
   * 
   * SRS RGA 100 communication library
   * 
   * MACHINE Lab
   * WHOI
   * 
   * Written by Brett Longworth
   * Original code by Jeff Coogan
   * 
   * BSD License
*/
#ifndef RGA_h
#define RGA_h

#include <Arduino.h>

#define RGA_SERIAL Serial3 // serial port for RGA
#define RGA_RTS 15 // RTS pin for RGA
#define MAX_PACKET_LENGTH 50


class RGA {
public:
    //RGA(HardwareSerial& _Serial_port, uint8_t rtsPin);
    RGA();
    bool begin();
    float status(char *x, int a, int b);
    bool setNoiseFloor(int noiseFloor);
    bool scanMass(int mass);
    long readMass();
    bool totalPressure();
    bool turnOffFilament();
    bool turnOnFilament();
    bool calibrateAll();
    //bool get_device_id();

private:
    bool newData;
    int packetLength;
    byte packet[MAX_PACKET_LENGTH];
    void readSerial();
    void flushReadBuffer();
    //HardwareSerial* Serial_port;
    void sendCommand(char* command, char* parameter);
    long bytesToCurrent(byte cb[4]);
};

#endif