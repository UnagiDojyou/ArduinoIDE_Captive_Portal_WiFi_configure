#include <Arduino.h>
#include <LittleFS.h>
#if defined  ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
#else
  #include <WiFi.h>
  #include <WebServer.h>
#endif
#if defined(ESP32)
  #include <esp_mac.h>
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

//read one line (may be empty) from an already-opened file, stripping trailing \r
static String readFileLine(File& file) {
  String s = "";
  if (file.available()) {
    s = file.readStringUntil('\n');
  }
  s.replace("\r", "");
  return s;
}

//read one line from an already-opened file and parse it into an IPAddress (0.0.0.0 when empty or invalid)
static IPAddress readFileLineIP(File& file) {
  IPAddress ip(0, 0, 0, 0);
  ip.fromString(readFileLine(file).c_str());
  return ip;
}

//escape a string for safe use inside an HTML attribute
static void htmlEscape(const String& in, char* out, int outSize) {
  int n = 0;
  out[0] = '\0';
  for (int i = 0; i < in.length() && n + 6 < outSize; i++) {
    char c = in[i];
    if (c == '&') {
      strcpy(out + n, "&amp;"); n += 5;
    } else if (c == '<') {
      strcpy(out + n, "&lt;"); n += 4;
    } else if (c == '>') {
      strcpy(out + n, "&gt;"); n += 4;
    } else if (c == '"') {
      strcpy(out + n, "&quot;"); n += 6;
    } else {
      out[n++] = c;
    }
    out[n] = '\0';
  }
}

//true when both addresses are inside the subnet defined by mask
static bool sameSubnet(const IPAddress& a, const IPAddress& b, const IPAddress& mask) {
  for (int i = 0; i < 4; i++) {
    if ((a[i] & mask[i]) != (b[i] & mask[i])) {
      return false;
    }
  }
  return true;
}

