/*
 *
 */
const char* _ssid 					= "netzbox";
const char* _password 				= "*******";

#include "config.h"
#include "common.h"
#include "uptime.h"

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h> 					// Bonjour
#include <SPI.h>		 					//Einbinden von SPI für Kommunikation  mit DOT Matrix
#include <Wire.h>
#include <Adafruit_GFX.h>					//Zeichensätze für DOT Matrix
#include "libs/Max72xxPanel/Max72xxPanel.h" //Ansteuerung für DOT Matrix mit MAX7219 Treiber
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WeTimer.h"
#include "WebServer.h"
#include "ringbuffer.h"
#include "twilight.h"

/*****************************************************************************/

int pinCS 							= 0;						//Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )
int numberOfHorizontalDisplays 		= 4; //Anzahl der Module Horizontal
int numberOfVerticalDisplays 		= 1;	//Anzahl der Module Vertikal
int outputpin 						= A0;
int LDRvalue 						= 0;	// value @ PIN A0	
/* set ledRotation for LED Display panels (3 is default)
0: no rotation
1: 90 degrees clockwise
2: 180 degrees
3: 90 degrees counter clockwise (default)
*/
int ledRotation 					= 3;

Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

const unsigned long SECOND 			= 1000UL;
const unsigned long MINUTE 			= SECOND * 60;
const unsigned long HOUR 			= MINUTE * 60;
int secCounter 						= 0;

#include <ArduinoOTA.h>
boolean ENABLE_OTA = true; // this will allow you to load firmware to the device over WiFi (see OTA for ESP8266)
String OTA_Password = "";  // Set an OTA password here -- leave blank if you don't want to be prompted for password

/*** font settings ***/
int baseline 						= 6; // depends on the FONT --> 0 for default font, 6 or 7 for bitmap fonts
// --> https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#define FONT fivebyseven5pt8b
#include "fivebyseven5.h"

WiFiClient espClient;
PubSubClient client(espClient);

/*** timer and settings ***/
WeTimer timer5Min 					= WeTimer(MINUTE * 5);
WeTimer timer1h 					= WeTimer(MINUTE * 60);
long pulseCount24hour 				= 0;
unsigned long cpuLastMicros 		= 0; // cpu utilisation
unsigned long avgCpuDelta 			= 0;	 // cpu utilisation

/*** mqtt setting s***/
long lastReconnectAttempt 			= 0;
bool bNewMqttMessage 				= false; // new message set by mqtt
bool bAlarm 						= false;

/*** display settings ***/
int scrollwait 						= 50;	//Zeit in ms für Scroll Geschwindigkeit wo gewartet wird
int spacer 							= 1;			//leer Zeichen länge
int width 							= 5 + spacer; //Schriftlänge ist 5 Pixel

/*** sunrise / sunset ***/
boolean twilightInit 				= true; // set true initially and only once !
static unsigned long lastOpenWeatherCheck = 2 * HOUR;		// initial setup to gather data right after booting
char sunsetTime[6] 					= "00:00";		//
char sunriseTime[6] 				= "00:00";		//
int dlsav 							= 0; 			// daylightSavings should be 1 
													// if it is in effect during the summer 
													// otherwise it should be 0

/*****************************************************************************/
/*
 *	
 */
void readConfigFromJson();
OpenWeather local_weather;
void calculateSunriseSunset();
void getWeather(char *openWeatherString);
int getLDRvalue() {return LDRvalue; }
static Config cfg;
Config getConfig() 
{ 
	if (sizeof(cfg) == 0)
	{
		dbgprintln(ico_error, "size of Config (cfg) is zero");
		readConfigFromJson();
	} else
	{
		return cfg; 
	}
}

/*****************************************************************************/
/* 
 *	https://arduinojson.org/v6/example/config/
 *	https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/ConfigFile/ConfigFile.ino
 */
void readConfigFromJson()
{
	dbgprintln(ico_info, "SPIFFS begin");

	SPIFFS.begin();
	// SPIFFS.remove("/config.json");
	if (!SPIFFS.exists("/config.json")) {
		dbgprintln(ico_null, "SPIFFS: create default file");
		File defaultfile = SPIFFS.open("/config.json", "w");
		char json[128];
		if (snprintf(json, sizeof(json), "{\n\"ssid\":\"%s\",\n\"passsword\":\"%s\"\n}\n", _ssid, _password) < 0)
		{
			strcpy(json, "{}");
		}
		defaultfile.println(json);
		defaultfile.flush();
		defaultfile.close();
	}

	dbgprintln(ico_null, "SPIFFS: read file in json buffer");
	File configFile = SPIFFS.open("/config.json", "r");

	// Allocate a buffer to store contents of the file.
	size_t size = configFile.size();
	std::unique_ptr<char[]> buf(new char[size]);

	// We don't use String here because ArduinoJson library requires the input
	// buffer to be mutable. If you don't use ArduinoJson, you may as well
	// use configFile.readString instead.
	configFile.readBytes(buf.get(), size);

	StaticJsonDocument<512> doc;
	DeserializationError error = deserializeJson(doc, buf.get());
	configFile.close();
	if (error)
	{
		dbgprintf(ico_error, "Failed to parse config file: %s", error.c_str());

#ifdef MY_DEBUG
		Dir dir = SPIFFS.openDir("/");
		while (dir.next())
		{
			dbgprintf(ico_info, "FS File: %s, size: %s", dir.fileName().c_str(), formatBytes(dir.fileSize()).c_str());
		}
#endif // MY_DEBUG
	}
	else
	{
		dbgprintln(ico_ok, "SPIFFS: reading was success");
	}

#ifdef MY_DEBUG
	// dbgprintln(ico_info, "content of configFile:");
	// dbgprintln(ico_null, buf.get());
	dbgprintln(ico_info, "serialized json: ");
	serializeJsonPretty(doc, Serial);
	dbgprintln();
#endif // MY_DEBUG

	strlcpy(cfg.hostname, doc["hostname"] | HOSTNAME, 32);
	strlcpy(cfg.ssid, doc["ssid"] | _ssid, 32);
	strlcpy(cfg.password, doc["password"] | _password, 32);
	strlcpy(cfg.mqtt_server, doc["mqtt_server"] | "", 32);
	strlcpy(cfg.mqtt_subscription, doc["mqtt_subscription"] | "", 32);
	strlcpy(cfg.mqtt_user, doc["mqtt_user"] | "", 32);
	strlcpy(cfg.mqtt_password, doc["mqtt_password"] | "", 32);
	cfg.brightness = doc["brightness"].as<int>() | 1;
	cfg.useLDR = doc["useLDR"].as<int>() | 0;
	strlcpy(cfg.api_key, doc["api_key"] | "", 64);
	// cfg.cityid			= doc["cityid"].as<int>();
	cfg.latitude = doc["latitude"].as<double>();
	cfg.longitude = doc["longitude"].as<double>();
	cfg.offset = doc["offset"].as<int>();
	cfg.twilight = doc["twilight"].as<int>();

	printConfig(cfg);

	setDisplayBrightness(cfg.brightness, (bool)cfg.useLDR, false);

	calculateSunriseSunset();
}

