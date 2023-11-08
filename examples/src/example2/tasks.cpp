/*

 * firmware.cpp
 *
 *  Created on: 6 juin 2019
 *      Author: Fabriceo
 */

#include "XCScheduler.h"
#include "XCgettime.h"
#include <stdio.h>

/* global variables can be used across task as long as they are marked volatile
 * the yield() mecanism doesnt require use of atomic statements to access them.
 */
volatile int loopCount;
volatile unsigned taskChanend;

EXTERN
void mytask1();  //mandatory but only used to be able to get mytask1.nstackwords
void mytask1() {


  // the following yield forces a task switch in order to execute the next task setup, before enterring our own local loop
  yield();

  /* our local loop below. this could be a call to any xc or cpp function called loop1() for example
   * doing the loop inside task1 is just a trick to share local variable between the setup part and the loop part */
  printf("enterring ccp mytask1 loop\n");
  int prev=0;
  while (1) { // loop
      int loop = loopCount;
      if (loop != prev) {
          prev = loop;
          printf("loopCount changed to %d\n",loop);
          if (loop == 10) {
              printf("*** ending mytask1 definitely ****\n");
              loopCount = -10;
              return;
          }
      }
    yield();
  }
}

EXTERN
void setup(); // this ensure that the main xc program can access it
void setup(){
    int nstackw;
    XCS_GET_NSTACKWORDS(setup, nstackw);
    printf("enterring cpp setup(), requires %d nstackwords\n", nstackw);
    XCS_GET_NSTACKWORDS(mytask1, nstackw);
    printf("allocate cpp mytask1(), requires %d nstackwords\n", nstackw);
    XCSchedulerCreateTask(mytask1);
    // global variable initialization for our main loop demo code
    loopCount = 0;
    printf("leaving cpp setup()\n");
}

/* this loop function can be populated, like in an arduino type of program. by defaut an empty weak loop function exist in XCduino
 * the loop() is called by the .xc main program within the select default statement. see xc main code where the XCduinoTask is declared
 */
EXTERN
void loop();
void loop(){
    while(loopCount >= 0) {
    yieldDelay(20000000);//every 200ms we come back below
    loopCount++;
    }
    printf("*** ending loop() definitely ****\n");
}
