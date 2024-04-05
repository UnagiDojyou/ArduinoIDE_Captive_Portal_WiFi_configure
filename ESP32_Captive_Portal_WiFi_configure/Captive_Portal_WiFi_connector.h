#ifndef CPWiFiConfigure_h
#define CPWiFiConfigure_h

#include "Arduino.h"

class CPWiFiConfigure {
  public:
    //HardwareSerial CPSerial;
    CPWiFiConfigure(int _switchpin, int _ledpin, HardwareSerial& port);
    
    bool softap;
    bool led;
    bool begin();
    String readSSID();
    String readPASS();
    bool readButton();
    const char* wifi_config = "/WiFi.txt";
    IPAddress IP;
    int presstime;
    int blinktime;
    char *baseMacChr;
  private:
    HardwareSerial& CPSerial;
    int switchpin;
    int ledpin;
    static void handleNotFound();
    static void handleSubmit();
    static void handleRoot();
    /*char baseMacChr[17];
    void handleNotFound();
    void handleSubmit();
    void handleRoot();*/
};

void handleNotFound();
void handleSubmit();
void handleRoot();

#endif