/* 
 *	https://arduinojson.org/v6/example/config/
 *	https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/ConfigFile/ConfigFile.ino
 */
bool saveConfigToJson(Config _cfg)
{
	cfg = _cfg;

	StaticJsonDocument<512> doc;
	doc["brightness"] = (int)_cfg.brightness;
	doc["useLDR"] = (int)_cfg.useLDR;
	doc["ssid"] = _cfg.ssid;
	doc["password"] = _cfg.password;
	doc["hostname"] = _cfg.hostname;
	doc["mqtt_server"] = _cfg.mqtt_server;
	doc["mqtt_subscription"] = _cfg.mqtt_subscription;
	doc["mqtt_user"] = _cfg.mqtt_user;
	doc["mqtt_password"] = _cfg.mqtt_password;
	// doc["cityid"]				= (int)_cfg.cityid;
	doc["api_key"] = _cfg.api_key;
	doc["latitude"] = _cfg.latitude;
	doc["longitude"] = _cfg.longitude;
	doc["offset"] = _cfg.offset;
	doc["twilight"] = _cfg.twilight;

#ifdef MY_DEBUG
	dbgprintln(ico_null, "saveConfigToJson:");
	serializeJsonPretty(doc, Serial);
	dbgprintln();
#endif // MY_DEBUG

	File configFile = SPIFFS.open("/config.json", "w");
	if (!configFile)
	{
		dbgprintln(ico_info, "Failed to open config.json for writing");
		return false;
	}

	if (serializeJsonPretty(doc, configFile) == 0)
	{ // error, 0 bytes written
		dbgprintln(ico_info, "Failed to write to file");
		configFile.close();
		return false;
	}
	else
	{
		dbgprintln(ico_null, "config file successfully written");
		configFile.close();
	}

	setDisplayBrightness(cfg.brightness, (bool)cfg.useLDR, false);

	calculateSunriseSunset();

	return true;
}

/* 
 * alternatives to delay()
 */
void delayESP8266(unsigned long ulMilliseconds)
{
	unsigned long ulPreviousMillis = millis();
	do
	{
		yield();
	} while (millis() - ulPreviousMillis <= ulMilliseconds);
}


/* 
 *
 */
void calculateSunriseSunset()
{
	struct tm tm;
	static char _timestr[6] = "00:00"; // => 5 + \0
	time_t now = time(&now);
	localtime_r(&now, &tm);

	int day = tm.tm_mday;
	int month = tm.tm_mon + 1; // month counter starts at 0 !!!
	int year = tm.tm_year;

	double hours;

	// sunrise
	float localT = twilight(year, month, day, cfg.latitude, cfg.longitude, cfg.offset, dlsav, true);
	float minute = modf(localT, &hours) * 60;
	if (snprintf(_timestr, sizeof(_timestr), "%02.0f:%02.0f", hours, minute) < 0) // Means append did not (entirely) fit
	{
		_timestr[0] = '\0';
	}
	strcpy(sunriseTime, _timestr);
	dbgprintf(ico_info, "Sunrise: %s", _timestr);

	// sunset
	localT = fmod(24 + twilight(year, month, day, cfg.latitude, cfg.longitude, cfg.offset, dlsav, false), 24.0);
	minute = modf(localT, &hours) * 60;
	if (snprintf(_timestr, sizeof(_timestr), "%02.0f:%02.0f", hours, minute) < 0) // Means append did not (entirely) fit
	{
		_timestr[0] = '\0';
	}
	strcpy(sunsetTime, _timestr);
	dbgprintf(ico_info, "Sunset: %s", _timestr);
}

/* 
 *
 */
String getSunsetTime()
{
	return String(sunsetTime);
}
String getSunriseTime()
{
	return String(sunriseTime);
}

