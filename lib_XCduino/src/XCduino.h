/*
 * lib.h
 *
 *  Created on: 6 juin 2019
 *      Author: Fabriceo
    an attempt to provide a libray header and programs
    to bring arduino style in the xmos programing environement
 */

#ifndef XCDUINO_H_
#define XCDUINO_H_

/* this trick below defines the EXTERNAL prefix and make it compatible either cpp or xc
 * credits feniksa here : https://www.xcore.com/viewtopic.php?t=3017
 */
#ifndef EXTERNAL
#ifdef __XC__
# define EXTERNAL extern
#else
# ifdef __cplusplus
#  define EXTERNAL extern "C"
# else
#  define EXTERNAL
# endif // __cplusplus
#endif // __XC__
#endif // ndef EXTERNAL


#if defined(__XS1B__) || defined(__XS2A__) || defined(__XS3A__)
#include <XS1.h>
#include <xccompat.h>
#endif
#include <stdint.h>
// #include "XCprint.h" // this is the standard XC print
#include <stdio.h> // this is standard printf scanf stuff

// lets include the digitalRead & write function which are used by everyone
//#include "XCport.h"

/* the below includes comes from xmos libray and they are not c++, so we encapsule them within extern "C" {}
 */
#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"

#ifdef __cplusplus
}
#endif

/* all defines or functions below are compatibles with cpp or xc langage
 * they are based or coming from an orginal arduino.h file
 * this does not garantee any full compatibility but makes platform transition easy
 */

#define PROGMEM
#define PGM_P(str) const ( char *)(str)
#define PSTR(str) (str)

#define pgm_read_byte(x) (uint8_t)(*(x))
#define pgm_read_word(x) (uint16_t)(*(x))

typedef  unsigned char byte;

#define HIGH 0x1
#define LOW  0x0

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define SERIAL  0x0
#define DISPLAY 0x1

#define LSBFIRST 0
#define MSBFIRST 1

#define CHANGE 1
#define FALLING 2
#define RISING 3


#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
//#define abs(x) ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

#define clockCyclesPerMicrosecond() ( 100 )
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )
#define microsecondsToClockCycles(a) ( (a) * clockCyclesPerMicrosecond() )

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#define bit(b) (1UL << (b))

// avr-libc defines _NOP() since 1.6.2
#ifndef _NOP
#define _NOP() do { __asm__ volatile ("nop"); } while (0)
#endif

/** very usefull to access memory used by other xcore tasks in xc langage ... */
#ifndef GET_SHARED_GLOBAL
#define GET_SHARED_GLOBAL(x, g) asm volatile("ldw %0, dp[" #g "]":"=r"(x)::"memory")
#endif
#ifndef SET_SHARED_GLOBAL
#define SET_SHARED_GLOBAL(g, v) asm volatile("stw %0, dp[" #g "]"::"r"(v):"memory")
#endif
/** get the instant value of the SP stack pointer register, x parameter can be local or global variable */
#ifndef GET_SP0
#define GET_SP0(x)   asm volatile("ldaw %0, sp[0]":"=r"(x)::"memory")
#endif

/*
 * typedef void (*voidFuncPtr)(void);
 */

/** time handling basic arduino functions existin in XCduino.xc or .cpp*/
EXTERNAL int micros();
EXTERNAL int millis();
EXTERNAL void delay(int ms);
EXTERNAL void delayMicroseconds(int us);
EXTERNAL void delayUs(int us);
EXTERNAL void yieldMs(int ms);
EXTERNAL void yieldUs(int ms);
EXTERNAL void yieldSyncUs(REFERENCE_PARAM(int,localTimer), int us);
EXTERNAL void yieldSyncMs(REFERENCE_PARAM(int,localTimer), int ms);
EXTERNAL int getXCtimer();
EXTERNAL void XCtimerReset();

/** coming from XCduino.cpp */
EXTERNAL void yield();
EXTERNAL void init();
EXTERNAL void initVariant();
EXTERNAL int  setup();
EXTERNAL void loop();

#ifdef __XC__
extern timer XCtimer;
#endif


#endif /* XCDUINO_H_ */
