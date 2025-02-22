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

#if defined( AVDSP_RUNTIME ) && ( AVDSP_RUNTIME > 0 )
#include "lavdsp_runtime.h"
#endif


//base record containing avdsp program key information
avdsp_base_t avdspBase;

unsigned uc_avdsp;          //resource value of the channel used for triggering from audio
unsigned u_avdspif;         //resource value of the channel used for the interface

unsigned avdspSynchronizer;
unsigned avdspReady = 0;


static void tcbInit() { unsafe {
    avdspBase.tcb.runable.ull = 0;
    avdspBase.tcb.runlast.ull = 0;
    avdspBase.tcb.running.ull = 0;
    for (int i=0; i<8; i++) {
        avdspBase.tcb.inf[i].codePtr = 0;
        avdspBase.tcb.inf[i].time = 0;
        avdspBase.tcb.inf[i].addr = (char *)&avdspBase.tcb.running + i; };
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

//used to declare a weak statement within .xc files by patching assembly generated file
#define WEAK(x) asm(".weak " #x ""); asm(".weak " #x ".nstackwords"); asm(".weak " #x ".maxcores"); asm(".weak " #x ".maxtimers"); asm(".weak " #x ".maxchanends")

void avdspInit()                    { WEAK(avdspInit); };
int  avdspSetProgram(int prog)      { WEAK(avdspSetProgram); return 0; };
int  avdspChangeFS(unsigned newFS)  { WEAK(avdspChangeFS); return 0; };

#if defined( AVDSP_RUNTIME ) && ( AVDSP_RUNTIME > 0 )
#define AVDSPTASK(_n) avdsp_rt_task(_n)
#else
void avdspTask1() { WEAK(avdspTask1); };
void avdspTask2() { WEAK(avdspTask2); };
void avdspTask3() { WEAK(avdspTask3); };
void avdspTask4() { WEAK(avdspTask4); };
void avdspTask5() { WEAK(avdspTask5); };
void avdspTask6() { WEAK(avdspTask6); };
void avdspTask7() { WEAK(avdspTask7); };
void avdspTask8() { WEAK(avdspTask8); };
#define AVDSPTASK(_n) avdspTask ## _n()
#endif

static int mainTask(server interface avdsp_if i, chanend caudio){
    asm volatile( "stw r5,dp[avdspSynchronizer]");
    set_core_high_priority_off();
    set_thread_fast_mode_off();
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
                AVDSPTASK(1);
                break;
            }
            default : break;
            }
            break; }

#if defined( AVDSP_RUNTIME ) && ( AVDSP_RUNTIME > 0 )
        case i.loadCodePage(unsigned page, unsigned buf[16]) -> int result : {
            testToken();
            unsigned localBuf[16];
            memcpy(localBuf,buf,sizeof(localBuf));
            result = avdsp_rt_loadCodePage(page, localBuf);
            break; }
#endif

        case i.getInfo(avdsp_info_t info) -> int res : {
            testToken();
            info.tasksLaunched = avdspBase.tasks;
            for (int i=0; i<8; i++) info.time[i] = avdspBase.tcb.inf[i].time;
            res = 0;
            break; }

        case i.start() -> int res : {
            testToken();
            avdspBase.started = 1;
            res = 0;
            break; }

        case i.stop() -> int res : {
            testToken();
            avdspBase.started = 0;
            res = 0;
            break; }

        case i.changeProgram(int newProg) -> int res : {
            testToken();
            avdspSetProgram(newProg);
            avdspBase.program = newProg;
            break; }

        case i.changeFS(int newFS) -> int res : {
            testToken();
            avdspChangeFS(newFS);
            avdspBase.fs = newFS;
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
            AVDSPTASK(2);
            avdspBase.tasks = mainTask(avdspif, caudio);
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 3)
        case 3 : { par {
            AVDSPTASK(2); AVDSPTASK(3);
            avdspBase.tasks = mainTask(avdspif, caudio);
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 4)
        case 4 : { par {
            AVDSPTASK(2); AVDSPTASK(3); AVDSPTASK(4);
            avdspBase.tasks = mainTask(avdspif, caudio);
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 5)
        case 5 : { par {
            AVDSPTASK(2); AVDSPTASK(3); AVDSPTASK(4); AVDSPTASK(5);
            avdspBase.tasks = mainTask(avdspif, caudio);
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 6)
        case 6 : { par {
            AVDSPTASK(2); AVDSPTASK(3); AVDSPTASK(4); AVDSPTASK(5); AVDSPTASK(6);
            avdspBase.tasks = mainTask(avdspif, caudio);
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 7)
        case 7 : { par {
            AVDSPTASK(2); AVDSPTASK(3); AVDSPTASK(4); AVDSPTASK(5); AVDSPTASK(6); AVDSPTASK(7);
            avdspBase.tasks = mainTask(avdspif, caudio);
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 8)
        case 8 : { par {
            AVDSPTASK(2); AVDSPTASK(3); AVDSPTASK(4); AVDSPTASK(5); AVDSPTASK(6); AVDSPTASK(7); AVDSPTASK(8);
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