/* 
 * https://en.wikipedia.org/wiki/Beaufort_scale
 * https://planetcalc.com/384/ --> v = 0.837*B^(3/2) m/s
 * // show weather
 * // Beaufort Bft = (v / 0,836 m/s )2/3		--> funktioniert aber nicht !!!
 * // 1 km/h = 0,53996 kn
 * // Beaufort Bft = (v+10kn)/6kn
 * // nt bScale=round(pow(x/3.01,0.66667));
 * // https://forum.mysensors.org/topic/7368/mywindsensor
 * // float bft = pow(((weatherClient.getWindRounded(0).toFloat() * 1000 / 3600) / 0.836) , (2 / 3));
 * // float bft = ((weatherClient.getWindRounded(0).toFloat() * 0.53996) + 10) / 6;
 * // int bScale = round(pow(weatherClient.getWindRounded(0).toFloat()/3.01,0.66667));
 */
int wind2Beaufort(const int kmpersec)
{
	int bft = -1;
	switch (kmpersec)
	{
	case 0 ... 1:
		bft = 0; // calm
		break;
	case 2 ... 5:
		bft = 1; // light air
		break;
	case 6 ... 11:
		bft = 2; // light breeze
		break;
	case 12 ... 19:
		bft = 3; // gentle breeze
		break;
	case 20 ... 28:
		bft = 4; // moderate breeze
		break;
	case 29 ... 38:
		bft = 5; // fresh breeze
		break;
	case 39 ... 49:
		bft = 6; // strong breeze
		break;
	case 50 ... 61:
		bft = 7; // near gale
		break;
	case 62 ... 74:
		bft = 8; // gale
		break;
	case 75 ... 88:
		bft = 9; // strong/severe gale
		break;
	case 89 ... 102:
		bft = 10; // storm
		break;
	case 103 ... 117:
		bft = 11; // violent storm
		break;
	case 118 ... 500:
		bft = 12; // hurricane force
		break;
	default:
		bft = -1;
		break;
	}
	return bft;
}

/*
 *	display state (ON / OFF)
 */
boolean displayState = ON; // on
boolean getDisplayState()
{
	return (boolean)displayState;
}
void setDisplayState(boolean state)
{
	displayState = state;
	// shutdown true ==> display false
	matrix.shutdown(!state);
}

/*
 *	display brightness (0..15)
 */
int getDisplayBrightness()
{
	return (int)cfg.brightness;
}
void setDisplayBrightness(int brightness, boolean useA0, boolean save)
{
	// set using A0 pin or not
	cfg.useLDR = useA0;
	if (brightness >= 0 && brightness < 16)
	{
		matrix.setIntensity(brightness);
		if (save == true)
		{
			if (cfg.brightness != brightness)
			{
				cfg.brightness = brightness;
				saveConfigToJson(cfg);
			}
		}
	}
}

/*
 *
 */
void staticMessage(const char *text, int iWait = 0)
{
	if (getDisplayState() == ON)
	{
		// Allocate the correct amount of memory for the payload copy
		char *textbuf = (char *)malloc(strlen(text) + 1); // +1 for the terminator
		// Copy the payload to the new buffer
		strcpy(textbuf, text);

		utf8ascii(textbuf);

		matrix.fillScreen(LOW);

		int i = 0;
		int chr = 0;
		// loop for each character
		while (i < abs(strlen(textbuf)))
		{
			// step horizontal by width of character and spacer
			matrix.drawChar(chr * width, baseline, textbuf[i], HIGH, LOW, 1);
			i++;
			chr++;
			yield();
		}
		matrix.write(); // Send bitmap to display

		// Free the memory
		free(textbuf);

		delayESP8266(SECOND * iWait);
	}
}

/*
 *
 */
void scrollMessage(const char *text)
{
	if (getDisplayState() == ON)
	{
		// show loader anmiation on website
		serverShowLoader(true);

		// Allocate the correct amount of memory for the payload copy
		// byte *p = (byte *)malloc(length);
		// memcpy(p, payload, length);

		// Allocate the correct amount of memory for the payload copy
		char *textbuf = (char *)malloc(strlen(text) + 1); // +1 for the terminator
		// Copy the payload to the new buffer
		strcpy(textbuf, text);

		utf8ascii(textbuf);

		for (int i = 0; i < width * abs(strlen(textbuf)) + matrix.width(); i++)
		{
			matrix.fillScreen(LOW);

			int letter = i / width;
			int x = (matrix.width() - 1) - i % width;
			while (x + width - spacer >= 0 && letter >= 0)
			{
				if (letter < abs(strlen(textbuf)))
				{
					matrix.drawChar(x, baseline, textbuf[letter], HIGH, LOW, 1);
				}
				letter--;
				x -= width;
				yield();
			}
			//Display beschreiben
			matrix.write();
			//Warten für Scroll Geschwindigkeit
			delayESP8266(scrollwait);
		}
		// Free the memory
		free(textbuf);

		delayESP8266(500);

		// hide loader anmiation on website
		serverShowLoader(false);
	}
}

/*
 *
 */
