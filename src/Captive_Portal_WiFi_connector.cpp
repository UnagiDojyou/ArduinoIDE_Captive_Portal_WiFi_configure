#include <Arduino.h>
#include <LittleFS.h>
#if defined  ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
#else
  #include <WiFi.h>
  #include <WebServer.h>
#endif
#include <DNSServer.h>
#include "Captive_Portal_WiFi_connector.h"

CPWiFiConfigure::CPWiFiConfigure(int _buttonPin, int _ledPin)
  : CPSerial(Serial), server(80), client(), dnsServer() {
  ENSerial = false;
  if (_ledPin < 0) {
    ledPin = -_ledPin;
    ledPullup = true;
  } else {
    ledPin = _ledPin;
    ledPullup = false;
  }
  if (_buttonPin < 0) {
    buttonPin = -_buttonPin;
    buttonPulldown = true;
  } else {
    buttonPin = _buttonPin;
    buttonPulldown = false;
  }
}

CPWiFiConfigure::CPWiFiConfigure(int _buttonPin, int _ledPin, Stream& port)
  : CPSerial(port), server(80), client(), dnsServer() {
  ENSerial = true;
  if (_ledPin < 0) {
    ledPin = -_ledPin;
    ledPullup = true;
  } else {
    ledPin = _ledPin;
    ledPullup = false;
  }
  if (_buttonPin < 0) {
    buttonPin = -_buttonPin;
    buttonPulldown = true;
  } else {
    buttonPin = _buttonPin;
    buttonPulldown = false;
  }
}

void CPWiFiConfigure::Serialprintln(String str){
  if(ENSerial){
    CPSerial.println(str);
  }
}

void CPWiFiConfigure::Serialprint(String str){
  if(ENSerial){
    CPSerial.print(str);
  }
}

bool CPWiFiConfigure::begin() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledPullup^LOW);
  pinMode(buttonPin, INPUT_PULLUP);
  if (!LittleFS.begin()) {
    Serialprintln("[CPWiFiConfigure] SPIFFS failed, or not present");
    return false;
  } else if (LittleFS.exists(wifi_config)) {
    softAP = false;
    Serialprintln("[CPWiFiConfigure] Config exist");
    LittleFS.end();
    return true;
  } else {
    softAP = true;
    Serialprintln("[CPWiFiConfigure] start AP");
    // Get MAC address for WiFi station
    uint8_t baseMac[6];
    WiFi.macAddress(baseMac);
    sprintf(baseMacChar, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    char softSSID[10] = { 0 };
    sprintf(softSSID, "%s-%02X%02X%02X", boardName, baseMac[3], baseMac[4], baseMac[5]);

    Serialprint("[CPWiFiConfigure] MAC: ");
    Serialprintln(baseMacChar);
    Serialprint("[CPWiFiConfigure] SSID: ");
    Serialprintln(softSSID);

    WiFi.softAP(softSSID);
    IPAddress IP = WiFi.softAPIP();
    Serialprint("[CPWiFiConfigure] AP IP address: ");
    Serialprintln(WiFi.softAPIP().toString());

    createHTML();
    server.on("/", HTTP_GET, [this]() {
      this->server.send(200, "text/html", this->rootHTLM);
    });
    server.on("/submit", HTTP_POST, [this]() {
      if (server.hasArg("SSID") && server.hasArg("Password")) {
        String staSSID = this->server.arg("SSID");
        String staPassword = this->server.arg("Password");
        this->server.send(200, "text/html", this->submitHTML);
        LittleFS.begin();
        File file = LittleFS.open(wifi_config, "w");
        file.println(staSSID);      //SSID
        file.println(staPassword);  //password
        file.close();
        LittleFS.end();
        this->softAP = false;
      } else {
        this->server.send(200, "text/html", this->rootHTLM);
      }
    });
    server.onNotFound([IP, this]() {
      char rooturl[22];
      sprintf(rooturl, "http://%d.%d.%d.%d", IP[0], IP[1], IP[2], IP[3]);
      this->server.sendHeader("Location", rooturl, true);
      this->server.send(302, "text/plain", "");
      this->server.client().stop();
    });

    dnsServer.start(53, "*", IP);
    server.begin();
    int count = 0;
    while (softAP) {
      server.handleClient();
      dnsServer.processNextRequest();
      count++;
      if (count > blinkTime) {
        if (!ledStatus) {
          ledStatus = true;
          digitalWrite(ledPin, ledPullup^HIGH);
        } else {
          digitalWrite(ledPin, ledPullup^LOW);
          ledStatus = false;
        }
        count = 0;
      }
      delay(1);
    }
    LittleFS.end();
    dnsServer.stop();
    server.stop();
    WiFi.softAPdisconnect(true);

    return true;
  }
}

void CPWiFiConfigure::createHTML() {
  snprintf(rootHTLM, 1000, "<!DOCTYPE html><html><head><title>%s</title>\
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body>\
<h2>%s</h2>\
<p>MACaddress<br>%s</p>\
<form action=\"/submit\" method=\"POST\">\
SSID<br>\
<input type=\"text\" name=\"SSID\" required><br>\
Password<br>\
<input type=\"text\" name=\"Password\" required><br>\
<input type=\"submit\" value=\"send\">\
</form></body></html>",
           htmlTitle, htmlTitle, baseMacChar);
  snprintf(submitHTML, 1000, "<!DOCTYPE html><html><head><title>%s</title>\
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body>\
<h2>%s</h2>\
<p>MACaddress<br>%s</p>\
<p>Attempts to connect to WiFi after 10 seconds.</p>\
<p>When LED blinks at 1 second intervals, %s is trying to connect to WiFi.<br>\
If it continues for a long time, press and hold the reset button for more than %d seconds and setting WiFi again.</p>\
</body></html>",
           htmlTitle, htmlTitle, baseMacChar, boardName, pressTime / 100);
}

String CPWiFiConfigure::readSSID() {
  LittleFS.begin();
  File file = LittleFS.open(wifi_config, "r");
  String staSSID = file.readStringUntil('\n');
  file.close();
  LittleFS.end();
  staSSID.replace("\r", "");
  return staSSID;
}

String CPWiFiConfigure::readPASS() {
  LittleFS.begin();
  File file = LittleFS.open(wifi_config, "r");
  String staPASS = file.readStringUntil('\n');
  staPASS = "";
  if (file.available()) {
    staPASS = file.readStringUntil('\n');
  }
  file.close();
  LittleFS.end();
  staPASS.replace("\r", "");
  return staPASS;
}

bool CPWiFiConfigure::readButton() {
  if (buttonPulldown^!digitalRead(buttonPin)) {
    int count = 0;
    digitalWrite(ledPin, ledPullup^LOW);
    ledStatus = false;
    while (buttonPulldown^!digitalRead(buttonPin)) {
      delay(10);
      count++;
      if (count > pressTime) {
        ledStatus = true;
        digitalWrite(ledPin, ledPullup^HIGH);
        Serialprintln("[CPWiFiConfigure] Erase wifi_config");
        LittleFS.begin();
        LittleFS.remove(wifi_config);
        LittleFS.end();
        while (buttonPulldown^!digitalRead(buttonPin)) {
          delay(10);
        }
        return true;
      }
    }
  }
  return false;
}