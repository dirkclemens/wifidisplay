/*
 *
 */
#include "config.h"
#include "common.h"
#include "uptime.h"

// web elements
#include "index_html.h"
#include "style_css.h"
#include "favicon_ico_gz.h"
#include "mask_icon_svg_gz.h"
#include "reboot_html.h"

#include "WebServer.h"
#include <ESP8266WiFi.h>
#include "ESP8266WiFi.h"
#include "ESPAsyncTCP.h"
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

bool eventTriggered 						= true;

enum tabs {
	home 		= 0,
	stats		= 1,
	network		= 2,
	settings	= 3
} tab_selected;

//flag to use from web update to reboot the ESP
bool shouldReboot 							= false;

/* 
 *
 */
void serverShowLoader(boolean show)
{
	events.send(show ? "block" : "none", "loader");
}

/* 
 *
 */
void send_Event(const char *content, const char *section)
{
	events.send(content, section);
}

/* 
 *
 */
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
	// dbgprintf(ico_info, "ws[%s][%u] connect\n", server->url(), client->id());
	if(type == WS_EVT_DATA)
	{
    	AwsFrameInfo * info = (AwsFrameInfo*)arg;
		char _payload[len];
		if(info->final && info->index == 0 && info->len == len){
			//the whole message is in a single frame and we got all of it's data
			// dbgprintf(ico_info, "ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
			if(info->opcode == WS_TEXT){
				data[len] = '\0';
				size_t i = 0;
				for (i = 0; i < len; i++)
				{
					_payload[i] = (char)data[i];
				}
				_payload[i] = '\0';				
			}
			// what tab was beeing clicked by the user in the javascript doTab() function ? 
			tab_selected = home; // default
			if (strcmp(_payload, "tab_stats") == 0 ) tab_selected = stats;
			if (strcmp(_payload, "tab_network") == 0 ) tab_selected = network;
			if (strcmp(_payload, "tab_settings") == 0 ) tab_selected = settings;
			// dbgprintf(ico_info, "websocket message: [%s]\n", _payload);
		}
	}
}

/* 
 *
 */