void callback(char *topic, byte *payload, unsigned int length)
{
	char _payload[64];
	int i = 0;

	// byte poiter to char*
	payload[length] = '\0';
	for (i = 0; i < abs(length); i++)
	{
		_payload[i] = (char)payload[i];
	}
	_payload[i] = '\0';

	// show loader anmiation on website
	serverShowLoader(true);

	// char _topic[16] = {'\0'};
	// // cut first part of topic, leave only the part after the last slash "/"
	// for(byte i = strlen(topic); i > 0; i--)
	// {
	// 	if (topic[i] == '/') {
	// 		strcpy(_topic, &topic[i+1]);
	// 		break;
	// 	}
	// }
	// dbgprintf(ico_info, "mqtt callback > topic: %s / %s", topic, _topic);


	// {"message":"das ist ein Test","level":"0","scroll":"1","repeat":"2"}
	if (strcmp(topic, MQTT_TOPIC_JSON) == 0)
	{
		// StaticJsonBuffer<200> jsonBuffer;
		// Parse JSON object
		// String szPayload = String(_payload);
		// JsonObject &doc = jsonBuffer.parseObject(szPayload);
		StaticJsonDocument<521> doc;
		DeserializationError error = deserializeJson(doc, _payload);
		if (error)
		{
			dbgprintf(ico_error, "Error reading json in mqtt callback: %s ", error.c_str());
			scrollMessage("json error\0");
		}
		else
		{
			int level = -1;
			int repeat = -1;
			int type = -1;
			const char *message = doc["message"];
			//char* message = doc["message"];
			level = int(doc["level"]);
			repeat = int(doc["repeat"]);
			type = int(doc["scroll"]); // 1 = scroll

			if (level >= 0 && level < 16)
			{
				setDisplayBrightness(level, false, true);
			}

			if (type == 1)
			{ // scroll
				if (repeat > 0 && repeat < 6)
				{
					for (int i = 0; i < repeat; i++)
					{
						scrollMessage(message);
					}
				}
			}
			else
			{
				staticMessage(message, 0);
				if (repeat > 0 && repeat < 6)
				{
					delayESP8266(SECOND * repeat);
				}
			}
			// dbgprintf(ico_info, "ringBufferAddItem: %s", (const char*)doc["message"]);
			ringBufferAddItem(doc["message"]);
		}
	}
	else
	{
		// do not show the MQTT_WILL_MESSAGE
		if (strcmp(_payload, MQTT_WILL_MESSAGE) != 0)
		{
			// dbgprintf(ico_info, "ringBufferAddItem: %s", _payload);
			ringBufferAddItem(_payload);
		}
	}

	if (strcmp(topic, MQTT_TOPIC_MESSAGE) == 0)
	{
		bNewMqttMessage = true;
		if (length > 5)
		{
			int tmns = 3; // repeat 3 times
			if (length > 32)
				tmns = 2; // only when string is short
			for (int i = 0; i < tmns; i++)
			{
				scrollMessage(_payload);
			}
		}
		else
			staticMessage(_payload, 3);
	}

	if (strcmp(topic, MQTT_TOPIC_ALARM) == 0)
	{
		bNewMqttMessage = true;
		//bAlarm = true;
		if (length > 5)
			for (int i = 0; i < 3; i++)
			{
				scrollMessage(_payload);
			}
		else
		{
			staticMessage(_payload, 3);
		}

		delayESP8266(MINUTE * 1); // ALARM wird 1 Minute angezeigt
	}

	if (strcmp(topic, MQTT_TOPIC_STATIC) == 0)
	{
		bNewMqttMessage = true;
		staticMessage(_payload, 5);
	}

	if (strcmp(topic, MQTT_TOPIC_SCROLL) == 0)
	{
		bNewMqttMessage = true;
		scrollMessage(_payload);
	}

	if (strcmp(topic, MQTT_TOPIC_NOTICE) == 0)
	{
		bNewMqttMessage = true;
		if (length > 5)
			scrollMessage(_payload);
		else
			staticMessage(_payload, 3);
	}

	if (strcmp(topic, MQTT_TOPIC_DIM) == 0)
	{
		bNewMqttMessage = true;
		int level = atoi(_payload);
		if (level >= 0 && level < 16)
		{
			setDisplayBrightness(level, false, true);
		}
	}

	if (strcmp(topic, MQTT_TOPIC_SPEED) == 0)
	{
		bNewMqttMessage = true;
		int iSpeed = atoi(_payload);
		if (iSpeed >= 0 && iSpeed < 200)
		{
			scrollwait = iSpeed;
		}
	}

	if (strcmp(topic, MQTT_TOPIC_WIFI) == 0)
	{
		bNewMqttMessage = true;
		char buf[16] = {'\0'};
		if (snprintf(buf, sizeof(buf), "wifi: %d dBm", WiFi.RSSI()) < 0) // Means append did not (entirely) fit
		{
			buf[0] = '\0';
		}
		scrollMessage(buf);
	}

	if (strcmp(topic, MQTT_TOPIC_RESET) == 0)
	{
		dbgprintln(ico_null, "Resetting");
		scrollMessage("reset");
		delayESP8266(1000);
		ESP.restart();
	}

	if (strcmp(topic, MQTT_TOPIC_ON) == 0)
	{
		dbgprintln(ico_null, "on");
		// Enable display
		setDisplayState(ON);
	}

	if (strcmp(topic, MQTT_TOPIC_OFF) == 0)
	{
		dbgprintln(ico_null, "off");
		// Disable display
		setDisplayState(OFF);
	}

	// hide loader anmiation on website
	serverShowLoader(false);

	// dbgprintln(ico_warning, getAllRingBufferItems().c_str());
}

/*
 *	return the number of elements in a struct array ringBuffer
 */
int getRingBufferIndex()
{
	int ringBufferSize = (sizeof(ringBuffer) / sizeof(*ringBuffer));
	return ringBufferSize;
}

/*
 *
 */
String getAllRingBufferItems()
{
	return ringBufferGetAllItemsCircular();
	// char buf[2048] = {'\0'};
	// ringBufferGetAllItemsCircular(buf);
	// return String(buf);
}

/*
 *
 */
char openWeatherString[32] = {'\0'};
String getLastopenWeather()
{
	return String(openWeatherString);
}

/*
 *	https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/TestEspApi/TestEspApi.ino
 */
