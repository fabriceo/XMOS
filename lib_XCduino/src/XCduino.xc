/*
 * lib.xc
 *
 *  Created on: 6 juin 2019
 *      Author: Fabriceo
 *      this file contains procedure needed for providing some sort of arduino compatibility
 *      they are writen in the xc langage section as they use xc resources (timer or port).
 */

#include "XCduino.h"

timer    XCtimer; // incremented every 10 nanoseconds. roll over every 43 seconds!
unsigned XCtimerPrev;
unsigned XCtimerHi = 0; // incrementd by software every 22 seconds
int XCmicros = 0;
int XCmillis = 0;


#define MUL_2P32_DIV_100    42949673
#define MUL_2P40_DIV_100000 10995116


/* this function provide the XCtimer counter and update the global variable XCmicros and XCmillis
 * this works ok only if this function is called at least every 10 seconds or so...
 */
int getXCtimer() {
    unsigned time;
    asm volatile ("gettime %0":"=r"(time));
    if (time < XCtimerPrev ) {
            // transition from 1 to 0 of bit 31 means a carry is to be considered
            XCtimerHi ++; }
    XCtimerPrev = time;

    return XCtimerPrev;
}

void XCtimerReset(){
    asm volatile ("gettime %0":"=r"(XCtimerPrev));
    XCtimerHi = 0;
    XCmicros  = 0;
    XCmillis  = 0;
}

int micros(){
    //compute the timer value
    getXCtimer();
    unsigned mac_h, mac_l, hi, lo;
    {mac_h, mac_l} = lmul(XCtimerPrev, MUL_2P32_DIV_100, 0, 0);
    {hi, lo }      = lmul(XCtimerHi,   MUL_2P32_DIV_100, 0, 0);
    XCmicros = lo + mac_h;
    return XCmicros;
}



/*
 * this function returns a 32 bits value representing the generic timer divided by 100 000
 * to provide an absolute time in miliseconds.
 * due to 32 bits original size, the counter will roll off avec 42days...
 * note the division is replaced by a multiply instruction,
 * but taking only the 32 higher bits of the 64 bits result
 */
int millis(){
    //compute the timer value
    getXCtimer();
    unsigned mac_h, mac_l, hi, lo;
    lo = ((XCtimerHi & 0xFF) << 24) + (XCtimerPrev >> 8);
    hi = (XCtimerHi >> 8);
    {mac_h, mac_l} = lmul(lo, MUL_2P40_DIV_100000, 0, 0);
    {hi, lo}       = lmul(hi, MUL_2P40_DIV_100000, 0, 0);
    XCmillis = lo + mac_h;
    return XCmillis;
}


/*
 * this is a fully blocking function based on the generic timer runing at 100mhz
 * and the xc standard approach
 */
void delayMicroseconds(int us){
    int time, end;
    asm volatile ("gettime %0":"=r"(time));
    end = time + us*100L;
    do {
        asm volatile ("gettime %0":"=r"(time));
    } while(time < end);
}

inline void delayUs(int us){
   delayMicroseconds(us);
}

/* * delay in microseconds with embedded yield
 */
void yieldUs(int us){
   int time = micros();
   us += time;
   while ( (us - time) > 0) {
          yield();
          time = micros(); }
}


/* delay in Miliseconds with embedded yield
 * use yieldUs() function for better precision
 */
void yieldMs(int ms){
    yieldUs(ms * 1000);
}



/* * delay in microseconds with embedded yield
 * the time spent here will be based on the previous call
 * to this method and so fully synchronous.
 */
void yieldSyncUs(REFERENCE_PARAM(int,localTimer), int us){
   int time = micros();
   if (localTimer == 0) { localTimer = time; }
   localTimer += us;
   while ( (localTimer - time) > 0) {
          yield();
          time = micros(); }
}

void yieldSyncMs(REFERENCE_PARAM(int,localTimer), int ms){
   yieldSyncUs(localTimer, ms*1000);
}
