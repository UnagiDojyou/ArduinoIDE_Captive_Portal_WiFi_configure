#include <WiFi.h>
#include <esp_system.h>
#include <Captive_Portal_WiFi_connector.h>
#include <LittleFS.h>

#ifdef CONFIG_IDF_TARGET_ESP32C3
  #define BOOT_SW 9 //BOOT PIN of ESP32C3 is GPIO9
#elif defined CONFIG_IDF_TARGET_ESP32C6
  #define BOOT_SW 9 //BOOT PIN of ESP32C6 is GPIO9
#else
  #define BOOT_SW 0
#endif

CPWiFiConfigure CPWiFi(BOOT_SW, LED_BUILTIN, Serial);

void setup() {
  Serial.begin(115200);
  sprintf(CPWiFi.boardName, "ESP32");
  sprintf(CPWiFi.htmlTitle, "Capitive_Portal_WiFi_configure sample code on ESP32");
  if (!LittleFS.begin(true)) {
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
    WiFi.config(CPWiFi.staIP, CPWiFi.staGateway, CPWiFi.staSubnet, CPWiFi.staDNS);
  }
  WiFi.begin(CPWiFi.staSSID.c_str(), CPWiFi.staPassword.c_str());

  int count = 0;
  bool led = false;
  while (WiFi.status() != WL_CONNECTED) {
    if (count > 40) {
      Serial.println("WiFi connect Fail. reboot.");
      ESP.restart();
    }
    delay(1000);  //1sencods
    if (CPWiFi.readButton()) {
      ESP.restart();
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
    ESP.restart();
  }
  // write what you want to do using WiFi
}