void setup_wifi()
{
	// scrollMessage("wifi setup");
	WiFi.setPhyMode(WIFI_PHY_MODE_11N); // Force 802.11N connection

	delayESP8266(10);
	// We start by connecting to a WiFi network
	dbgprintln();
	dbgprintf(ico_ok, "Connecting to %s", cfg.ssid);

	WiFi.mode(WIFI_STA);
	WiFi.begin(cfg.ssid, cfg.password);

	int counter = 0;
	while (WiFi.status() != WL_CONNECTED)
	{
		delayESP8266(500);
		if (++counter > 100)
			ESP.restart();
		Serial.print(".");
	}

	WiFi.hostname((const char *)cfg.hostname);

	randomSeed(micros());

	dbgprintln();
	dbgprintf(ico_ok, "WiFi connected with IP address: %s", (WiFi.localIP().toString()).c_str());

	// scrollMessage("connected");
}

/*
 *  https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_reconnect_nonblocking/mqtt_reconnect_nonblocking.ino
 *	boolean connect (clientID, [username, password], [willTopic, willQoS, willRetain, willMessage], [cleanSession])
 */
boolean reconnect()
{
	// Attempt to connect
	// uint8_t willQos = 2;
	char mqtt_clientID[24];
	if (snprintf(mqtt_clientID, sizeof(mqtt_clientID), "%s-%X", MQTT_CLIENT_ID, random(0xffff)) < 0) // Means append did not (entirely) fit
	{
		strcpy(mqtt_clientID, MQTT_CLIENT_ID);
	}

	if (client.connect(mqtt_clientID, cfg.mqtt_user, cfg.mqtt_password)) //, MQTT_WILL_TOPIC, willQos, true, MQTT_WILL_MESSAGE, true))
	{
		dbgprintf(ico_info, "mqtt connected with [%s], subscribing [%s]", mqtt_clientID, MQTT_SUBSCRIBE_TOPIC);
		client.subscribe(MQTT_SUBSCRIBE_TOPIC);
	}
	else
	{
		dbgprintf(ico_error, "mqtt client connect failed, rc=%d", client.state());
	}
	return client.connected();
}

/*
 *
 */
void setup_OTA()
{
	ArduinoOTA.setHostname((const char *)cfg.hostname);

	ArduinoOTA.onStart([]() {
		// Clean SPIFFS
	    SPIFFS.end();

		// show loader anmiation on website
		serverShowLoader(true);		

		dbgprintln(ico_null, "Start");
		send_Event("[OTA] Starting ...", "debug");
		staticMessage("ota .");
	});
	ArduinoOTA.onEnd([]() {
		dbgprintln(ico_null, "End");
		send_Event("[OTA] Successfully done.", "debug");
		staticMessage("ota !");

		// hide loader anmiation on website
		serverShowLoader(false);
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		dbgprintf(ico_info, "[OTA] Progress: %d%%", progress / (total / 100));
		char buf[32] = {'\0'}; 
		if (snprintf(buf, sizeof(buf), " %d%%   [OTA] Progress", abs(progress / (total / 100))) < 0) // Means append did not (entirely) fit
		{
			buf[0] = '\0';
		}
		send_Event(buf, "debug");
		staticMessage(buf);
	});
	ArduinoOTA.onError([](ota_error_t error) {
		char buf[32] = {'\0'};
		if (snprintf(buf, sizeof(buf), "[OTA] Error: %s", String(error).c_str()) < 0) // Means append did not (entirely) fit
		{
			buf[0] = '\0';
		}
		send_Event(buf, "debug");
		dbgprintln(ico_info, buf);
		scrollMessage("ota error");
		if (error == OTA_AUTH_ERROR)
			dbgprintln(ico_info, "Auth Failed");
		else if (error == OTA_BEGIN_ERROR)
			dbgprintln(ico_info, "Begin Failed");
		else if (error == OTA_CONNECT_ERROR)
			dbgprintln(ico_info, "Connect Failed");
		else if (error == OTA_RECEIVE_ERROR)
			dbgprintln(ico_info, "Receive Failed");
		else if (error == OTA_END_ERROR)
			dbgprintln(ico_info, "End Failed");
	});
	if (OTA_Password != "")
	{
		ArduinoOTA.setPassword(((const char *)OTA_Password.c_str()));
	}
	ArduinoOTA.begin();
	dbgprintf(ico_info, "FOTA Initialized using IP address: %s", WiFi.localIP().toString().c_str());
}

struct tm tm;
// https://fipsok.de/Esp8266-Webserver/ntp-zeit-esp8266.tab
// enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)
const char *const PROGMEM TZ_INFO = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
// #define MYTZ TZ_Europe_Berlin
const char *const PROGMEM NTP_SERVER[] = {"fritz.box", "de.pool.ntp.org", "ptbtime1.ptb.de", "europe.pool.ntp.org"};
/*
 *	https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/NTP-TZ-DST/NTP-TZ-DST.ino
 */
void setupTime()
{
	// if (esp8266::coreVersionNumeric() >= 20700000)
	// {
	configTime(TZ_INFO, NTP_SERVER[0], NTP_SERVER[1], NTP_SERVER[2]); // check TZ.h, find your location
																	  // }
																	  // else
																	  // {
																	  // 	dbgprintln(ico_null, "using 'setenv' because esp8266::coreVersionNumeric() < 20700000");
																	  // 	// setenv("TZ", TZ_INFO, 1);
																	  // 	configTime(0, 0, NTP_SERVER[0], NTP_SERVER[1], NTP_SERVER[2]);
																	  // }
}

/*
 *
 */
