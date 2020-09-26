
## Arduino Settings:
    Board:Wemos D1 R2 & mini

## upload /data folder to ESP8266
`pio run -t buildfs && pio run -t uploadfs`

## History
    Update: 13.04.2020  // bootvorgang anzeigen
    Update: 25.07.2020  // Sting ersetzen durch https://gitlab.com/arduino-libraries/stackstring/-/blob/master/src/StackString.hpp
    Update: 04.08.2020	// komplette überarbeitung u.a. utf8ascii, ota, mqtt-topics, ...
    Update: 12.09.2020	// neues WebUI inkl. config, settings, ...

## Mosquitto
    mosquitto_pub -q 2 -h 192.168.2.222 -u dirk -P ***** -t "smarthome/infodisplay/message" -m "test"

## Required libs
    Max72xxPanel
        --> https://github.com/markruys/arduino-Max72xxPanel
    PubSubClient --> https://pubsubclient.knolleary.net/api
        --> https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
    Font Customizer:
        --> https://tchapi.github.io/Adafruit-GFX-Font-Customiser/

## Hints / ideas taken from ...
*   http://esp32-server.de/ntp/
*   https://tttapa.github.io/ESP8266/Chap15%20-%20NTP.html
*   https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/NTPClient/NTPClient.ino
*   https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/NTP-TZ-DST/NTP-TZ-DST.ino
*   https://fipsok.de/Esp32-Webserver/ntp-zeit-Esp32.tab
*   http://esp32-server.de/ntp/
*   https://www.electronics-lab.com/project/network-clock-using-esp8266-oled-display/
*   http://forum.arduino.cc/index.php?topic=143903.msg1080948#msg1080948
*   https://github.com/Qrome/marquee-scroller
*   https://www.mikrocontroller.net/attachment/highlight/389184
*   https://forum.mysensors.org/topic/1976/scrolling-text-sensor-node-with-new-v_text/2
*   https://playground.arduino.cc/Main/Utf8ascii/         funktioniert nicht wirklich
*   https://forum.arduino.cc/index.php?topic=171056.975   andere Library
*   https://blog.thesen.eu/   lokale-uhrzeit-mit-dem-esp8266-und-einem-ntp-zeitserver-inklusive-sommerwinterzeit/
 *  https://tttapa.github.io/ESP8266/Chap09%20-%20Web%20Server.html
 * 	https://ullisroboterseite.de/esp8266-webserver-klasse.html
 * 	https://git.hacknology.de/wolfgang/CucooLixe/src/branch/master/src/main.cpp
 * 	https://esp32.com/viewtopic.php?t=9783
 * 	https://forum.arduino.cc/index.php?topic=484418.0
 *  https://github.com/lorol/ESPAsyncWebServer/blob/master/examples/SmartSwitch/SmartSwitch.ino

## Anschliessen der DOT Matix
* DOT Matrix:       ESP8266 NodeMCU:
* VCC               5V (VUSB)
* GND               GND
* DIN               D7 (GPIO13)
* CS                D3 (GPIO0)
* CLK               D5 (GPIO14)
* DIM LDR           A0

## homebridge configuration
```	
    {
		"accessory": "HTTP-LIGHTBULB",
		"name": "display",
		"onUrl": "http://192.168.2.102/on",
		"offUrl": "http://192.168.2.102/off",
		"statusUrl": "http://192.168.2.102/state",
		"brightness": {
			"setUrl": "http://192.168.2.102/brightness?value=%s",
			"statusUrl": "http://192.168.2.102/brightness"
		}
	}
```    
