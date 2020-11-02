/*

 * firmware.cpp
 *
 *  Created on: 6 juin 2019
 *      Author: Fabriceo
 */

#ifdef XCDUINO
//#include "XCduino.h"
#include "XCScheduler.h"
#include "XCSerial.h"
#include "c++/v1/support/xcore/atomic.h"

/* global variables can be used across task as long as they are marked volatile */
volatile int loopCount;

// softwareserial object instance
SoftwareSerial Serial;

extern "C" {
void XCSerialInit(SERVER_INTERFACE(XCSerial_if, uartIF)){
    Serial.init(uartIF);
    Serial.begin(115200);
} }

usbSerial Serial2;

extern "C" {
void XCSerialUsbInit(SERVER_INTERFACE(XCSerial_if, usbIF)){
    Serial2.init(usbIF);
    Serial2.begin(115200);
} }



void mytask1(){
    int xx;
    int zz = xcore_sync_fetch_and_add (xx, 1);
  printf("enterring ccp task1\n");
  // initialize my local variables or peripherals before enterring in my local loop
  int i=1000;

  // the following yield forces a task switch in order to execute the next task setup, before enterring our own local loop
  yield();
  /* our local loop below. this could be a call to any xc or cpp function called loop1() !
   * doing the loop inside task1 is just a trick to share local variable between the setup part and the loop part... */
  int localSync = micros();
  while (1) { // loop
    unsigned val; GET_SP0(val);

    //printf("loop1 SP = %d, i= %d, millis = %dms\n", val, i, time);
    i++;
    /* the following delay is blocking volontary the CPU, to show the impact
     * on the total yield cycle (printed every seconds in main.xc)
     * and NO impact on the millis value printed, due to yieldSync approach below */
    delayUs(10000); // 10ms
    // waiting max 500ms and transferring the corresponding cpu time to the other tasks
    yieldSyncMs(&localSync, 500);
    // come back here synchronously and precisely every 500ms (+jitter due to cooperative model ...)
    int x = micros(), y = millis(), z = getXCtimer();
    printf("millis = %dms, micros = %dus, timer = %dns\n",y,x,z);
  }
}



extern "C" { // this ensure that the main xc program can access the following functions

int setup(){

    printf("enterring cpp setup()\n");
    // initialise the scheduler with a 1500 byte stack for this main setup function and its loop() below
    if (!XCScheduler.setup(1500)) return 0;
    if (!XCScheduler.start(mytask1,2000)) return 0;

    // global variable initialization for our main loop demo code
    loopCount = 0;

    /* executing this very first yield below will launch all the tasks declared above,
     * one by one, before enterring in our main program loop() function below
     * if not desired, then just remove it, another one exist in the main caller after the loop() call. */
    yield();

    printf("leaving cpp setup()\n");
    return 1; // task initiated properly (there was enough stack space to allocate all stacks
}

/* this loop function can be populated, like in an arduino type of program. by defaut an empty weak loop function exist in XCduino
 * the loop() is called by the .xc main program within the select default statement. see xc main code where the XCduinoTask is declared
 */
void loop(){
    loopCount++;
#if 0
    int val; GET_SP0(val);
    //printstrln("hello");
    printf("loop0 SP = %d, X = %d\n", val,X);
    yieldMs(2000);
#endif
}



} // extern
#endif
