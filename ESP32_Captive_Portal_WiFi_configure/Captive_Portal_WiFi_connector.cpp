#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "Captive_Portal_WiFi_connector.h"

WebServer server(80);
WiFiClient client;
DNSServer dnsServer;

CPWiFiConfigure::CPWiFiConfigure(int _switchpin, int _ledpin, HardwareSerial& port)
  : CPSerial(port) {
  softap = true;
  presstime = 500;
  blinktime = 300;
  switchpin = _switchpin;
  ledpin = _ledpin;
  baseMacChr[17] = { 0 };
  sprintf(htmltitle, "Captive Portal WiFi Connector");
  sprintf(boardname, "board");
}

bool CPWiFiConfigure::begin() {
  pinMode(ledpin, OUTPUT);
  digitalWrite(ledpin, LOW);
  pinMode(switchpin,INPUT_PULLUP);
  if (!LittleFS.begin(true)) {
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
    sprintf(softSSID, "%s-%02X%02X%02X", boardname, baseMac[3], baseMac[4], baseMac[5]);

    CPSerial.print("MAC: ");
    CPSerial.println(baseMacChr);
    CPSerial.print("SSID: ");
    CPSerial.println(softSSID);

    WiFi.softAP(softSSID);
    IPAddress IP = WiFi.softAPIP();
    CPSerial.print("AP IP address: ");
    CPSerial.println(IP);

    createhtml();
    server.on("/", HTTP_GET, [this]() {
      server.send(200, "text/html", this->roothtml);
    });
    server.on("/submit", HTTP_POST, [this]() {
      if (server.hasArg("SSID") && server.hasArg("Password")) {
        String staSSID = server.arg("SSID");
        String staPassword = server.arg("Password");
        server.send(200, "text/html", this->submithtml);
        LittleFS.begin();
        File file = LittleFS.open(wifi_config, "w");
        file.println(staSSID);      //SSID
        file.println(staPassword);  //password
        file.close();
        LittleFS.end();
        this->softap = false;
      } else {
        server.send(200, "text/html", this->roothtml);
      }
    });
    server.onNotFound([IP]() {
      char rooturl[22];
      sprintf(rooturl, "http://%d.%d.%d.%d", IP[0], IP[1], IP[2], IP[3]);
      server.sendHeader("Location", rooturl, true);
      server.send(302, "text/plain", "");
      server.client().stop();
    });

    dnsServer.start(53, "*", IP);
    server.begin();
    int count = 0;
    while (softap) {
      server.handleClient();
      dnsServer.processNextRequest();
      count++;
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
      //delay(1);
    }
    LittleFS.end();
    dnsServer.stop();
    server.stop();
    WiFi.softAPdisconnect(true);

    return true;
  }
}

void CPWiFiConfigure::createhtml() {
  snprintf(roothtml, 1000, "<!DOCTYPE html><html><head><title>%s</title>\
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
           htmltitle, htmltitle, baseMacChr);
  snprintf(submithtml, 1000, "<!DOCTYPE html><html><head><title>%s</title>\
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body>\
<h2>%s</h2>\
<p>MACaddress<br>%s</p>\
<p>Attempts to connect to WiFi after 10 seconds.</p>\
<p>When led blinks at 1 second intervals, %s is trying to connect to WiFi.<br>\
If it continues for a long time, press and hold the reset button for more than %d seconds and setting WiFi again.</p>\
</body></html>",
           htmltitle, htmltitle, baseMacChr, boardname, presstime / 100);
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
    led = false;
    while (!digitalRead(switchpin)) {
      delay(10);
      count++;
      if (count > presstime) {
        led = true;
        digitalWrite(ledpin, HIGH);
        CPSerial.println("Erase wifi_config");
        LittleFS.begin();
        LittleFS.remove(wifi_config);
        LittleFS.end();
        while (!digitalRead(switchpin)) {
          delay(10);
        }
        return true;
      }
    }
  }
  return false;
}