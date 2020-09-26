#pragma once

#ifndef WeTimer_h
#define WeTimer_h

#include "Arduino.h"

class WeTimer {
  public:
	WeTimer();
	WeTimer(unsigned long interval_millis);
	void setInterval(unsigned long interval_millis);
	unsigned long getInterval();
	uint8_t check();
	void reset();

  private:
	unsigned long  previous_millis, interval_millis;
};

#endif