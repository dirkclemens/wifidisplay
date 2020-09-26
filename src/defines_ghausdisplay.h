/*
 *  mosquitto_pub -v -h 192.168.2.222 -u dirk -P ****** -t "smarthome/ghausdisplay/message" -m "test"
 */
#define WEB_TITLE               "wifi display gartenhaus"
#define HOSTNAME 				"ghausdisplay"
#define MQTT_CLIENT_ID 			"ghausdisplay"
#define MQTT_SUBSCRIBE_TOPIC 	"smarthome/ghausdisplay/#"
#define MQTT_WILL_TOPIC 	    "smarthome/ghausdisplay/status"
#define MQTT_WILL_MESSAGE	    "ghausdisplay is offline"
#define MQTT_TOPIC_MESSAGE 		"smarthome/ghausdisplay/message"
#define MQTT_TOPIC_JSON 		"smarthome/ghausdisplay/json"
#define MQTT_TOPIC_NOTICE 		"smarthome/ghausdisplay/notice"
#define MQTT_TOPIC_ALARM 		"smarthome/ghausdisplay/alarm"
#define MQTT_TOPIC_DIM 			"smarthome/ghausdisplay/dim"
#define MQTT_TOPIC_SPEED 		"smarthome/ghausdisplay/speed"
#define MQTT_TOPIC_STATIC 		"smarthome/ghausdisplay/static"
#define MQTT_TOPIC_SCROLL 		"smarthome/ghausdisplay/scroll"
#define MQTT_TOPIC_WIFI 		"smarthome/ghausdisplay/wifi"
#define MQTT_TOPIC_RESET 		"smarthome/ghausdisplay/reset"
#define MQTT_TOPIC_ON 		    "smarthome/ghausdisplay/on"
#define MQTT_TOPIC_OFF 		    "smarthome/ghausdisplay/off"
