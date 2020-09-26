#ifndef WETIMER_H_INCLUDED
#define WETIMER_H_INCLUDED
/*
 *
 */
#include "WeTimer.h"

WeTimer::WeTimer(){
	this->interval_millis = 1000;
}

WeTimer::WeTimer(unsigned long interval_millis){
	this->interval_millis = interval_millis;
}

void WeTimer::setInterval(unsigned long interval_millis){
	this->reset();
	this->interval_millis = interval_millis;
}

unsigned long WeTimer::getInterval(){
	return this->interval_millis; 
}

uint8_t WeTimer::check(){

  unsigned long now = millis();
  
  if ( interval_millis == 0 ){
	previous_millis = now;
	return 1;
  }
 
  if ( (now - previous_millis) >= interval_millis) {
	previous_millis += interval_millis ; 
	return 1;
  }
  
  return 0;
}

void WeTimer::reset() {
  this->previous_millis = millis();
}

#endif // WETIMER_H_INCLUDED