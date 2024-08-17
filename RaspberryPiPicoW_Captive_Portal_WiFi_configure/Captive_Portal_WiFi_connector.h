#ifndef CPWiFiConfigure_h
#define CPWiFiConfigure_h

#include "Arduino.h"
#include <WebServer.h>
#include <DNSServer.h>

class CPWiFiConfigure {
  public:
    //no HardwareSerial
    CPWiFiConfigure(int _switchpin, int _ledpin);
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
    int presstime; //10ms
    int blinktime; //ms
    char htmltitle[100]; //enter CaptivePortal html title
    char boardname[100]; //enter Boardname to display on CaptivePortal webpage
  private:
    WebServer server;
    WiFiClient client;
    DNSServer dnsServer;
    HardwareSerial& CPSerial;
    int switchpin;
    int ledpin;
    bool hwserial;
    char baseMacChr[18];
    char roothtml[1000];
    char submithtml[1000];
    void createhtml();
    void printhwserialln(String _str);
    void printhwserial(String _str);
};

#endif