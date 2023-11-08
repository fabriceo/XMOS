/*
 * Rotary encoder library for Arduino.

 * Copyright 2011 Ben Buxton. Licenced under the GNU GPL Version 3.
 * Contact: bb@cactii.net
 * modified by fabriceo june 23th 2019
 * from original files here :
 * https://github.com/buxtronix/arduino/tree/master/libraries/Rotary
 */

#ifndef rotary_h
#define rotary_h

#include "XCduino.h"

// Enable this to emit codes twice per step.
//#define HALF_STEP


// Values returned by 'process'
// No complete step yet.
#define DIR_NONE 0x0
// Clockwise step.
#define DIR_CW 0x10
// Anti-clockwise step.
#define DIR_CCW 0x20

class Rotary
{
  public:
    Rotary(int, int);
    Rotary();
    // Process pin(s)
    int process();
    int process(int, int);
  private:
    int state;
    int pin1;
    int pin2;
};

#endif
