/*
  Captive_Portal_connector.h
  Created by UnagiDojyou
  https://github.com/UnagiDojyou/ArduinoIDE_Captive_Portal_WiFi_configure
*/

#ifndef CPWiFiConfigure_h
#define CPWiFiConfigure_h

#include "Arduino.h"
#if defined  ESP8266
  #include <ESP8266WebServer.h>
#else
  #include <WebServer.h>
#endif
#include <DNSServer.h>

class CPWiFiConfigure {
  public:
    //no HardwareSerial
    CPWiFiConfigure(int _switchPin, int _ledPin);
    //HardwareSerial CPSerial;
    CPWiFiConfigure(int _switchPin, int _ledPin, HardwareSerial& port);
    
    bool softAP;
    bool ledStatus;
    bool begin();
    String readSSID();
    String readPASS();
    bool readButton();
    const char* wifi_config = "/WiFi.txt";
    IPAddress IP;
    int pressTime; //10ms
    int blinkTime; //ms
    char htmlTitle[100]; //enter CaptivePortal html title
    char boardName[100]; //enter Boardname to display on CaptivePortal webpage
  private:
#if defined  ESP8266
  ESP8266WebServer server;
#else
  WebServer server;
#endif    
    WiFiClient client;
    DNSServer dnsServer;
    HardwareSerial& CPSerial;
    int switchPin;
    int ledPin;
    bool hwSerial;
    char baseMacChar[18];
    char rootHTLM[1000];
    char submitHTML[1000];
    void createHTML();
    void hwSerialprintln(String str);
    void hwSerialprint(String str);
};

#endif