// https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WebServer/examples/PostServer/PostServer.ino
void handleNotFound(AsyncWebServerRequest *request)
{
	String message = "404: File Not Found\n\n";
	message += "URI: ";
	message += request->url();
	message += "\nMethod: ";
	message += (request->method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += request->args();
	message += "\n";
	for (uint8_t i = 0; i < request->args(); i++) {
		message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
	}
	request->send(404, "text/plain", message);
}

/* 
 *
 */
// http://192.168.2.101/brightness?value=15
void handleBrightness(AsyncWebServerRequest *request)
{
#ifdef MY_DEBUG	
	dbgprintln(ico_null, "handleBrightness");
	String message = "Number of args received:\n";
	int params = request->params();
	for (int i = 0; i < params; i++) {	
		AsyncWebParameter* p = request->getParam(i);
		if(p->isPost()){
			dbgprintln(ico_null, "isPost");
		} else {
			dbgprintln(ico_null, "isGet");
		}
		message += "Arg #" + (String)i + " -> ";   	//Include the current iteration value
		message += p->name() + "=";     			//Get the name of the parameter
		message += p->value() + "\n";      			//Get the value of the parameter
	}
	dbgprintln(ico_null, message.c_str());
#endif // MY_DEBUG

	// AsyncWebParameter* p = request->getParam(0);
	// if(p->name() == "value")
	char msg[32] = {'\0'};
	auto p = request->getParam("value", true);
	if(p)
	{
		int brightness = p->value().toInt();
		dbgprintf(ico_null, "request contains value: %d", brightness);
		if (brightness < 0) brightness = 0;		
		// if (brightness > 100) brightness = 100;
		if (brightness > 15) brightness = map(brightness, 0, 100, 0, 15);
		setDisplayBrightness(brightness, false, true);
		if (snprintf(msg, sizeof(msg), "brightness set to %d (0..15)", brightness) < 0) // Means append did not (entirely) fit
		{
			msg[0] = '\0';
		}
		request->send(200, "text/plain", msg);
	} else 
	{
		dbgprintln(ico_null, "request without value, returning state only");
		if (snprintf(msg, sizeof(msg), "%d", getDisplayBrightness()) < 0) // Means append did not (entirely) fit
		{
			msg[0] = '\0';
		}
		request->send(200, "text/plain", msg);		
	}
}

/* 
 *
 */
void getStats()
{
	static char timestr[21] = {'\0'}; // Aug 02 2020 12:34:56 => 20 + \0
	getCurrentTimeString(timestr, "%b %d %Y %H:%M:%S");

	Config cfg = getConfig();
	char content[1024] = {'\0'};
	if (snprintf_P(content, sizeof(content), stats_html, 
			(const char *)WiFi.hostname().c_str(),
			getBootTime(),
			timestr,
			__DATE__,
			__TIME__,
			runtime(),
			uptime(),
			(const char *)getSunsetTime().c_str(),
			(const char *)getSunriseTime().c_str(),
			(const char *)formatBytes(ESP.getFreeHeap()).c_str(),
			(const char *)formatBytes(ESP.getFreeSketchSpace()).c_str(),
			(const char *)formatBytes(ESP.getFlashChipSize()).c_str(),
			(const char *)formatBytes(ESP.getFlashChipRealSize()).c_str(),
			// ESP.getCpuFreqMHz(),
			// ESP.getFlashChipSpeed() / 1000000,
			ESP.getResetReason().c_str()
			// CORE_VERSION,
			// SW_VERSION
		) < 0) {
			strcpy(content, "<div class=\"error\"> ERROR </div>");
		}
	events.send(content, "stats");
	content[0] = {'\0'};
}

/* 
 *
 */
void getNetwork()
{
	Config cfg = getConfig();
	char content[1024] = {'\0'};
	if (snprintf_P(content, sizeof(content), network_html, 
			(const char *)WiFi.hostname().c_str(),
			(const char *)WiFi.localIP().toString().c_str(),
			(const char *)WiFi.SSID().c_str(),
			(const char *)WiFi.gatewayIP().toString().c_str(),
			WiFi.channel(),
			(const char *)WiFi.macAddress().c_str(),
			(const char *)WiFi.subnetMask().toString().c_str(),
			(const char *)WiFi.dnsIP().toString().c_str(),
			WiFi.RSSI()
		) < 0) {
			strcpy(content, "<div class=\"error\"> ERROR </div>");
		}
	events.send(content, "network");
	content[0] = {'\0'};
}

/* 
 *
 */
void getLog()
{
	static char timestr[21] = {'\0'}; // Aug 02 2020 12:34:56 => 20 + \0
	getCurrentTimeString(timestr, "%b %d %Y %H:%M:%S");

	char content[1024] = {'\0'};
	if (snprintf_P(content, sizeof(content), log_html, 
			getAllRingBufferItems().c_str(),
			getLastopenWeather().c_str(),
			getDisplayBrightness(),
			getLDRvalue(),
			timestr,
			uptime(),
			getSunsetTime().c_str(),
			getSunriseTime().c_str()
		) < 0) {
			
			strcpy(content, "<div class=\"error\"> ERROR </div>");
		}
	events.send(content, "home");
	content[0] = {'\0'};
}

/* 
 *
 */
String processor(const String& var)
{
	Config cfg = getConfig();
	// dbgprintln(ico_null, var.c_str());

	if (var == "SETTINGS_HOSTNAME"){
		return String(cfg.hostname);
	}
	if (var == "SETTINGS_SSID"){
		return String(cfg.ssid);
	}
	if (var == "SETTINGS_PASSWORD"){
		return String(cfg.password);
	}
	if (var == "SETTINGS_MQTT_SERVER"){
		return String(cfg.mqtt_server);
	}
	if (var == "SETTINGS_MQTT_SUBSCRIPTION"){
		return String(cfg.mqtt_subscription);
	}
	if (var == "SETTINGS_MQTT_USER"){
		return String(cfg.mqtt_user);
	}
	if (var == "SETTINGS_MQTT_PASSWORD"){
		return String(cfg.mqtt_password);
	}
	if (var == "SETTINGS_APIKEYR"){
		return String(cfg.api_key);
	}
	if (var == "SETTINGS_CITYID"){
		return String(cfg.cityid);
	}
	if (var == "SETTINGS_LATITUDE"){
		char lat[10];
		ftoa(lat, cfg.latitude, 7);
		return String(lat);
	}	
	if (var == "SETTINGS_LONGITUDE"){
		char lon[10];
		ftoa(lon, cfg.longitude, 7);
		return String(lon);
	}
	if (var == "SETTINGS_OFFSET"){
		return String(cfg.offset);
	}
	if (var == "SETTINGS_TWILIGHT"){
		if (cfg.twilight == 1) return String("checked"); // checked="checked"
		else return String();
	}
	if (var == "SETTINGS_BRIGHTNESS"){
		return String(getDisplayBrightness());
	}
	if (var == "SETTINGS_USELDR"){
		if (cfg.useLDR == 1) return String("checked"); // checked="checked"
		else return String();
	}
	
	if (var == "PLACEHOLDER_ONOFFBUTTON"){
		if (getDisplayState() == ON)
		{
			return F("<a class=\"button\" href=\"/off\">off</a>");
		}
		else
		{
			return F("<a class=\"button\" href=\"/on\">on</a>");
		}
	}	
	return String();
}

/* 
 *	https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/CheckFlashConfig/CheckFlashConfig.ino
 *	https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/TestEspApi/TestEspApi.ino
 */ 
const char * const FLASH_SIZE_MAP_NAMES[] = {
  "FLASH_SIZE_4M_MAP_256_256",
  "FLASH_SIZE_2M",
  "FLASH_SIZE_8M_MAP_512_512",
  "FLASH_SIZE_16M_MAP_512_512",
  "FLASH_SIZE_32M_MAP_512_512",
  "FLASH_SIZE_16M_MAP_1024_1024",
  "FLASH_SIZE_32M_MAP_1024_1024"
};
const char * const RST_REASONS[] = {
  "REASON_DEFAULT_RST",
  "REASON_WDT_RST",
  "REASON_EXCEPTION_RST",
  "REASON_SOFT_WDT_RST",
  "REASON_SOFT_RESTART",
  "REASON_DEEP_SLEEP_AWAKE",
  "REASON_EXT_SYS_RST"
};
void sendStatus(AsyncWebServerRequest *request)
{
	uint32_t realSize = ESP.getFlashChipRealSize();
	uint32_t ideSize = ESP.getFlashChipSize();
	FlashMode_t ideMode = ESP.getFlashChipMode();

	AsyncResponseStream *resp = request->beginResponseStream("text/plain");
	resp->print ("Status\n-----------------------------\n");
	resp->printf("    ESP Chip id: 0x%8x\n", ESP.getChipId());
	resp->println();
	resp->printf("  Flash real id: 0x%8x\n", ESP.getFlashChipId());
	resp->printf("Flash real size: %d bytes\n", realSize);
	resp->printf("Flash ide  size: %d bytes\n", ideSize);
	resp->printf("Flash ide speed: %d Hz\n",  ESP.getFlashChipSpeed());
	resp->print ("Flash ide  mode: " + String(ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN") + "\n");
	resp->println();
	resp->printf("           Heap: %d bytes\n", ESP.getFreeHeap());
	resp->println();

	resp->printf("          Flash: %s\n", FLASH_SIZE_MAP_NAMES[system_get_flash_size_map()]);
	resp->println();

	const rst_info * resetInfo = system_get_rst_info();
  	resp->printf("   reset reason: %s\n", RST_REASONS[resetInfo->reason]);
	resp->println();

	FSInfo fs_info;
	if (SPIFFS.info(fs_info))
	{
		resp->printf("  FS totalBytes: %d bytes\n", fs_info.totalBytes);
		resp->printf("  FS  usedBytes: %d bytes\n", fs_info.usedBytes);
		resp->printf("  FS  blockSize: %d bytes\n", fs_info.blockSize);
		resp->printf("  FS   pageSize: %d bytes\n", fs_info.pageSize);
		resp->printf("   maxOpenFiles: %d \n", fs_info.maxOpenFiles);
		resp->printf("  maxPathLength: %d \n", fs_info.maxPathLength);
		resp->println();
	}

	resp->print ("Files\n-----------------------------\n");
	Dir dir = SPIFFS.openDir("/");
	while (dir.next()) {    
		resp->printf("FS File: %s, size: %s\n",  dir.fileName().c_str(), formatBytes(dir.fileSize()).c_str());
	}
	resp->println();

	resp->print ("config.json\n-----------------------------\n");
	File configFile = SPIFFS.open("/config.json", "r");
	String data = configFile.readString();
	configFile.close();
	resp->printf("%s\n", data.c_str());

	request->send(resp);
}

/* 
 *
 */
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  if(!index){
    Serial.printf("UploadStart: %s\n", filename.c_str());
  }
  for(size_t i=0; i<len; i++){
    Serial.write(data[i]);
  }
  if(final){
    Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index+len);
  }
}

/* 
 *
 */
void setup_webServer()
{
	tab_selected = home;

	// if (!SPIFFS.begin())
	// {
	// 	dbgprintln(ico_null, "SPIFFS Mount failed");
	// }
	// else
	// {
	// 	dbgprintln(ico_ok, "SPIFFS Mount succesfull");
	// 	FSInfo fs_info ;   
	// 	SPIFFS.info(fs_info) ;
  	// 	dbgprintf(ico_null, "FS Total %d, used %d", fs_info.totalBytes, fs_info.usedBytes ) ;		
	// 	Dir dir = SPIFFS.openDir("/");
	// 	while (dir.next()) {    
	// 		dbgprintf(ico_null, "FS File: %s, size: %s",  dir.fileName().c_str(), formatBytes(dir.fileSize()).c_str());
	// 	}
	// }

	ws.onEvent(onWsEvent);
	server.addHandler(&ws);

	events.onConnect([](AsyncEventSourceClient *client) {
		client->send("connected !", NULL, millis(), 1000);
	});
	server.addHandler(&events);

	server.on("/favicon.ico", [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", favicon_ico_gz, sizeof(favicon_ico_gz));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	server.on("/mask-icon.svg", [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "image/svg", mask_icon_svg_gz, sizeof(mask_icon_svg_gz));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	server.on("/style.css", [](AsyncWebServerRequest *request){
		request->send_P(200, "text/css", style_css, nullptr);
	});

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
#ifdef MY_DEBUG			
		String message = "URI: ";
		message += request->url();
		message += "\nMethod: ";
		message += (request->method() == HTTP_GET) ? "GET" : "POST";
		message += "\nArguments: ";
		message += request->args();
		message += "\n";
		for (uint8_t i = 0; i < request->args(); i++) {
			message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
		}
		dbgprintln(ico_null, message.c_str());
#endif //MY_DEBUG

		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html, processor);
		request->send(response);
	});	

	server.on("/content.htm", HTTP_GET, [](AsyncWebServerRequest* request){
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", content_html, processor);
		request->send(response);
	});

	// server.on("/settings", [](AsyncWebServerRequest *request) {
	// 	strcpy(menuMode, "\n"); 
	// 	request->send_P(200, "text/html", settings_html, processor);
	// });
	AsyncCallbackJsonWebHandler* jsonhandler = new AsyncCallbackJsonWebHandler("/json", [](AsyncWebServerRequest *request, JsonVariant &json) {
		dbgprintln(ico_null, "AsyncCallbackJsonWebHandler - reading data from web form");

		StaticJsonDocument<512> doc;
		DeserializationError error = deserializeJson(doc, json);
		if (error)
		{
			dbgprintln(ico_error, "Failed to deserializeJson form content");
		} else
		{
			dbgprintln(ico_ok, "deserializeJson was successfull");
			Config cfg;
			strlcpy(cfg.hostname, doc["hostname"], 32);
			strlcpy(cfg.ssid, doc["ssid"], 32);
			strlcpy(cfg.password, doc["password"], 32);
			strlcpy(cfg.mqtt_server, doc["mqtt_server"], 32);
			strlcpy(cfg.mqtt_subscription, doc["mqtt_subscription"], 32);
			strlcpy(cfg.mqtt_user, doc["mqtt_user"], 32);
			strlcpy(cfg.mqtt_password, doc["mqtt_password"], 32);
			cfg.brightness 		= doc["brightness"].as<int>();
			cfg.useLDR			= doc["ldr"].as<int>();	
			strlcpy(cfg.api_key, doc["api_key"], 64);
			// cfg.cityid			= doc["cityid"].as<int>();
			cfg.latitude			= doc["latitude"].as<double>();
			cfg.longitude		= doc["longitude"].as<double>();
			cfg.offset			= doc["offset"].as<int>();			
			cfg.twilight			= doc["twilight"].as<int>();

			printConfig(cfg);

			if (!saveConfigToJson(cfg))
			{
				dbgprintln(ico_error, "Error writing config file!");
			}					
		}
		// request->send_P(200, "text/html", settings_html, processor);
		request->send_P(200, "text/html", index_html, processor);
	});
	server.addHandler(jsonhandler);	

	server.on("/brightness", [](AsyncWebServerRequest *request) {
		handleBrightness(request);
	});

	server.on("/stats", [](AsyncWebServerRequest *request) {
		sendStatus(request);
	});

	server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
		setDisplayState(ON);
		request->send_P(200, "text/html", index_html, processor);
	});

	server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
		setDisplayState(OFF);
		request->send_P(200, "text/html", index_html, processor);
	});

	server.on("/state", [](AsyncWebServerRequest *request) {
		if (getDisplayState() == ON)
		{
			request->send(200, "text/plain", "on");  
		}
		else
		{
			request->send(200, "text/plain", "off");  
		}
	});

	server.on("/reconnect", HTTP_GET, [](AsyncWebServerRequest *request){
		// server.send(304, "message/http");
		WiFi.reconnect();
		delay(2000);
		request->send_P(200, "text/html", index_html, processor);
	});

  	server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send_P(200, "text/html", reboot_html, processor);
	});
	server.on("/reboot", HTTP_POST, [](AsyncWebServerRequest *request) {
		shouldReboot = !Update.hasError();
		char page[512] = {'\0'};
		if (snprintf(page, sizeof(page), "reboot results %s <a href=\"/\">back<a>", shouldReboot?"OK":"FAIL") < 0)
		{
			strcpy(page, "error rebooting <a href=\"/\">back<a>");
		}
		AsyncWebServerResponse *response = request->beginResponse(200, "text/html", page);
    	response->addHeader("Connection", "close");
    	request->send(response);
	});
		
	server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final)
	{
		if(!index)
			dbgprintf(ico_null, "UploadStart: %s", filename.c_str());
		dbgprintf(ico_null, "%s", (const char*)data);
		if(final)
			dbgprintf(ico_null, "UploadEnd: %s (%u)", filename.c_str(), index+len);
	});
	
	server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
	{
		if(!index)
			dbgprintf(ico_null, "BodyStart: %u", total);
		dbgprintf(ico_null, "%s", (const char*)data);
		if(index + len == total)
			dbgprintf(ico_null, "BodyEnd: %u", total);
	});

	// When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
	server.onNotFound(handleNotFound);

	server.begin();
	dbgprintln(ico_ok, "AsyncWebServer started successfully.");
}

/* 
 *
 */
void loop_webServer()
{
	if(shouldReboot){
		dbgprintln(ico_info, "Rebooting...");
		delay(100);
		ESP.restart();
	}

 	ws.cleanupClients();

	// dbgprintf(ico_info, "tab_selected: %d", tab_selected);
	switch (tab_selected)
	{
	case stats:
		getStats();
		break;
	case network:
		getNetwork();
		break;
	case settings:
		break;
	default:
		getLog();
		break;
	}
	serverShowLoader(false);


	// events
	if (eventTriggered){ // your logic here
		char timestamp[22];
		getCurrentTimeString(timestamp, "%Y-%m-%d %H:%M:%S");
		char buf[64];
		if (snprintf(buf, sizeof(buf), "%s<br />heap: %s", timestamp, formatBytes(ESP.getFreeHeap()).c_str()) < 0 ){
			strcpy(buf, "<div class=\"error\">error</div>");
		}
		events.send(buf, "debug");
		timestamp[0] = {'\0'};
		buf[0] = {'\0'};
  	}
}