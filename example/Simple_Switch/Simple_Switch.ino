//Example of using Captive_Portal_WiFi_connector
//https://github.com/UnagiDojyou/ArduinoIDE_Captive_Portal_WiFi_configure
//What can this code do?
//Could control LED_BUILTIN via http or BOOT_SW
//If you want to trun on LED_BUILTIN, access to http://IPADDRESS/on
//Off, access to http://IPADDRESS/on
//want to kown now status, access to http://IPADDRESS/status

#include <Captive_Portal_WiFi_connector.h>
#include <LittleFS.h>
#if defined ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #define LED_BUILTIN_ON LOW  //GPIO2 of ESP12 or ESP8266mod is pulluped
  #define LED_BUILTIN_OFF HIGH
  ESP8266WebServer server(80);
#else
  #include <WiFi.h>
  #include <WebServer.h>
  #define LED_BUILTIN_ON HIGH
  #define LED_BUILTIN_OFF LOW
  WebServer server(80);
#endif

#define BOOT_SW 0

CPWiFiConfigure CPWiFi(BOOT_SW, LED_BUILTIN, Serial);

void restart(){
#if defined PICO_RP2040
  rp2040.reboot();
#else
  ESP.restart();
#endif
}

bool led = false;

void handleOn() {
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
  led = true;
  server.send(200, "text/plain", "1\r\n");
  Serial.println("on");
}

void handleOff() {
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
  led = false;
  server.send(200, "text/plain", "0\r\n");
  Serial.println("off");
}

void handleStatus() {
  if (led) {
    server.send(200, "text/plain", "1\r\n");
  } else {
    server.send(200, "text/plain", "0\r\n");
  }
  Serial.println("status");
}

void handleNotFound() {
  server.send(404, "text/plain", "This is HTTP_switch. To turn on is access to /on, off is /off, To get staus is /status.\r\n");
  Serial.println("404");
}

void setup() {
  Serial.begin(115200);
  sprintf(CPWiFi.boardName, "CPWiFiSampleSwitch");
#if defined ESP32
  if (!LittleFS.begin(true)) {
    Serial.println("Fail to start LittleFS");
  }
#endif
  if (!CPWiFi.begin()) {
    Serial.println("Fail to start Capitive_Portal_WiFi_configure");
    return;
  }
  WiFi.begin(CPWiFi.readSSID().c_str(), CPWiFi.readPASS().c_str());
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (count > 20) {
      Serial.println("WiFi connect Fail. reboot.");
      restart();
    }
    delay(1000);  //1sencods
    if (CPWiFi.readButton()) {
      restart();
    }
    Serial.print(".");
    if (!led) {
      digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
      led = true;
    } else {
      digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
      led = false;
    }
    count++;
  }
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
  led = false;
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP is ");
  Serial.println(WiFi.localIP());
  // write what you want to do using WiFi
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

bool Button_flag = true;
bool oldButton_flag = true;

void loop() {
  server.handleClient();
  if (led) {
    digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
  } else {
    digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
  }
  oldButton_flag = Button_flag;
  Button_flag = digitalRead(BOOT_SW);
  if (!Button_flag && oldButton_flag) {
    if (CPWiFi.readButton()) {
      restart();
    }
    led = !led;
  }
}
