#pragma once

#include <Arduino.h>

/*
 *	// from http://playground.arduino.cc/Main/Utf8ascii
 * 	UTF8-Decoder: convert UTF8-string to extended ASCII 
 */
byte utf8ascii(byte ascii);
String utf8ascii(String s);
void utf8ascii(char* s);
