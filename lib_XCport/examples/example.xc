/*
 * main.xc
 *
 *  Created on: 28 juin 2019
 *      Author: Fabrice
 */

#include "XCport.h"
#include <XS1.h>

// change the below value to switch the other code examples.
#define EXAMPLE0
/*
#include "xscope.h"
void xscope_user_init(void) {
    xscope_register(0, 0, "", 0, "");
    xscope_config_io(XSCOPE_IO_TIMED); }
*/


#ifdef EXAMPLE0

void test(port p1, port ?p2){
    while(1) {
        select {
            case p1 when pinseq(0x451) :> int _:
                break;
            case (!isnull(p2)) => p2 when pinseq(0x54) :> int _:
                break;
        }
    }
}

// this is the official way of handling ports
// very siple and very efficient!
on tile[0] : port p1 = XS1_PORT_1L;
on tile[0] : port p2 = XS1_PORT_1K;
int main(){
    while (1) {

        test(p1, p2);
    }
    return 0;
}
#endif

/******************************************************************************
 * this is the most basic example with XCport
 */
#ifdef EXAMPLE1

void taskIO1(){
    XCportStep(0); // initializing lock
    XCPORT_PRINTF("Entering taskIO1\n");
    pinMode( P1L , OUTPUT );
    pinMode( P8C,  OUTPUT_PULLUP );
    pinMode( P8C2, INPUT_PULLUP);
    digitalWrite(P8C, 0xF0);
    XCportStep(1); // authorize other task in step2
    XCportStep(4); // wating e
    XCPORT_PRINTF("continuing taskIO1\n");
    int tog=0, i;
    timer t;
    int time;
    t :> time; time+= 10000; // 100 micro seconds
    do {
        select {
            case t when timerafter(time) :> void:
                time += 10000;
                    digitalWrite( P1L, tog);
                    tog=1-tog;
                    digitalWrite(P8C6, tog);
                    i = digitalRead(P8C2); // using peek on P8C
                    i = digitalRead(PORT_MEM(P8C)); // read port value stored in memory
                break;
        }
    } while(0);
    XCPORT_PRINTF("end of task IO1\n");
    XCportStep(5);
}

void taskIO2(){
    XCportStep(2); // waiting step 1 and moving to step 2
    XCPORT_PRINTF("Entering taskIO2\n");
    enum listPin { myPin = P4E3 };
    pinMode( myPin, OUTPUT ); // => same as p8c7
    XCportStep(3);// end of step 2 authorizing step 3
    XCportStep(5);
    XCPORT_PRINTF("continue taskIO2\n");
    do {
        digitalWrite(P4E, PORT_XOR(0x08)); // togle p4E3
        digitalWrite(myPin, 1);
    } while(0);
}


int main() {
par {
    taskIO1();
    taskIO2();
    }
    return 0;
}

#endif // example1


#ifdef EXAMPLE2


extern void toggleLED(); // in cpp

void taskReadPin(unsigned pin) {
    int tog = 0;
     XCPORT_PRINTF("taskReadPin %d\n", pin);
     while(1) {
        tog = digitalRead(pin);
     }
}


void togglePin( unsigned pin) {
    int tog = 0;
    XCPORT_PRINTF("togglePin pin %d\n", pin);
    while(1) {
        digitalWrite(pin, tog); tog = 1-tog;
    }
}

void togglePort(unsigned po) {
    int tog = -1;
    XCPORT_PRINTF("togglePort %d\n", po);
    while(1) {
        digitalWrite(po, PORT_XOR(tog));
    }
}


// this is a convenient way to declare each of the IO with a nick name
// just list them all in some enum statement, using predefined names and macros
enum myXCpins {

    led = PORT_ON_TILE0( P1L0 ),
    // led = P1L0_0, // same as above
    pc8pin6 = P8C6_0, // forced on tile 0
    pc8all = P8C };

enum myXCports {

    p8Ctile0 = P8C_0,
    p8Ctile1 = P8C_1,
    p8C_on_Tile_1 = PORT_ON_TILE(P8C,1),
    p8C_on_Tile1  = PORT_ON_TILE1(P8C)
    };

// this is a convenient solution to list some ports, plain or bit,
// for bulk initialization by calling XCportModeList(name)
XCPORT_MODE(myBasePinConfig) = {
    P1L,    OUTPUT,
    P8C_0,  PORT_SHARED(OUTPUT),
    P8D,    OUTPUT_SHARED,
    0 }; // mark the end of table, do not forget it!

