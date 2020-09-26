/*
 *
 */
#include <Arduino.h>

// struct Config { // 7*32 + 5*16 // we better use size_t 512
typedef struct __attribute((__packed__)) packet_t{
    char dummy[32]       			= {'\n'}; // this field will be owerwritten somehow ??? wrong memory management ???
    char hostname[32]       		= {'\n'};
    char ssid[32]           		= {'\n'};
    char password[32]       		= {'\n'};
	char mqtt_server[32]    		= {'\n'};
	char mqtt_subscription[32]    	= {'\n'};
    char mqtt_user[32]      		= {'\n'};
    char mqtt_password[32]  		= {'\n'};
    char api_key[64]        		= {'\n'};
    int cityid              		= {'\n'};
	int useLDR			    		= 1;	// use LDR @ pin A0
	int brightness 		    		= 1;    // Brightness 0 to 15
    double latitude         		= 0;
    double longitude        		= 0;
    int offset              		= 0;
    int twilight            		= 1;    // switch on/off display
} Config;

typedef struct 
{
	float lat;					// City geo location, longitude
	float lon;					// City geo location, latitude

	int weatherId;				// Weather condition id, https://openweathermap.org/weather-conditions
	char condition[32] = {'\n'};	//  Group of weather parameters (Rain, Snow, Extreme etc.)
	char description[32] = {'\n'};	//  Weather condition within the group
	char icon[32] = {'\n'};		// Weather icon id

	float temp;					// Temperature. Unit Default: Kelvin, Metric: Celsius, Imperial: Fahrenheit.
	int pressure;				// Atmospheric pressure, hPa
	int humidity;				// Humidity, %

	float wind_speed;			// m/s
	int visibility; 			// Meter
	int clouds_all; 			// Cloudiness, %

	long dt;					//	Time of data calculation, unix, UTC

	// long cityId;				// City ID
	char city[32] = {'\n'};		// City Name
	char country[4]	= {'\n'};	// Country code (GB, JP etc.)
	long sunrise;				// Sunrise time, unix, UTC
	long sunset;				// Sunset time, unix, UTC
	int timezone;				// Shift in seconds from UTC
	boolean cached;
	char error[64]	= {'\n'};
} OpenWeather;

// https://www.w3schools.com/charsets/ref_utf_dingbats.asp
enum dbgIcons {
	ico_null,
	ico_ok,	 		// \u2705   ✅
	ico_error,		// \u274C   ❌
					// \u274E   ❎
	ico_warning, 	// \u2757   ❗ 
					// \u275B   ❛
	ico_info, 		// \u2771   ❱
	ico_arrow,		// \u2794   ➔
					// \u26AA   ⚪
					// \u26AB   ⚫
	ico_dot			// \u2024
};

// utilities
char*       dbgprint( dbgIcons icon, const char* textbuf );
void        dbgprintln( );
void        dbgprintln( dbgIcons icon, const char* textbuf );
char*       dbgprintf( dbgIcons icon, const char* format, ... );
void 		printConfig(Config _cfg);
const String formatBytes(size_t const &bytes);
char 		*ftoa(char *a, double f, int precision);

// to access funtions in other classes/files
void 		getCurrentTimeString(char *timestamp, const char *format);
Config      getConfig();
int 		getLDRvalue();
bool        saveConfigToJson(Config _cfg);
boolean     reconnect();
boolean     getDisplayState();
void        setDisplayState(boolean state);
int         getDisplayBrightness();
void        setDisplayBrightness(int brightness, boolean useA0, boolean save);
int         getRingBufferIndex();
String      getAllRingBufferItems();
String      getLastopenWeather();
String      getSunsetTime();
String      getSunriseTime();

