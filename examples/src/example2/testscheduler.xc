/*

 * testmain.xc
 *
 *  Created on: 11 juin 2019
 *      Author: Fabriceo
 */

#include <XS1.h>
#include <stdio.h>
#include "XCScheduler.h"

#ifdef XSCOPE
#include <xscope.h>
void xscope_user_init()
{   xscope_register(0, 0, "", 0, "");
    xscope_config_io(XSCOPE_IO_BASIC);  }   // or XSCOPE_IO_TIMED or XSCOPE_IO_BASIC
#endif

EXTERN void setup();    //from cpp file, arduino style
EXTERN void loop();     //from cpp file, arduino style
EXTERN void mytask1();  //from cpp file, arduino style

void cooperativeTask(const int th, const char name[]){
    printf("Entering %s %d\n",name,th);
    // create cpp tasks only in first thread. showcase possibility to create task from anywhere
    if (th == 1) setup();
    int n=0;
    const unsigned period = 50000000; //0.5sec
    printf("while(1) %s %d\n",name,th);
    int timeRef = XC_SET_TIME(period);
    int delta;
    while(1) {
        while ( ! (delta = XC_END_TIME(timeRef)) ) yield();
        timeRef += period;
        n++;
        printf("%s %d [ %d ] deviation %d\n", name, th, n, delta);
        if ((th==1) && (n==25)) {
            printf("*** ending %s %d after %d ***\n", name, th, n);
            return;
        }
    }
}

void poolChanend(streaming chanend c, const char name[]){
    printf("Entering %s\n",name);
    while(1) {
    if ( XCStestStreamingChanend(c) ) {
        int cc; c :> cc;
        printf("received chanend in %s value = %d\n", name, cc);
        //c <: cc+1;    //send back
    }
    yield();
    }
}


void XCthread(const int th, streaming chanend c) {
    printf("Entering XCthread %d\n",th);
    //create a cooperative task to be executed within this XC thread
    XCSchedulerCreateTaskParam( cooperativeTask, th );
    //initialize poolchanend task from thread 2. unsafe used due to type casting of streaming chanend to unsigned
    //if (th == 2) unsafe { XCSchedulerCreateTaskParam( poolChanend, (unsigned)c ); }
    const unsigned period = 100000000;
    int n = 0;
    timer t;
    unsigned tnext;
    t :> tnext;
    if(th==2) tnext+=10000000; //100ms later for thread2
    tnext += period; //1 sec
    printf("Entering XCthread %d loop\n",th);
    asm volatile("duration:");
    while (1) {
        select {
            case t when timerafter(tnext) :> void: tnext += period;
                n++;
                printf("XCthread %d count %d\n",th,n);
                //if ((th==1)&&((n&1)==1)) { c <: n; }
                break;
         /*   case (th==1) => c :> int cc:
                printf("XCthread %d received chanend %d\n",th,cc);
                break; */
            default:
                //the thread is now delegating time to all task in its list
                yield();
                break;
        }
    }
}

int main(){
    streaming chan c;
        par {
            { asm volatile("#XCthread1:"); XCthread( 1, c ); }
            { asm volatile("#XCthread2:"); XCthread( 2, c ); }

        }
    return 0;
}

