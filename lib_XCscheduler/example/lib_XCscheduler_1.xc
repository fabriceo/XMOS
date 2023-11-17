/*

 * testmain.xc
 *
 *  Created on: 8th november 2023
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

unsigned int random = 0; // Seed

unsigned int random_number_gen(unsigned max) {
    if (random == 0) {
        setps(0x060b, 0xF); //switch on ring-oscillators
        delay_microseconds(50);
        setps(0x060b, 0); delay_ticks(10);
        random = getps(0x070b);
    }
    crc32(random,-1,0xEB31D82E);
    unsigned int hi,lo;
    asm("lmul %0,%1,%2,%3,%4,%5":"=r"(hi),"=r"(lo):"r"(random),"r"(max),"r"(0),"r"(0));
    return hi;
}

#define MAX         5000
#define NTASKS      100
unsigned tab[NTASKS];
unsigned numtask = 0;
unsigned quantity = 0;

void onetask(unsigned param, const char name[], unsigned tcb){
    numtask ++;
    printf("enterring %s %d : %d @ %d\n",name, param, numtask, tcb);
    while(1) {
        int rand = random_number_gen(NTASKS);
        if (rand == param) {
            tab[param]++;
            if (tab[param] >= MAX) break;
        }
        quantity++;
        yield();
    }
    numtask--;
   printf("leaving %s %d : %d\n",name, param, numtask);
}

void demorandom() {

    for (int i=0; i<NTASKS; i++) {  //create 100 tasks
        tab[i] = 0;
        XCSchedulerCreateTaskParam(onetask,i);
    }
    int yieldTimer = XC_SET_TIME(100000000);
    do {
        if (XC_END_TIME(yieldTimer)) {
            yieldTimer += 100000000;
            printf("[%d] ",numtask);
            for (int i=0; i<NTASKS; i++) printf("%5d ,",tab[i]);
            printf("\n");
        } else yield();
    } while (numtask);
    printf("done %d random numbers generated\n",quantity);
}

//demo yield : main -> task 1 -> main -> task 2 -> main.

void task(unsigned param, const char name[], unsigned tcb) {
    while(1) {
        printf("%s %d : tcb = %d\n",name, param, tcb);
        delay_microseconds(100000);
        yield();
    }
}


void demoyield(){
    XCSchedulerCreateTaskParam(task,1);
    XCSchedulerCreateTaskParam(task,2);
    unsigned tcb = 0;
    while(1) {
        printf("maintask : tcb = %d\n",tcb);
        delay_microseconds(100000);
        tcb = yield();
    }
}


int main(){

        par {
            { demoyield(); }
            //{ demorandom(); }
        }
    return 0;
}

