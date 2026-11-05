// Before running:
// Select Tools->Flash Size->(some size with a FS/filesystem)

#include <WiFi.h>
#include <Captive_Portal_WiFi_connector.h>
#include <LittleFS.h>

#define BOOT_SW 12 //Pico W SW0 (BOOT) button is GPIO12

CPWiFiConfigure CPWiFi(BOOT_SW, LED_BUILTIN, Serial);

void setup() {
  Serial.begin(115200);
  delay(1000);
  sprintf(CPWiFi.boardName, "PicoW");
  sprintf(CPWiFi.htmlTitle, "Capitive_Portal_WiFi_configure sample code on Raspberry Pi Pico W");
  if (!LittleFS.begin()) {
    Serial.println("Fail to start LittleFS");
  }
  if (!CPWiFi.begin()) {
    Serial.println("Fail to start Capitive_Portal_WiFi_configure");
    while (true) {}
  }
  Serial.printf("settings: mode=%s hostname=%s ip=%s/%s gw=%s dns=%s\n",
                CPWiFi.staIPMode.c_str(), CPWiFi.staHostname.c_str(),
                CPWiFi.staIP.toString().c_str(), CPWiFi.staSubnet.toString().c_str(),
                CPWiFi.staGateway.toString().c_str(), CPWiFi.staDNS.toString().c_str());
  if (CPWiFi.staHostname.length() > 0) {
    WiFi.setHostname(CPWiFi.staHostname.c_str());
  }
  if (CPWiFi.staIPMode == "static") {
    //the library defaults staDNS to 8.8.8.8 when static IP is set without a DNS
    WiFi.config(CPWiFi.staIP, CPWiFi.staDNS, CPWiFi.staGateway, CPWiFi.staSubnet);
  }
  WiFi.begin(CPWiFi.staSSID.c_str(), CPWiFi.staPassword.c_str());
  int count = 0;
  bool led = false;
  while (WiFi.status() != WL_CONNECTED) {
    if (count > 40) {
      Serial.println("WiFi connect Fail. reboot.");
      rp2040.reboot();
    }
    delay(1000);  //1sencods
    if (CPWiFi.readButton()) {
      rp2040.reboot();
    }
    Serial.print(".");
    if (!led) {
      digitalWrite(LED_BUILTIN, HIGH);
      led = true;
    } else {
      digitalWrite(LED_BUILTIN, LOW);
      led = false;
    }
    count++;
  }
  if (CPWiFi.staIPMode != "static" && CPWiFi.staDNS != IPAddress(0, 0, 0, 0)) {
    WiFi.setDNS(CPWiFi.staDNS); //override DNS in DHCP mode
  }
  digitalWrite(LED_BUILTIN, LOW);
  led = false;
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP is ");
  Serial.println(WiFi.localIP());
  // write what you want to do using WiFi
}

void loop() {
  if (CPWiFi.readButton()) {
    rp2040.reboot();
  }
  // write what you want to do using WiFi
}
