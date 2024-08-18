# ArduinoIDE_Captive_Portal_WiFi_configure
Start it as AP mode and use the Captive Portal to direct you to a page that asks you to enter your SSID and password. Once entered, it will automatically connect to the WiFi entered in STA mode.

## How to install
* Download Zip and import this library to ArduinoIDE.
* Enable littleFS.

## How to use
### Add code
See example code.
You can add code under the two places (setup() and loop()) where "write what you want to do using WiFi" is written in the comment.
### How to set up WiFi.
1. Open WiFi settings on any device.
2. Look for the SSID boardName-XXXXXX.<br>
If it cannot be found, there is a high possibility that the FS settings are not configured correctly. Please reset or review the Arduino IDE settings.
3. Connect to boardName-XXXXXX.
4. Captive Portal will display the WiFi input fields for SSID and password.
5. Remember your MAC address. It will be used later to identify the IP address.
6. Enter the SSID and password of the WiFi to which you want to connect board.
7. Press "send" button.
8. wait 30 second.
9. Check LED status. If the slow blinking continues, please do "Reset WiFi Setting" and review your WiFi settings.

### Reset WiFi Setting
1. Press and hold the BOOT_SW for more than 5 seconds.<br>
2. Release the BOOT_SW when the LED lights up.

## LED status
### Blink quickly
WiFi settings have not been completed.
### Blink slowly
Try to connect to WiFi.<br>
If the slow blinking continues, please do "Reset WiFi Setting" and review your WiFi settings.
