#include <ESP8266WiFi.h>
#include <Captive_Portal_WiFi_connector.h>

#define BOOT_SW 0

CPWiFiConfigure CPWiFi(BOOT_SW, LED_BUILTIN, Serial);

void setup() {
  Serial.begin(115200);
  sprintf(CPWiFi.boardName, "ESP8266");
  if (!CPWiFi.begin()) {
    Serial.println("Fail to start Capitive_Portal_WiFi_configure");
    return;
  }
  WiFi.begin(CPWiFi.readSSID().c_str(), CPWiFi.readPASS().c_str());
  int count = 0;
  bool led = false;
  while (WiFi.status() != WL_CONNECTED) {
    if (count > 20) {
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
  digitalWrite(LED_BUILTIN, HIGH); //GPIO2 of ESP12 or ESP8266mod is pulluped
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
  digitalWrite(LED_BUILTIN, HIGH); //GPIO2 of ESP12 or ESP8266mod is pulluped
  // write what you want to do using WiFi
}
