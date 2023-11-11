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

#define RGA_SERIAL Serial4 // serial port for RGA
#define RGA_RTS 3 // RTS pin for RGA
#define MAX_PACKET_LENGTH 50
#define CHUNK_SIZE 64

/* TODO:
 * send_command
 * check_status_byte
 * list of status reporting commands
*/


class RGA {
public:
    //RGA(HardwareSerial& _Serial_port, uint8_t rtsPin);
    RGA();
    bool begin();
    bool setNoiseFloor(int noiseFloor);
    bool scanMass(int mass);
    long readMass();
    float totalPressure();
    bool turnOffFilament();
    bool turnOnFilament();
    bool setEmissionCurrent(double current);
    double getEmissionCurrent();
    bool calibrateAll();
    //bool get_device_id();

private:
    static const char* statusCommands[];
    float emissionCurrent = 1.0;
    float emissionCurrentInc = 0.1;
    static const bool cdemPresent = true;
    bool newData;
    int packetLength;
    byte packet[MAX_PACKET_LENGTH];
    void readSerial();
    void flushReadBuffer();
    //HardwareSerial* Serial_port;
    void sendCommand(String command);
    void sendCommand(String command, String parameter);
    bool isStatusReportingCmd(String command);
    void checkStatusByte();
    uint8_t* readBufferChunked(byte lengthBytes, byte attempts);
    String readBufferLineASCII();

    // needs expected return
    long bytesToCurrent(byte cb[4]);
};

#endif