XCPORT_MODE(myOtherPinConfig) = {

    0 }; // mark the end of table, do not forget


#define baseTile  0             // used for trying the cross tile mechanisms
#define otherTile 1-baseTile

int main() {
    XCPORT_TILE_IF(baseTileIF)[2]; // declare 2 interface for handling cross tile I/O

    XCPORT_TILE_IF(otherTileIF)[1]; // declare an interface for handling cross tile I/O

    par
    {   /*** TASKS ON THE BASE TILE ***********/

    on tile[baseTile] : // this can be considered as the main task on this tile so we initialize our library instance
        {   XCportInitTileIF(baseTileIF[0],0); // initialize XCport library with possibility to call interfaces
            XCportModeList(myBasePinConfig);        // bulk initialization of the pins on this tile
            XCportStep(1); // step1 done, release step 2
            XCportStep(4); // wait end of step 3 so the second task can complete extra initialization
            togglePin( pc8pin6 ); // the task is launched nearly at same time as the second task
        }

    on tile[baseTile] :
        {   XCportInitTileIF(baseTileIF[1],1);
            XCportStep(2); // this is a way to wait the end of the setup done in previous task/step1
            pinMode(P8A_0, INPUT_PULLUP); // continue our setup
            XCportStep(3); // wait step 2 (obviously ok) aand now authorize step 4!
            taskReadPin(  P8A_0 );
        }
    on tile[baseTile]  : // this task will handles all port access on baseTile requested by any core of the "otherTile"
       XCportTileTask(otherTileIF,1); // provide a server interface to access the digitalRead/write method inside baseTile

    /*** TASKS ON THE OTHER TILE ***********/


    on tile[otherTile] :
    {   XCportInitTileIF(otherTileIF[0],0); // initialize XCport library with possibility to call interfaces
        XCportModeList(myOtherPinConfig);        // bulk initialization of the pins on this tile
        XCportStep(1); // step1 done, autorize step2!

        togglePin(pc8pin6 );
    }

   // this task handles a server interface for sharing I/O requested from "baseTile"
    on tile[otherTile] : XCportTileTask(baseTileIF,2); // provide a server interface to access the digitalRead/write method inside "otherTile"



    }
    return 0;
}

#endif // example2



#ifdef EXAMPLE3
/*
 *
 */

int main() {


    return 0;
}
#endif // example3



#ifdef EXAMPLE4
/*
 *
 */
#define setc(a,b)    {__asm__  __volatile__("setc res[%0], %1": : "r" (a) , "r" (b));}
#define portin(a,b)  {__asm__  __volatile__("in %0, res[%1]": "=r" (b) : "r" (a));}
#define portout(a,b) {__asm__  __volatile__("out res[%0], %1": : "r" (a) , "r" (b));}
#define portpeek(a,b){__asm__  __volatile__("peek %0, res[%1]": "=r" (b) : "r" (a));}


on tile[0] : port myp = XS1_PORT_8C;


void init(){
    portSetup(XS1_PORT_8C, OUTPUT, 0xFF);
}

void taskIO1(){
    int tog = 0;
    int time;
    timer t;
    init();
    t :> time;
    while(1){
        setc(XS1_PORT_8C, XS1_SETC_DRIVE_DRIVE);
        portout(XS1_PORT_8C, tog);
        time += 100000000;
        t when timerafter(time) :> void;
        tog = 64-tog;
    }
}

void taskIO2(){
    int i;
    int time;
    timer t;
    init();
    t :> time; time+=50000000; // 500ms
    t when timerafter(time) :> void;
    while(1) {
        select {
            case myp when pinsneq(0) :> int re:
                XCPORT_PRINTF("change %d\n",res);
                break;
        }
        portout(XS1_PORT_1L, 1);

    }

}


int main() {

    par {
        on tile[0] : {
            taskIO1();
        }
        on tile[0] : taskIO2();
    }

    return 0;
}
#endif // example3


/*****************************************
on tile[0] : out port pp = XS1_PORT_8C;

void flashHard(){
    timer t;
    int time, tog=0;
    t :> time; time += 50000000; // half a ms
    while(1) {
       t when timerafter(time) :> void;
       time += 50000000;
       printf("tog %d\n",tog);
       pp <: (tog << 6);
       tog = 1-tog;
    }
}
*/

