/*
 * lib.xc
 *
 *  Created on: 6 juin 2019
 *      Author: Fabriceo
 *      this file contains procedure needed for providing some sort of arduino compatibility
 *      they are writen in the xc langage section as they use xc resources (timer or port).
 */

#include "XCduino.h"
#include "XCgettime.h"

void delayMicroseconds(int us){
    int time = XC_SET_TIME( clockCyclesPerMicrosecond * us );
    while (! XC_END_TIME(time)) { };
}

void delayUs(int us){
   delayMicroseconds(us);
}


void delay(int ms){
    int time = XCmillis() + ms;
    while ( (time - XCmillis() ) > 0 ) { yield(); }
}


// Declared weak in Arduino.h to allow user redefinitions.
int atexit(void (* /*func*/ )()) { return 0; }

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }


void init() __attribute__((weak));
void init() { }

void yield() __attribute__((weak));
void yield() { }

int setup() __attribute__((weak));
int setup() { return 0; }

void loop() __attribute__((weak));
void loop() { yield(); }