//true when ip is the broadcast address of the subnet defined by mask
static bool isBroadcast(const IPAddress& ip, const IPAddress& mask) {
  for (int i = 0; i < 4; i++) {
    uint8_t hostbits = ~mask[i];
    if ((ip[i] & hostbits) != hostbits) {
      return false;
    }
  }
  return true;
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
    File file = LittleFS.open(wifi_config, "r");
    if (file) {
      staSSID = readFileLine(file);      //SSID
      staPassword = readFileLine(file);  //password
      staHostname = readFileLine(file);  //hostname (may be empty)
      staIPMode = readFileLine(file);    //"dhcp" or "static"
      staIP = readFileLineIP(file);      //IP address (0.0.0.0 when dhcp)
      staSubnet = readFileLineIP(file);  //subnet mask (0.0.0.0 when dhcp)
      staGateway = readFileLineIP(file); //default gateway (0.0.0.0 when dhcp)
      staDNS = readFileLineIP(file);     //DNS server (0.0.0.0 when not set)
      file.close();
    }
    if (staIPMode != "static") {
      staIPMode = "dhcp";
      staIP = IPAddress(0, 0, 0, 0);
      staSubnet = IPAddress(0, 0, 0, 0);
      staGateway = IPAddress(0, 0, 0, 0);
    }
    if (staIPMode == "static" && staDNS == IPAddress(0, 0, 0, 0)) {
      staDNS = IPAddress(8, 8, 8, 8); //default DNS when static IP is set without DNS
    }
    LittleFS.end();
    return true;
  } else {
    softAP = true;
    Serialprintln("[CPWiFiConfigure] start AP");
    // Get MAC address for WiFi station.
    // On ESP32 the WiFi driver is not started yet, so read the factory MAC straight
    // from eFuse (WiFi.macAddress() would return all zeros at this point).
    uint8_t baseMac[6] = { 0 };
#if defined(ESP32)
    esp_efuse_mac_get_default(baseMac);
#else
    WiFi.macAddress(baseMac);
#endif
    sprintf(baseMacChar, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    //softSSID member: boardName(24) + "-XXXXXX" + NUL
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
      this->createHTML(); //re-render the form with the last submitted values (pre-fill after an error)
      this->server.send(200, "text/html", this->rootHTLM);
    });
    server.on("/submit", HTTP_POST, [this]() {
      if (server.hasArg("SSID") && server.hasArg("Password")) {
        String staSSID = this->server.arg("SSID");
        String staPassword = this->server.arg("Password");
        String staHostname = this->server.hasArg("Hostname") ? this->server.arg("Hostname") : "";
        String staIPMode = this->server.hasArg("IPMode") ? this->server.arg("IPMode") : "dhcp";
        String staIP = this->server.hasArg("IP") ? this->server.arg("IP") : "";
        String staSubnet = this->server.hasArg("Subnet") ? this->server.arg("Subnet") : "";
        String staGateway = this->server.hasArg("Gateway") ? this->server.arg("Gateway") : "";
        String staDNS = this->server.hasArg("DNS") ? this->server.arg("DNS") : "";
        if (staIPMode != "static") {
          staIPMode = "dhcp";
          staIP = "";
          staSubnet = "";
          staGateway = "";
        }
        if (staIPMode == "static" && staDNS.length() == 0) {
          staDNS = "8.8.8.8"; //default DNS when static IP is set without DNS
        }
        // validate the submitted values and collect the problems
        const char* errors[8] = { 0 };
        int nerrors = 0;
        auto appendError = [&](const char* msg) {
          if (nerrors < 8) {
            errors[nerrors++] = msg;
          }
        };
        if (staSSID.length() > 32) {
          appendError("SSID is too long (max 32 characters).");
        }
        if (staPassword.length() > 63) {
          appendError("Password is too long (max 63 characters).");
        }
        if (staHostname.length() > 20) {
          appendError("Hostname is too long (max 20 characters).");
        }
        IPAddress ip, subnet, gateway;
        if (staIPMode == "static") {
          if (!ip.fromString(staIP.c_str()) || ip == IPAddress(0, 0, 0, 0)) {
            appendError("Invalid IP address.");
          }
          if (!subnet.fromString(staSubnet.c_str()) || subnet == IPAddress(0, 0, 0, 0)) {
            appendError("Invalid subnet mask.");
          }
          if (!gateway.fromString(staGateway.c_str()) || gateway == IPAddress(0, 0, 0, 0)) {
            appendError("Invalid default gateway.");
          }
          if (ip != IPAddress(0, 0, 0, 0) && subnet != IPAddress(0, 0, 0, 0) && gateway != IPAddress(0, 0, 0, 0)) {
            if (!sameSubnet(ip, gateway, subnet)) {
              appendError("Default gateway is not in the same subnet as the IP address.");
            }
            if (isBroadcast(ip, subnet)) {
              appendError("IP address is the broadcast address of the subnet.");
            }
          }
        }
        if (staDNS.length() > 0) {
          IPAddress dns;
          if (!dns.fromString(staDNS.c_str()) || dns == IPAddress(0, 0, 0, 0)) {
            appendError("Invalid DNS server address.");
          }
        }
        if (nerrors > 0) {
          // keep the submitted values so the form is re-shown pre-filled
          this->staSSID = staSSID;
          this->staHostname = staHostname;
          this->staIPMode = staIPMode;
          this->formIP = staIP;
          this->formSubnet = staSubnet;
          this->formGateway = staGateway;
          this->formDNS = staDNS;
          hasSubmitted = true; //the pre-fill now follows the submitted values, not the defaults
          // join the error messages; they are shown in red under the MAC address
          int elen = 0;
          for (int i = 0; i < nerrors; i++) {
            if (i > 0) {
              elen += snprintf(formErrors + elen, 512 - elen, "<br>");
            }
            elen += snprintf(formErrors + elen, 512 - elen, "%s", errors[i]);
          }
          createHTML(); //re-render the form, errors shown in red under the MAC address
          this->server.send(200, "text/html", this->rootHTLM);
        } else {
          formErrors[0] = '\0';
          this->server.send(200, "text/html", this->submitHTML);
          LittleFS.begin();
          File file = LittleFS.open(wifi_config, "w");
          file.println(staSSID);      //SSID
          file.println(staPassword);  //password
          file.println(staHostname);  //hostname (may be empty)
          file.println(staIPMode);    //"dhcp" or "static"
          file.println(staIP);        //IP address (empty when dhcp)
          file.println(staSubnet);    //subnet mask (empty when dhcp)
          file.println(staGateway);   //default gateway (empty when dhcp)
          file.println(staDNS);       //DNS server (may be empty)
          file.close();
          LittleFS.end();
          this->staSSID = staSSID;
          this->staPassword = staPassword;
          this->staHostname = staHostname;
          this->staIPMode = staIPMode;
          if (staIPMode == "static") {
            this->staIP.fromString(staIP.c_str());
            this->staSubnet.fromString(staSubnet.c_str());
            this->staGateway.fromString(staGateway.c_str());
          } else {
            this->staIP = IPAddress(0, 0, 0, 0);
            this->staSubnet = IPAddress(0, 0, 0, 0);
            this->staGateway = IPAddress(0, 0, 0, 0);
          }
          if (staDNS.length() > 0) {
            this->staDNS.fromString(staDNS.c_str());
          } else {
            this->staDNS = IPAddress(0, 0, 0, 0);
          }
          this->formIP = staIP;
          this->formSubnet = staSubnet;
          this->formGateway = staGateway;
          this->formDNS = staDNS;
          this->softAP = false;
        }
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
  //pre-fill the form with the last submitted values
  //on first boot (no submission yet) the hostname defaults to the AP SSID (the DNS field stays empty)
  String hostVal = staHostname;
  if (!hasSubmitted && hostVal.length() == 0) {
    char hostDef[21]; //the hostname field is limited to 20 characters
    snprintf(hostDef, 21, "%s", softSSID);
    hostVal = hostDef;
  }
  char vSSID[80], vHost[48], vDNS[48], vIP[48], vSub[48], vGW[48];
  htmlEscape(staSSID, vSSID, sizeof(vSSID));
  htmlEscape(hostVal, vHost, sizeof(vHost));
  htmlEscape(formDNS, vDNS, sizeof(vDNS));
  htmlEscape(formIP, vIP, sizeof(vIP));
  htmlEscape(formSubnet, vSub, sizeof(vSub));
  htmlEscape(formGateway, vGW, sizeof(vGW));
  const char* dhcpChecked = (staIPMode != "static") ? " checked" : "";
  const char* staticChecked = (staIPMode == "static") ? " checked" : "";
  char errorLine[600];
  if (formErrors[0] != '\0') {
    snprintf(errorLine, 600, "<p style=\"color:red\">%s</p>", formErrors);
  } else {
    errorLine[0] = '\0';
  }
  snprintf(rootHTLM, 2048, "<!DOCTYPE html><html><head><title>%s</title>\
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body>\
<h2>%s</h2>\
<p>MACaddress<br>%s</p>\
%s\
<form action=\"/submit\" method=\"POST\">\
SSID<br>\
<input type=\"text\" name=\"SSID\" value=\"%s\" required><br>\
Password<br>\
<input type=\"text\" name=\"Password\" required><br>\
Hostname<br>\
<input type=\"text\" name=\"Hostname\" value=\"%s\" maxlength=\"20\"><br>\
IP mode<br>\
<input type=\"radio\" name=\"IPMode\" value=\"dhcp\"%s> DHCP/DNS only (auto)<br>\
<input type=\"radio\" name=\"IPMode\" value=\"static\"%s> Manual (static IP)<br>\
DNS<br>\
<input type=\"text\" name=\"DNS\" value=\"%s\"><br>\
IP address<br>\
<input type=\"text\" name=\"IP\" value=\"%s\"><br>\
Subnet mask<br>\
<input type=\"text\" name=\"Subnet\" value=\"%s\"><br>\
Default gateway<br>\
<input type=\"text\" name=\"Gateway\" value=\"%s\"><br>\
<input type=\"submit\" value=\"Connect\">\
</form></body></html>",
           htmlTitle, htmlTitle, baseMacChar, errorLine, vSSID, vHost, dhcpChecked, staticChecked, vDNS, vIP, vSub, vGW);
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
  return staSSID;
}

String CPWiFiConfigure::readPASS() {
  return staPassword;
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