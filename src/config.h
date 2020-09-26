/*
 *
 */
#pragma once

#include <Arduino.h>
#include "utf8ascii.h"
#include <FS.h> // SPIFFS is declared
// #include "LittleFS.h" // LittleFS is declared
// #define SPIFFS LittleFS

#define CORE_VERSION                "3.1.0"
#define SW_VERSION                  "3.0.4"


// https://arduinojson.org/v6/api/config/use_double/
// use before #include <ArduinoJson.h>
#define ARDUINOJSON_USE_DOUBLE      1

#ifdef GHAUS_INCLUDES
#include "defines_ghausdisplay.h"
#else
#include "defines_infodisplay.h"
#endif

#define ON      true
#define OFF     false
