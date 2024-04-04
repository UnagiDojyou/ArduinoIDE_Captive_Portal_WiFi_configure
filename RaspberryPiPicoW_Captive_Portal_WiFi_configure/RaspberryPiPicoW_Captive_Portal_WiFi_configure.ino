#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <FS.h>
#include <LittleFS.h>

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
</html>",baseMacChr,staSSID.c_str());
    server.send(200, "text/html", htmlForm);

    File file = LittleFS.open(wifi_config, "w");
    file.println(staSSID); //SSID
    file.println(staPassword); //password
    file.close();
    delay(10000); //10seconds
    rp2040.reboot();

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
  delay(5000);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  if (!LittleFS.begin()) {
    Serial.println("SPIFFS failed, or not present");
    return;
  }
  if(LittleFS.exists(wifi_config)){
    //Setup staAP mode
    softap = false;
    Serial.println("try connect to WiFi");
    File file = LittleFS.open(wifi_config, "r");
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
    WiFi.begin(staSSID.c_str(), staPassword.c_str());
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

  }else{
    //Setup SoftAP mode
    softap = true;
    Serial.println("start AP");
    // Get MAC address for WiFi station
    uint8_t baseMac[6];
    WiFi.macAddress(baseMac);
    sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    char softSSID[10] = {0};
    sprintf(softSSID, "RaspberryPiPiciW-%02X%02X%02X", baseMac[3], baseMac[4], baseMac[5]);

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
  if(BOOTSEL){
    count = 0;
    while(BOOTSEL){
      delay(10);
      count++;
      if(count > 500){ //5seconds
        Serial.println("Erase wifi_config and reboot");
        LittleFS.remove(wifi_config);
        rp2040.reboot();
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
    delay(1);
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
