/*
  Captive_Portal_connector.h
  Created by UnagiDojyou
  https://github.com/UnagiDojyou/ArduinoIDE_Captive_Portal_WiFi_configure
*/

#ifndef CPWiFiConfigure_h
#define CPWiFiConfigure_h

#include "Arduino.h"
#if defined  ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
#else
  #include <WiFi.h>
  #include <WebServer.h>
#endif
#include <DNSServer.h>

class CPWiFiConfigure {
  public:
    //no Serial
    CPWiFiConfigure(int _buttonPin, int _ledPin);
    //port=Serial
    CPWiFiConfigure(int _buttonPin, int _ledPin, Stream& port);
    
    bool ledPullup; //If led is pulluped, _ledPin is the GPIO number with the sign reversed ,or CPWiFi.ledPullup = true.
    bool buttonPulldown; //If is pulldowned, _buttonPin is the GPIO number with the sign reversed ,or CPWiFi.buttonPulldown = true.
    bool softAP = true;
    bool ledStatus;
    bool begin();
    //kept for backward compatibility; return the values loaded by begin()
    String readSSID();
    String readPASS();
    bool readButton();
    //Settings loaded from the wifi_config file by begin()
    String staSSID;
    String staPassword;
    String staHostname; //empty string if not set
    String staIPMode;   //"dhcp" or "static", "dhcp" if not set
    IPAddress staIP;       //IP address, 0.0.0.0 if dhcp or not set
    IPAddress staSubnet;   //subnet mask, 0.0.0.0 if dhcp or not set
    IPAddress staGateway;  //default gateway, 0.0.0.0 if dhcp or not set
    IPAddress staDNS;      //DNS server, 0.0.0.0 if not set
    const char* wifi_config = "/WiFi.txt";
    IPAddress IP;
    int pressTime = 500; //10ms
    int blinkTime = 300; //ms
    char htmlTitle[100] = "Captive Portal WiFi Connector"; //enter CaptivePortal html title
    char boardName[25] = "board"; //enter Boardname to display on CaptivePortal webpage
  private:
#if defined  ESP8266
    ESP8266WebServer server;
#else
    WebServer server;
#endif    
    WiFiClient client;
    DNSServer dnsServer;
    Stream& CPSerial;
    uint8_t buttonPin;
    uint8_t ledPin;
    bool ENSerial;
    char baseMacChar[18] = { 0 };
    char softSSID[32] = { 0 }; //AP mode SSID (boardName-XXXXXX), also the default hostname
    bool hasSubmitted = false; //true once a form submission was made; the pre-fill then uses the submitted values
    //raw values of the last submission, used to pre-fill the form after an error
    String formIP, formSubnet, formGateway, formDNS;
    char rootHTLM[2048];
    char submitHTML[1000];
    char formErrors[512] = { 0 }; //error text shown in red under the MAC address when the last submission had invalid values
    void createHTML();
    void Serialprintln(String str);
    void Serialprint(String str);
};

#endif