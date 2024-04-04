#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <esp_system.h>
#include <FS.h>
#include <SPIFFS.h>

#define BOOT_SW 0

const char* wifi_config = "/WiFi.txt";
bool softap = true;
bool led = false;
int count = 0;

char baseMacChr[18] = {0};

WebServer server(80);
WiFiClient client;
DNSServer dnsServer;

void handleRoot() {
  char htmlForm[500];
  snprintf(htmlForm,500,"<!DOCTYPE html>\
<html>\
<head>\
<title>ESP32 WiFi Setting</title>\
<meta name=\"viewport\" content=\"width=300\">\
</head>\
<body>\
<h2>ESP32 WiFi Setting</h2>\
<p>MACaddress<br>%s</p>\
<form action=\"/submit\" method=\"POST\">\
SSID<br>\
<input type=\"text\" name=\"SSID\" required\">\<br>\
Password<br>\
<input type=\"text\" name=\"Password\" required\">\<br>\
<input type=\"submit\" value=\"send\">\
</form>\
</body>\
</html>",baseMacChr);
  server.send(200, "text/html", htmlForm);
}

void handleSubmit() {
  if (server.hasArg("SSID") && server.hasArg("Password")) {
    String staSSID = server.arg("SSID");
    String staPassword = server.arg("Password");
    Serial.println(staSSID);
    Serial.println(staPassword);

    char htmlForm[500];
    snprintf(htmlForm,500,"<!DOCTYPE html>\
<html>\
<head>\
<title>ESP32 WiFi Setting</title>\
<meta name=\"viewport\" content=\"width=300\">\
</head>\
<body>\
<h2>ESP32 WiFi Setting</h2>\
<p>MACaddress<br>%s</p>\
<p>Attempts to connect to %s after 10 seconds.</p>\
<p>When led blinks at 1 second intervals, ESP32 is trying to connect to WiFi.<br>\
If it continues for a long time, press and hold the reset button for more than 5 seconds and setting WiFi again.</p>\
</body>\
</html>",baseMacChr,staSSID.c_str());
    server.send(200, "text/html", htmlForm);

    File file = SPIFFS.open(wifi_config, FILE_WRITE);
    file.println(staSSID); //SSID
    file.println(staPassword); //password
    file.close();
    delay(10000); //10seconds
    ESP.restart();

  } else {
    server.send(200, "text/plain", "Message not received");
  }
}

void handleNotFound() {
  String IPaddr = ipToString(server.client().localIP());
  server.sendHeader("Location", String("http://") + IPaddr, true);
  server.send(302, "text/plain", "");
  server.client().stop();
}

// https://qiita.com/dojyorin/items/ac56a1c2c620782d90a6
String ipToString(uint32_t ip) {
  String result = "";
  result += String((ip & 0xFF), 10);
  result += ".";
  result += String((ip & 0xFF00) >> 8, 10);
  result += ".";
  result += String((ip & 0xFF0000) >> 16, 10);
  result += ".";
  result += String((ip & 0xFF000000) >> 24, 10);
  return result;
}

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_SW,INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS failed, or not present");
    return;
  }
  if(SPIFFS.exists(wifi_config)){
    //Setup staAP mode
    softap = false;
    Serial.println("try connect to WiFi");
    File file = SPIFFS.open(wifi_config, FILE_READ);
    String staSSID = file.readStringUntil('\n');
    String staPassword = "";
    if (file.available()){
      staPassword = file.readStringUntil('\n');
    }
    file.close();
    staSSID.replace("\r", "");
    staPassword.replace("\r", "");
    Serial.println(staSSID);
    Serial.println(staPassword);
    WiFi.begin(staSSID, staPassword);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000); //1sencods
      readbutton();
      Serial.print(".");
      if(!led){
        digitalWrite(LED_BUILTIN, HIGH);
        led = true;
      }
      else{
        digitalWrite(LED_BUILTIN, LOW);
        led = false;
      }
    }
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.print("IP is ");
    Serial.println(WiFi.localIP());
    //write what you want to do using WiFi
  }else{
    //Setup SoftAP mode
    softap = true;
    Serial.println("start AP");
    // Get MAC address for WiFi station
    uint8_t baseMac[6];
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    char softSSID[10] = {0};
    sprintf(softSSID, "ESP32-%02X%02X%02X", baseMac[3], baseMac[4], baseMac[5]);

    Serial.print("MAC: ");
    Serial.println(baseMacChr);
    Serial.print("SSID: ");
    Serial.println(softSSID);

    WiFi.softAP(softSSID);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    server.on("/", HTTP_GET, handleRoot);
    server.on("/submit", HTTP_POST, handleSubmit);
    server.onNotFound(handleNotFound);

    dnsServer.start(53, "*", IP);
    server.begin();
  }
}

void readbutton(){
  if(!digitalRead(BOOT_SW)){
    count = 0;
    while(!digitalRead(BOOT_SW)){
      delay(10);
      count++;
      if(count > 500){ //5seconds
        Serial.println("Erase wifi_config and reboot");
        SPIFFS.remove(wifi_config);
        ESP.restart();
        break;
      }
    }
  }
  return;
}

void loop() {
  if(softap){
    server.handleClient();
    dnsServer.processNextRequest();
    count++;
    if(count > 300){//about 0.3seconds
      if(!led){
        digitalWrite(LED_BUILTIN, HIGH);
        led = true;
      }
      else{
        digitalWrite(LED_BUILTIN, LOW);
        led = false;
      }
      count = 0;
    }
  }else{
    readbutton();
    //write what you want to do using WiFi
  }
}