void showTime(bool dots = true)
{
	// static char timestr[6]; // je nach Format von "strftime" eventuell anpassen
	static char timestr[6] = {'\0'}; // 12:34 => 5 + \0
	static time_t lastsec = 0;
	time_t now = time(&now);
	localtime_r(&now, &tm);

	pulseCount24hour++;
	if (tm.tm_hour == 0 && tm.tm_min == 0 && tm.tm_sec < 5)
	{
		pulseCount24hour = 0;
	}

	if (tm.tm_sec != lastsec)
	{
		lastsec = tm.tm_sec;
		strftime(timestr, sizeof(timestr), "%H:%M", &tm); // http://www.cplusplus.com/reference/ctime/strftime/
		// dbgprintln(ico_null, timestr);

		// einmal am Tag die Zeit vom NTP Server holen o. jede Stunde "% 3600" aller zwei "% 7200"
		if (!(time(&now) % 86400))
		{
			setupTime();
		}
	}

	// start drawing
	matrix.fillScreen(LOW);

	int x = 0;
	// loop for each character in 'tape'
	for (int i = 0; i < abs(strlen(timestr)); i++)
	{
		x = i * width; // step horizontal by width of character and spacer
		// unsigned char c
		matrix.drawChar(x + 1, baseline, timestr[i], HIGH, LOW, 1); // draw a character from 'tape'
	}

	// show seconds as pixel dot at the bottom of the matrix
	if (dots == true)
	{
		secCounter = tm.tm_sec; //second(tm.tm_sec);
		if (secCounter > 29)
		{
			secCounter = secCounter - 30;  // the display can show only 30 seconds as pixel dots
			matrix.writePixel(0, 7, HIGH); // dot shows seconds > 30
		}
		else
		{
			matrix.writePixel(0, 7, LOW);
		}
		matrix.writePixel(secCounter + 1, 7, HIGH);
	}

	matrix.write(); // Send bitmap to display
	delayESP8266(SECOND * 1);
	if (dots == true)
	{
		matrix.writePixel(secCounter, 7, LOW); // remove last second's dot
	}
}

/* 
 *
 */
void handleTwilight()
{
	// calculate sunset/sunrise only, when not taken from OpenWeatherMap
	// and get sunset and sunrise each midnight
	if ((twilightInit == true || pulseCount24hour == 0))
	{
		twilightInit = false;
		calculateSunriseSunset();
	}

	// check sunrise in the mornings
	time_t now = time(&now);
	localtime_r(&now, &tm);

	static char timestr[6] = "00:00";				  // => 5 + \0
	strftime(timestr, sizeof(timestr), "%H:%M", &tm); // http://www.cplusplus.com/reference/ctime/strftime/

	// dbgprintf(ico_null, "%s - sunrise: %s - sunset %s", timestr, sunriseTime, sunsetTime);
	if (strcmp(timestr, sunriseTime) == 0)
	{
		setDisplayState(ON);
		dbgprintf(ico_null, "Sunrise: %02d:%02d", tm.tm_hour, tm.tm_min);
	}
	// check sunset in the evenings
	if (strcmp(timestr, sunsetTime) == 0)
	{
		setDisplayState(OFF);
		dbgprintf(ico_null, "Sunset: %02d:%02d", tm.tm_hour, tm.tm_min);
	}
}

/* 
 *
 */
void getTimeFromUnix(char *timestr, uint32_t unix, int offset = 0)
{

	uint16_t seconds = unix % 60; /* Get seconds from unix */
	unix /= 60;					  /* Go to minutes */
	uint16_t minutes = unix % 60; /* Get minutes */
	unix /= 60;					  /* Go to hours */
	uint16_t hours = unix % 24;	  /* Get hours */
	hours += offset;

	char buf[6] = "00:00";
	if (snprintf(buf, sizeof(buf), "%02d%:%02d\n", hours, minutes) < 0) // Means append did not (entirely) fit
	{
		Serial.println("error");
	}
	Serial.println(timestr);
	strcpy(timestr, buf);
}

/* 
 *
 */
