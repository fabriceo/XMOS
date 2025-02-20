/*
 * lavdsp.xc
 *
 *  Created on: 17 févr. 2025
 *      Author: Fabriceo
 */

#include <xs1.h>
#include "lavdsp.h"
#include "debug_print.h"
#include <string.h>     //requires memcpy


//base record containing avdsp program key information
avdsp_base_t avdspBase;

unsigned uc_avdsp;          //resource value of the channel used for triggering from audio
unsigned u_avdspif;         //resource value of the channel used for the interface

unsigned avdspSynchronizer;
unsigned avdspReady = 0;

static unsigned codeSize;

#ifndef AVDSP_RUNTIME_SIZE
#define AVDSP_RUNTIME_SIZE (1024) /* default size for the dsp opcodes in words */
#endif

unsigned long long avdspBuf[AVDSP_RUNTIME_SIZE/2];

static lavdp_tcb_t tcb;

static void tcbInit() { unsafe {
    tcb.runable.ull = 0;
    tcb.runlast.ull = 0;
    tcb.running.ull = 0;
    for (int i=0; i<8; i++) {
        tcb.inf[i].codePtr = 0;
        tcb.inf[i].time = 0;
        tcb.inf[i].addr = (char *)&tcb.running + i; };
} }



static inline int getTime() {
    int res;
    asm volatile("gettime %0" : "=r"(res));
    return res;
}

#pragma select handler
static inline void testct_byref(chanend c, int &returnVal)
{
    if(testct(c))   returnVal = 1;
    else            returnVal = 0;
}

/*
#pragma select handler
static inline void chkct_gettime(chanend c, int &returnVal) {
    asm volatile("{ chkct res[%1],1 ; gettime %0 }":"=r"(returnVal):"r"(c));
}
*/


static void testToken(){
    avdspReady = 0;
    asm volatile("nop;nop;nop;nop;nop;nop;nop":::"memory");
    //verify if the mainTask has been triggered by audio tasks
    if (avdspReady == 2) {
        avdspReady = 0;
        //capture token
        char ct;
        unsafe { ct = inct((chanend)uc_avdsp); }
        //treat ct value to cleanup channel
    }
}


void avdspTask1() { AVDSP_WEAK(avdspTask1);};
void avdspTask2() { AVDSP_WEAK(avdspTask2);};
void avdspTask3() { AVDSP_WEAK(avdspTask3);};
void avdspTask4() { AVDSP_WEAK(avdspTask4);};
void avdspTask5() { AVDSP_WEAK(avdspTask5);};
void avdspTask6() { AVDSP_WEAK(avdspTask6);};
void avdspTask7() { AVDSP_WEAK(avdspTask7);};
void avdspTask8() { AVDSP_WEAK(avdspTask8);};
void avdspInit()  { AVDSP_WEAK(avdspInit);};
int  avdspSetProgram(int prog) { AVDSP_WEAK(avdspSetProgram); return 0; };
int  avdspChangeFS(unsigned newFS) { AVDSP_WEAK(avdspChangeFS); return 0; };



static int mainTask(server interface avdsp_if i, chanend caudio){
    asm volatile( "stw r5,dp[avdspSynchronizer]");
    int tasksRequired = avdspBase.tasks;
    while (1) {
        int ctval;
        if (tasksRequired != avdspBase.tasks) return tasksRequired;
        avdspReady = 1;
        select {
        case testct_byref(caudio, ctval) : {
            avdspReady = 0;
            switch (ctval) {
            case XS1_CT_END : {
                //core execution
                avdspTask1();
                break;
            }
            default : break;
            }
            break; }

#if defined( AVDSP_RUNTIME ) && ( AVDSP_RUNTIME > 0 )
        case i.loadCodePage(unsigned page, unsigned buf[16]) -> int result : {
            testToken();
            static int size = 0;
            static int nextPage = 0;
            result = -1; // error by default
            if (page == nextPage) {
                if (size == 0) {
                    //dspResetProg();
                    codeSize = 0;
                }
                if ( ( size + page*16 ) < AVDSP_SIZE ) {
                    memcpy(avdspBuf, buf, 16*4);
                    size += 16; nextPage = page + 1;
                    result = 0; // success
                 }
            } else
            if (page == 0) {
                //end of process : check what was loaded
                codeSize = size; result = 0;
            }
            break; }
#endif

        case i.getInfo(avdsp_info_t info) -> int res : {
            testToken();
            info.tasksLaunched = avdspBase.tasks;
            for (int i=0; i<8; i++) info.time[i] = tcb.inf[i].time;
            res = 0;
            break; }

        case i.start() -> int res : {
            testToken();
            res = 0;
            break; }

        case i.stop() -> int res : {
            testToken();
            res = 0;
            break; }

        }
    }
    return 1;   //never happens
}


// main task in charge of dispatching interfaces requests
// or executing main DSP task triggerd by a channend token from audio task.
void lavdspMain(server interface avdsp_if avdspif, const int tasksMax) {
    chan caudio;
    tcbInit();
    asm volatile ("#lavdspMain:");
    unsafe {
        u_avdspif = (unsigned)avdspif;
        uc_avdsp = (unsigned) caudio; }
    avdspBase.tasksMax = tasksMax;
    avdspBase.tasks = 1;
    avdspInit();
    while(1) {
        if (avdspBase.tasks > tasksMax) {
            debug_printf("lavdsp : not enough cores (%d) to launch expected tasks (%d)\n",tasksMax,avdspBase.tasks);
            avdspBase.tasks = tasksMax;
        }
        switch (avdspBase.tasks) {
        case 1 : { avdspBase.tasks = mainTask(avdspif, caudio); break; }
#if (AVDSP_TASKS_MAX >= 2)
        case 2 : { par {
            avdspTask2();
            avdspBase.tasks = mainTask(avdspif, caudio);
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 3)
        case 3 : { par {
            avdspTask2();
            avdspTask3();
            avdspBase.tasks = mainTask(avdspif, caudio);
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 4)
        case 4 : { par {
            avdspTask2();
            avdspTask3();
            avdspTask4();
            avdspBase.tasks = mainTask(avdspif, caudio);
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 5)
        case 5 : { par {
            avdspTask2();
            avdspTask3();
            avdspTask4();
            avdspTask5();
            avdspBase.tasks = mainTask(avdspif, caudio);
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 6)
        case 6 : { par {
            avdspTask2();
            avdspTask3();
            avdspTask4();
            avdspTask5();
            avdspTask6();
            avdspBase.tasks = mainTask(avdspif, caudio);
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 7)
        case 7 : { par {
            avdspTask2();
            avdspTask3();
            avdspTask4();
            avdspTask5();
            avdspTask6();
            avdspTask7();
            avdspBase.tasks = mainTask(avdspif, caudio);
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 8)
        case 8 : { par {
            avdspTask2();
            avdspTask3();
            avdspTask4();
            avdspTask5();
            avdspTask6();
            avdspTask7();
            avdspTask8();
            avdspBase.tasks = mainTask(avdspif, caudio);
            } break; }
#endif
        }
    }
}




extern "C" {
#if defined( AVDSP_RUNTIME ) && ( AVDSP_RUNTIME > 0 )
int avdsp_loadPage(client interface avdsp_if i,unsigned page, unsigned buf[16]) {
    return i.loadCodePage(page,buf);
}
#endif
int avdsp_getInfo(client interface avdsp_if i, avdsp_info_t info) {
    return i.getInfo(info);
}
int avdsp_start(client interface avdsp_if i) {
    return i.start();
}
int avdsp_stop(client interface avdsp_if i) {
    return i.stop();
}
}
