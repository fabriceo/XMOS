/*

 * main.xc
 *
 *  Created on: 11 juin 2019
 *      Author: Fabriceo
 */
#ifdef XCDUINO
/* many usefull declarations provided inside xcduino, including print, stdio... */
#include "XCduino.h"
/* including the xcscheduler as it is not embedded by default in the xcduino header, for portability reason */
#include "XCScheduler.h"
#include "XCSerial.h"
#include "I2CSlave.h"
#include "xud_cdc.h"

interface XCduino_if {
    [[clears_notification]] int getData();
    void sendData(int x);
    [[notification]] slave void dataReady(void); };

/*
 * this task will run on tile0 and is used to read/write the tile 0 ports,
 * potentially control I2C master, and
 * for the interraction with Audio() task in charge of I2S handling.
 */
void TaskTile0(server interface XCduino_if myIF){

    while (1){
        select {
            case myIF.getData() -> int res:
                res = 1;
                break;
            case myIF.sendData(int x):
                printf("INTERFACE client sendData %d\n",x);
                break;
        } // select
    } // while
}

extern void XCSerialInit(unsigned uartIF); // used to pass the Interface ressource adress to the cpp code
/*
 * this is the xc task to run the arduino style setup, loop and cpp tasks
 * it will initialize the scheduler with a maximum stack size
 * (same as pragma value) and will start executing the setup function and each tasks.
 * a loop function can be called, other wise just a yield statement to switch context quickly
 * this task can be launched by the main application of usb-audio-app by putting it
 * in the USER_MAIN_CORE deined in the user_main.h file
 */
#pragma stackfunction 2500 // 2500 words 4 bytes long = 10000bytes
void XCduinoTask(
                 //client interface XCduino_if myIF,
                 client interface XCSerial_if ?uartIF,
                 client interface i2cSlave_if i2cIF
#ifdef CDC
               , client interface usb_cdc_interface usbcdcIF
#endif
        ){
    int wdtTime;
    int yieldPrev, yieldMax;
    /** initialize the scheduler with the total stack size allowed, identical to the praga statement **/
    XCSchedulerInit(2500*4);
    unsafe {
        XCSerialInit((unsigned) uartIF); }
    XCtimerReset(); // it is our choice to consider absolute time = 0 when enterring here
    // the code below call the setup cpp function which has access to the XCScheduler methods writen in cpp.
    i2cIF.regWrite(4,0);
    int tmp=1;
    tmp = setup();
    if (tmp) {
        XCtimer :> wdtTime;
        yieldPrev = wdtTime;
        wdtTime += 100000000; // 1second
        yieldMax = 0;
        while(1){
            select {
            // event driven approach, as usual when using select in xc langage, but not real time as there is a "default" case below!
            case XCtimer when timerafter(wdtTime) :> void:
                wdtTime += 100000000;
                printf("almost here every seconde, yieldMax = %dus\n", yieldMax/100);
                // myIF.sendData(yieldMax/100);
                /* reinitialize yield time measurement */
                yieldMax = 0; XCtimer :> yieldPrev;
                break;
            default:
#ifdef CDC
                // directly transfert USB-CDC with physical UART
                while (usbcdcIF.available_bytes()) {
                    uartIF.writeChar(usbcdcIF.get_char());  }
                while (uartIF.numCharIn()) {
                    usbcdcIF.put_char(uartIF.readChar());  }
#endif
#if 0 // launch the loop if needed but then the event above will not triger if the loop is "blocking" somehow
                loop();
                XCtimer :> yieldPrev; // needed if we just want to measure the yieldCycle time of the other tasks only
#endif
                yield(); // organize a complete cycle of switching all task initialized
                int delta = yieldPrev;
                XCtimer :> yieldPrev;;
                delta = yieldPrev - delta;
                if (delta > yieldMax) { yieldMax = delta; }
                break;
            } // select
        } // while 1
        }
    printf("probleme launching setup() or associated tasks");
}
#endif