boolean getOpenWeather(const char *api_key, float latitude, float longitude)
{
	const char *servername = "api.openweathermap.org"; // remote server we will connect to
	const char *apiGetDataFormat = "GET /data/2.5/weather?lat=%f&lon=%f&units=metric&lang=de&appid=%s HTTP/1.1";

	char apiGetData[128] = {'\n'};
	if (snprintf(apiGetData, sizeof(apiGetData), apiGetDataFormat, latitude, longitude, api_key) < 0) // Means append did not (entirely) fit
	{
		return false;
	}
	dbgprintln(ico_null, apiGetData);

	WiFiClient weatherClient;
	if (weatherClient.connect(servername, 80))
	{ 	//starts client connection, checks for connection
		weatherClient.println(apiGetData);
		weatherClient.println("Host: " + String(servername));
		weatherClient.println("User-Agent: ArduinoWiFi/1.1");
		weatherClient.println("Connection: close");
		weatherClient.println();
	}
	else
	{
		dbgprintf(ico_error, "connection for weather data failed"); //error message if no client connect
		strlcpy(local_weather.error, "Connection for weather data failed", 64);
		return false;
	}

	while (weatherClient.connected() && !weatherClient.available())
		delay(1); //waits for data

	dbgprintln(ico_null, "Waiting for data");

	// Check HTTP status
	char status[64] = {'\0'};
	weatherClient.readBytesUntil('\r', status, sizeof(status));
	dbgprintf(ico_null, "Response Header: %s", status);
	if (strcmp(status, "HTTP/1.1 200 OK") != 0)
	{
		dbgprintf(ico_error, "Unexpected response: %s", status);
		// strlcpy(local_weather.error, "Weather Data Error: " + status, sizeof(status));
		strlcpy(local_weather.error, "Weather data error!", 64);
		return false;
	}

	// Skip HTTP headers
	char endOfHeaders[] = "\r\n\r\n";
	if (!weatherClient.find(endOfHeaders))
	{
		dbgprintln(ico_error, "Invalid response");
		return false;
	}

	DynamicJsonDocument doc(1024); // 790 --> https://arduinojson.org/v6/assistant/

	// for debugging only
	// const char* json = "{\"coord\":{\"lon\":6.69,\"lat\":51.24},\"weather\":[{\"id\":803,\"main\":\"Clouds\",\"description\":\"Überwiegend bewölkt\",\"icon\":\"04n\"}],\"base\":\"stations\",\"main\":{\"temp\":12.55,\"feels_like\":12.02,\"temp_min\":11.67,\"temp_max\":13.33,\"pressure\":1023,\"humidity\":87},\"visibility\":10000,\"wind\":{\"speed\":1,\"deg\":80},\"clouds\":{\"all\":76},\"dt\":1599952374,\"sys\":{\"type\":1,\"id\":1264,\"country\":\"DE\",\"sunrise\":1599973545,\"sunset\":1600019559},\"timezone\":7200,\"id\":2863150,\"name\":\"Niederlörick\",\"cod\":200}";
	// const char* json = "{\"coord\":{\"lon\":-0.13,\"lat\":51.51},\"weather\":[{\"id\":300,\"main\":\"Drizzle\",\"description\":\"light intensity drizzle\",\"icon\":\"09d\"}],\"base\":\"stations\",\"main\":{\"temp\":280.32,\"pressure\":1012,\"humidity\":81,\"temp_min\":279.15,\"temp_max\":281.15},\"visibility\":10000,\"wind\":{\"speed\":4.1,\"deg\":80},\"clouds\":{\"all\":90},\"dt\":1485789600,\"sys\":{\"type\":1,\"id\":5091,\"message\":0.0103,\"country\":\"GB\",\"sunrise\":1485762037,\"sunset\":1485794875},\"id\":2643743,\"name\":\"London\",\"cod\":200}";

	// Parse JSON object
	// DeserializationError error = deserializeJson(doc, json);
	DeserializationError error = deserializeJson(doc, weatherClient);
	if (error)
	{
		dbgprintln(ico_error, "Weather data parsing failed!");
		strlcpy(local_weather.error, "Weather data parsing failed!", 64);
		return false;
	}

	weatherClient.stop(); //stop client

	size_t len = measureJson(doc);
	if (len <= 150)
	{
		dbgprintf(ico_error, "Error: gathering of data seems too less, size: %d", len);
		local_weather.cached = true;
		strlcpy(local_weather.error, doc["message"], 64);
		dbgprintf(ico_error, "Error: %s", local_weather.error);
		return false;
	}

#ifdef MY_DEBUG
	dbgprint(ico_null, "serializeJsonPretty:");
	serializeJsonPretty(doc, Serial);
	dbgprint(ico_null, "");
#endif // MY_DEBUG

	local_weather.lon = doc["coord"]["lon"].as<float>();		 	// 6.69
	local_weather.lat = doc["coord"]["lat"].as<float>();		 	// 51.24
	local_weather.visibility = doc["visibility"];				 	// 10000
	local_weather.wind_speed = doc["wind"]["speed"].as<float>(); 	// 0.5
	local_weather.clouds_all = doc["clouds"]["all"].as<int>();
																	// 93
	local_weather.dt = doc["dt"].as<long>();						// 1599945531
	local_weather.timezone = doc["timezone"].as<int>(); 			// 7200
	local_weather.weatherId = doc["id"].as<int>();					// 2863150
	strlcpy(local_weather.city, doc["name"], 32);					// "Niederlörick"

	JsonObject weather_0 = doc["weather"][0];
	local_weather.weatherId = weather_0["id"].as<long>();			// 804
	strlcpy(local_weather.condition, weather_0["main"], 32);		// "Clouds"
	strlcpy(local_weather.description, weather_0["description"], 32); // "overcast clouds"
	strlcpy(local_weather.icon, weather_0["icon"], 32);				// "04n"

	JsonObject main = doc["main"];
	local_weather.temp = main["temp"].as<float>();		 			// 286.88
	local_weather.pressure = main["pressure"].as<int>(); 			// 1023
	local_weather.humidity = main["humidity"].as<int>(); 			// 82

	JsonObject sys = doc["sys"];
	// local_weather.cityId = sys["id"].as<long>(); // 1264
	strlcpy(local_weather.country, sys["country"], 3); 				// "DE"
	local_weather.sunrise = sys["sunrise"].as<long>(); 				// 1599887051
	// getTimeFromUnix(sunriseTime, local_weather.sunrise, cfg.offset);
	local_weather.sunset = sys["sunset"].as<long>(); 				// 1599933297
	// getTimeFromUnix(sunsetTime, local_weather.sunset, cfg.offset);

	// http://www.cplusplus.com/reference/cstdio/printf/
	dbgprintf(ico_null, "lat: %f", local_weather.lat);
	dbgprintf(ico_null, "lon: %f", local_weather.lon);
	dbgprintf(ico_null, "dt: %ul", local_weather.dt);
	dbgprintf(ico_null, "city: %s", local_weather.city);
	dbgprintf(ico_null, "country: %s", local_weather.country);
	dbgprintf(ico_null, "temp: %f", local_weather.temp);
	dbgprintf(ico_null, "humidity: %d", local_weather.humidity);
	dbgprintf(ico_null, "pressure: %d", local_weather.pressure);
	dbgprintf(ico_null, "condition: %s", local_weather.condition);
	dbgprintf(ico_null, "wind: %f", local_weather.wind_speed);
	dbgprintf(ico_null, "clouds: %d", local_weather.clouds_all);
	dbgprintf(ico_null, "weatherId: %ul", local_weather.weatherId);
	// dbgprintf(ico_null, "cityId: %ul", local_weather.cityId);
	dbgprintf(ico_null, "description: %s", local_weather.description);
	dbgprintf(ico_null, "icon: %s", local_weather.icon);
	dbgprintf(ico_null, "timezone: %d", local_weather.timezone);
	dbgprintf(ico_null, "");

	return true;
} // getOpenWeather()

