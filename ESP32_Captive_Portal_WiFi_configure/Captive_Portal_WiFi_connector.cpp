#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "Captive_Portal_WiFi_connector.h"

CPWiFiConfigure::CPWiFiConfigure(int _switchPin, int _ledPin)
  : CPSerial(Serial), server(80), client(), dnsServer() {
  hwSerial = false;
  softAP = true;
  pressTime = 500;
  blinkTime = 300;
  switchPin = _switchPin;
  ledPin = _ledPin;
  baseMacChar[17] = { 0 };
  sprintf(htmlTitle, "Captive Portal WiFi Connector");
  sprintf(boardName, "board");
}

CPWiFiConfigure::CPWiFiConfigure(int _switchPin, int _ledPin, HardwareSerial& port)
  : CPSerial(port), server(80), client(), dnsServer() {
  hwSerial = true;
  softAP = true;
  pressTime = 500;
  blinkTime = 300;
  switchPin = _switchPin;
  ledPin = _ledPin;
  baseMacChar[17] = { 0 };
  sprintf(htmlTitle, "Captive Portal WiFi Connector");
  sprintf(boardName, "board");
}

void CPWiFiConfigure::hwSerialprintln(String str){
  if(hwSerial){
    CPSerial.println(str);
  }
}

void CPWiFiConfigure::hwSerialprint(String str){
  if(hwSerial){
    CPSerial.print(str);
  }
}

bool CPWiFiConfigure::begin() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(switchPin, INPUT_PULLUP);
  if (!LittleFS.begin()) {
    hwSerialprintln("[CPWiFiConfigure] SPIFFS failed, or not present");
    return false;
  } else if (LittleFS.exists(wifi_config)) {
    softAP = false;
    hwSerialprintln("[CPWiFiConfigure] Config exist");
    LittleFS.end();
    return true;
  } else {
    softAP = true;
    hwSerialprintln("[CPWiFiConfigure] start AP");
    // Get MAC address for WiFi station
    uint8_t baseMac[6];
    WiFi.macAddress(baseMac);
    sprintf(baseMacChar, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    char softSSID[10] = { 0 };
    sprintf(softSSID, "%s-%02X%02X%02X", boardName, baseMac[3], baseMac[4], baseMac[5]);

    hwSerialprint("[CPWiFiConfigure] MAC: ");
    hwSerialprintln(baseMacChar);
    hwSerialprint("[CPWiFiConfigure] SSID: ");
    hwSerialprintln(softSSID);

    WiFi.softAP(softSSID);
    IPAddress IP = WiFi.softAPIP();
    hwSerialprint("[CPWiFiConfigure] AP IP address: ");
    hwSerialprintln(WiFi.softAPIP().toString());

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
          digitalWrite(ledPin, HIGH);
        } else {
          digitalWrite(ledPin, LOW);
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
  if (!digitalRead(switchPin)) {
    int count = 0;
    digitalWrite(ledPin, LOW);
    ledStatus = false;
    while (!digitalRead(switchPin)) {
      delay(10);
      count++;
      if (count > pressTime) {
        ledStatus = true;
        digitalWrite(ledPin, HIGH);
        hwSerialprintln("[CPWiFiConfigure] Erase wifi_config");
        LittleFS.begin();
        LittleFS.remove(wifi_config);
        LittleFS.end();
        while (!digitalRead(switchPin)) {
          delay(10);
        }
        return true;
      }
    }
  }
  return false;
}