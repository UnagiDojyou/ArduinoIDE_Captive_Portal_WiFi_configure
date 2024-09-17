// Before running:
// Select Tools->Flash Size->(some size with a FS/filesystem)

#include <WiFi.h>
#include <Captive_Portal_WiFi_connector.h>

#define Button 1

CPWiFiConfigure CPWiFi(Button, LED_BUILTIN, Serial);

void setup() {
  Serial.begin(115200);
  delay(1000);
  sprintf(CPWiFi.boardName,"PicoW");
  sprintf(CPWiFi.htmlTitle, "Capitive_Portal_WiFi_configure sample code on Raspberry Pi Pico W");
  if(!CPWiFi.begin()){
    Serial.println("Fail to start Capitive_Portal_WiFi_configure");
    while (true) {}
  }
  WiFi.begin(CPWiFi.readSSID().c_str(), CPWiFi.readPASS().c_str());
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
