/*
 * XCduino.cpp
 *
 *  Created on: 6 juin 2019
 *      Author: Fabrice
 */


#include "XCduino.h"

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