/* 
 *
 */
void getWeather(char *openWeatherString)
{
	// gather data only once per hour
	if (millis() - lastOpenWeatherCheck < 1 * HOUR )
	{
		dbgprintf(ico_info, "false, lastOpenWeatherCheck %u - %u < %u", millis(), lastOpenWeatherCheck, 1 * HOUR);
		return;
	} 
	dbgprintf(ico_info, "ok, lastOpenWeatherCheck %u - %u > %u", millis(), lastOpenWeatherCheck, 1 * HOUR);

	if (strlen(cfg.api_key) == 0)
	{
		strcpy(openWeatherString, "no weather");
	}
	else if (getOpenWeather(cfg.api_key, cfg.latitude, cfg.longitude))
	{
		char msg[64] = {'\n'};
		if (snprintf(msg, sizeof(msg), "%s %.1f°C %d%% %d Bft %d hPa",
					 local_weather.description,
					 local_weather.temp,
					 local_weather.humidity,
					 wind2Beaufort(round(local_weather.wind_speed)),
					 local_weather.pressure) < 0) // Means append did not (entirely) fit
		{
			strcpy(openWeatherString, local_weather.error);
		}
		scrollMessage(msg);
		strcpy(openWeatherString, msg);
	}
	lastOpenWeatherCheck = millis();
} // getWeather()

/*****************************************( setup )****************************************/
void setup()
{
#ifdef MY_DEBUG
	Serial.begin(9600);
	while (!Serial)
	{
	} // Wait
#endif

	delayESP8266(2000);

	cpuLastMicros = micros();

	readConfigFromJson();

	ringBufferInit();

	pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
	digitalWrite(LED_BUILTIN, HIGH);

	LDRvalue = 0;
	setDisplayBrightness(cfg.brightness, boolean(cfg.useLDR), false); //Helligkeit von Display auf Default Wert einstellen
	matrix.fillScreen(LOW);											  // show nothing

	int maxPos = numberOfHorizontalDisplays * numberOfVerticalDisplays;
	for (int i = 0; i < maxPos; i++)
	{
		matrix.setRotation(i, ledRotation);
	}

	matrix.cp437(true);
#ifdef FONT
	matrix.setFont(&FONT);
	matrix.setTextSize(1);
#endif

	// staticMessage("setup");

	setup_wifi();

	if (MDNS.begin((const char *)cfg.hostname))
	{
		dbgprintf(ico_ok, "MDNS responder started with [%s]", cfg.hostname);
	}
	MDNS.addService("http", "tcp", 80);

	setup_webServer();

	client.setServer(cfg.mqtt_server, 1883);
	client.setCallback(callback);
	lastReconnectAttempt = 0;

	delayESP8266(1000); //1 Sekunde warten

	setupTime();

	setBootTime();

	if (ENABLE_OTA)
		setup_OTA();

#ifndef MY_DEBUG
	getWeather(openWeatherString);
#endif	
	// scrollMessage("init done");
}

/* 
 *
 */
unsigned long getCpuDelta()
{
	unsigned long thisMicros, delta;
	thisMicros = micros();
	delta = thisMicros - cpuLastMicros;
	cpuLastMicros = thisMicros;
	dbgprintf(ico_info, "cpu load: %lu ms", delta);
	return delta;
}

/***********************************( Hauptprogramm )*************************************/
void loop()
{
	// avgCpuDelta = getCpuDelta();

	if (WiFi.status() != WL_CONNECTED)
	{
		setup_wifi();
	}

	yield(); // Do (almost) nothing -- yield to allow ESP8266 background functions

	MDNS.update();

	loop_webServer();

	if (ENABLE_OTA)
	{
		// show loader anmiation on website
		serverShowLoader(true);		
		ArduinoOTA.handle();
		// hide loader anmiation on website
		serverShowLoader(false);		
	}

	// https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_reconnect_nonblocking/mqtt_reconnect_nonblocking.ino
	if (!client.connected())
	{
		long now = millis();
		if (now - lastReconnectAttempt > 5000)
		{
			lastReconnectAttempt = now;
			// Attempt to reconnect
			if (reconnect())
			{
				lastReconnectAttempt = 0;
			}
		}
	}
	else
	{
		// Client connected
		client.loop();
	}

	LDRvalue = map(analogRead(outputpin), 0, 1023, 0, 15); // Helligkeit 0 bis 15
	if (cfg.useLDR == 1)
	{																 // set level using A0 pin
		setDisplayBrightness(LDRvalue, true, true);
		dbgprintf(ico_info, "LDR brightness: %d", LDRvalue);
	}

	if (cfg.twilight == 1)
		handleTwilight();

	showTime();

	if (timer1h.check() == 1)
	{ // once per hour request data from openweathermap
		getWeather(openWeatherString);
	}

	if (timer5Min.check() == 1)
	{ // every n minutes scroll last weather string
		scrollMessage(openWeatherString);
	}
}
