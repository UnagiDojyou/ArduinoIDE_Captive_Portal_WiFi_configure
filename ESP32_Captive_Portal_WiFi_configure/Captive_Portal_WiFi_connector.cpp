#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "Captive_Portal_WiFi_connector.h"

WebServer server(80);
WiFiClient client;
DNSServer dnsServer;

CPWiFiConfigure::CPWiFiConfigure(int _switchpin, int _ledpin, HardwareSerial& port) : CPSerial(port) {
  softap = true;
  presstime = 500;
  blinktime = 300;
  switchpin = _switchpin;
  ledpin = _ledpin;
  baseMacChr[17] = { 0 };
}

bool CPWiFiConfigure::begin() {
  pinMode(switchpin, OUTPUT);
  digitalWrite(switchpin, LOW);
  if (!LittleFS.begin()) {
    CPSerial.println("SPIFFS failed, or not present");
    return false;
  } else if (LittleFS.exists(wifi_config)) {
    softap = false;
    CPSerial.println("Config exist");
    LittleFS.end();
    return true;
  } else {
    softap = true;
    CPSerial.println("start AP");
    // Get MAC address for WiFi station
    uint8_t baseMac[6];
    WiFi.macAddress(baseMac);
    sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    char softSSID[10] = { 0 };
    sprintf(softSSID, "CPWiFiConfigure-%02X%02X%02X", baseMac[3], baseMac[4], baseMac[5]);

    CPSerial.print("MAC: ");
    CPSerial.println(baseMacChr);
    CPSerial.print("SSID: ");
    CPSerial.println(softSSID);

    WiFi.softAP(softSSID);
    IPAddress IP = WiFi.softAPIP();
    CPSerial.print("AP IP address: ");
    CPSerial.println(IP);

    server.on("/", HTTP_GET, handleRoot);
    server.on("/submit", HTTP_POST, handleSubmit);
    server.onNotFound(handleNotFound);

    dnsServer.start(53, "*", IP);
    server.begin();
    int count = 0;
    while (softap) {
      server.handleClient();
      dnsServer.processNextRequest();
      count++;
      delay(1);
      if (count > blinktime) {
        if (!led) {
          led = true;
          digitalWrite(ledpin, HIGH);
        } else {
          digitalWrite(ledpin, LOW);
          led = false;
        }
        count = 0;
      }
      delay(1);
    }
    LittleFS.end();
    dnsServer.stop();
    server.stop();
    return true;
  }
}

void CPWiFiConfigure::handleNotFound() {
  char rooturl[22];
  IPAddress ip = WiFi.localIP();
  sprintf(rooturl, "http//%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  server.send(302, "text/plain", "");
  server.client().stop();
}

void CPWiFiConfigure::handleSubmit() {
  if (server.hasArg("SSID") && server.hasArg("Password")) {
    String staSSID = server.arg("SSID");
    String staPassword = server.arg("Password");
    //CPSerial.println(staSSID);
    //CPSerial.println(staPassword);

    char htmlForm[500];
    snprintf(htmlForm, 500, "<!DOCTYPE html>\
<html>\
<head>\
<title>Raspberry Pi Pico W WiFi Setting</title>\
<meta name=\"viewport\" content=\"width=300\">\
</head>\
<body>\
<h2>Raspberry Pi Pico W WiFi Setting</h2>\
<p>MACaddress<br>%s</p>\
<p>Attempts to connect to %s after 10 seconds.</p>\
<p>When led blinks at 1 second intervals, Raspberry Pi Pico W is trying to connect to WiFi.<br>\
If it continues for a long time, press and hold the reset button for more than 5 seconds and setting WiFi again.</p>\
</body>\
</html>",
             baseMacChr, staSSID.c_str());
    server.send(200, "text/html", htmlForm);

    File file = LittleFS.open(wifi_config, "w");
    file.println(staSSID);      //SSID
    file.println(staPassword);  //password
    file.close();
    softap = false;
  } else {
    server.send(200, "text/plain", "Message not received");
  }
}

void CPWiFiConfigure::handleRoot() {
  char htmlForm[500];
  snprintf(htmlForm, 500, "<!DOCTYPE html>\
<html>\
<head>\
<title>Raspberry Pi Pico W WiFi Setting</title>\
<meta name=\"viewport\" content=\"width=300\">\
</head>\
<body>\
<h2>Raspberry Pi Pico W WiFi Setting</h2>\
<p>MACaddress<br>%s</p>\
<form action=\"/submit\" method=\"POST\">\
SSID<br>\
<input type=\"text\" name=\"SSID\" required\">\<br>\
Password<br>\
<input type=\"text\" name=\"Password\" required\">\<br>\
<input type=\"submit\" value=\"send\">\
</form>\
</body>\
</html>",
           baseMacChr);
  server.send(200, "text/html", htmlForm);
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
  if (!digitalRead(switchpin)) {
    int count = 0;
    digitalWrite(ledpin, LOW);
    while (digitalRead(switchpin)) {
      delay(10);
      count++;
      if (count > presstime) {  //5seconds
        digitalWrite(ledpin, HIGH);
        CPSerial.println("Erase wifi_config");
        LittleFS.remove(wifi_config);
        while (!digitalRead(switchpin)) {
          delay(10);
        }
        return true;
      }
    }
  }
  return false;
}