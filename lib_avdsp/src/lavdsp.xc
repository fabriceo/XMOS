/*
 * lavdsp.xc
 *
 *  Created on: 17 févr. 2025
 *      Author: Fabriceo
 */

#include <xs1.h>
#include "lavdsp.h"         //all the interfaces and type definitions
#include "lavdsp_weak.h"    //definition of the weak procedure to be overloaded by the user program
#include "debug_print.h"
#include <string.h>     //requires memcpy

#if defined( AVDSP_RUNTIME ) && ( AVDSP_RUNTIME > 0 )
#include "lavdsp_runtime.h"
#endif


//base record containing avdsp key informations
avdsp_base_t avdspBase;
//this second symbol will be used when accessing data from audio task
extern avdsp_base_t baseAudio;
//this third symbol will be used locally by the maintask
extern avdsp_base_t baseLocal;
//this fourth symbol will be used by each other dsp tasks
extern avdsp_base_t baseTasks;

unsigned u_avdspif;         //resource address of the channel used for the interface

//provide a fixed delay without scheduling the task (mips spread to others)
static inline int timerafterTicks(const int ticks) {
    timer tmr;
    int time;
    tmr :> time;
    tmr when timerafter(time+ticks) :> time;
    return time;
}


#pragma unsafe arrays
static void tcbInit() { unsafe {
    baseLocal.tcb.runable.ull = 0;
    baseLocal.tcb.runlast.ull = 0;
    baseLocal.tcb.running.ull = 0;
    baseLocal.tcb.ready       = 0;
    baseLocal.tcb.timeLaunch  = 0;
    for (int i=0; i<8; i++) {
        baseLocal.tcb.inf[i].codePtr = 0;
        baseLocal.tcb.inf[i].time = 0;
        baseLocal.tcb.inf[i].addr = (char *)&baseLocal.tcb.running + i; };
} }


//in transfer mode 2, test the channel for a token presence and extract it,
// otherwise return 0, meaning a 32 bits data is pending
// in mode 0 or 1, just wait for and extract a "CT_END" token
#pragma select handler
static inline void testGetToken(chanend c, int &returnVal) {
#if (AVDSP_TRANSFER_MODE == 2) //use channel to transfer sample
    if(testct(c)) returnVal = inct(c);
    else returnVal = 0;
#else
    chkct(c, XS1_CT_END);
    returnVal = 0;
#endif
}

//used to transfer samples back and forth from the audio task, to fill our sample array
#pragma unsafe arrays
[[dual_issue]]void UserBufferManagement(unsigned sampsFromUsbToAudio[], unsigned sampsFromAudioToUsb[]){ unsafe {
    asm volatile("#UserBufferManagement:");
#if (AVDSP_TRANSFER_MODE == 2) //use channel for everything.
    //this is a user overloaded function returning the ressource channel to be used for communication
    unsigned caudio = avdspGetChanend();
    //transfer samples back and forth
    int ctval;
    select { //test if a token has been sent by the avdsp task, as a "ready" signal
        case testGetToken((chanend)caudio, ctval) : {
            //send samples from audio to dsp
            #pragma loop unroll
            for (int i=0; i < AVDSP_NUM_ADCIN; i++)  outuint((chanend)caudio,sampsFromAudioToUsb[ i ]);
            #pragma loop unroll
            for (int i=0; i < AVDSP_NUM_USBOUT; i++)  outuint((chanend)caudio,sampsFromUsbToAudio[ i ]);
            //receive sample from dsp to audio
            #pragma loop unroll
            for (int i=0; i < AVDSP_NUM_DACOUT; i++)  sampsFromUsbToAudio[ i ] = inuint((chanend)caudio);
            #pragma loop unroll
            for (int i=0; i < AVDSP_NUM_USBIN; i++)   sampsFromAudioToUsb[ i ] = inuint((chanend)caudio);
            chkct((chanend)caudio,XS1_CT_END);  //get end of transmission by avdsp
            outct((chanend)caudio, XS1_CT_END ); //send end of transmission by audio, this closes channel and triggers main task start
            break; }
        default : {
            //problem no token on time : timeout/overlap : call user function on its own tile.
            avdspTimeOverlap();
            break; }
    }
#else
    int time = getTime();   //time stamp to measure time spent here
    u64_t running = baseAudio.tcb.running;  //load tasks still running
    if (running.ll == 0) {
        //all tasks finished, verify if master is ready
        if (baseAudio.tcb.ready) {
            int offset = AVDSP_MAX_SAMPLES - baseAudio.samplesOfs;
#if (AVDSP_TRANSFER_MODE == 1) //use memory transfer
            //same tile as audio, eventually copy samples back and forth
            #pragma loop unroll
            for (int i=0; i < AVDSP_NUM_ADCIN; i++)
                baseAudio.samplePtr[ offset + i ] = sampsFromAudioToUsb[ i ];
            #pragma loop unroll
            for (int i=0; i < AVDSP_NUM_USBOUT; i++)
                baseAudio.samplePtr[ offset + (AVDSP_MAX( AVDSP_NUM_ADCIN , 1*AVDSP_MIN_SAMPLES ) ) + i ] = sampsFromUsbToAudio[ i ];
            #pragma loop unroll
            for (int i=0; i < AVDSP_NUM_DACOUT; i++)
                sampsFromUsbToAudio[ i ] = baseAudio.samplePtr[ offset + (AVDSP_MAX( AVDSP_NUM_ADCIN + AVDSP_NUM_USBOUT , 2*AVDSP_MIN_SAMPLES ) ) + i ];
            #pragma loop unroll
            for (int i=0; i < AVDSP_NUM_USBIN; i++)
                sampsFromAudioToUsb[ i ] = baseAudio.samplePtr[ offset + (AVDSP_MAX( AVDSP_NUM_ADCIN + AVDSP_NUM_USBOUT + AVDSP_NUM_DACOUT , 3*AVDSP_MIN_SAMPLES ) ) + i ];
#endif
            baseAudio.samplesOfs = offset;  //formally switch A/B array
            unsigned runable = baseAudio.tcb.runable.hl.lo;
            //check if we need more than 1 task, to kick all the other slave tasks eventually
            if (runable & 0xFFFFFF00)
                asm volatile("msync res[%0]"::"r"(baseAudio.tcb.synchronizer));
            //check if the main task can be launched
            if (runable & 0xFF)
                outct((chanend)baseAudio.tcb.caudio, XS1_CT_END ); //trigger new task
            baseAudio.tcb.running = baseAudio.tcb.runable;
        }
    } else {
        //time overlap. save which task(s) are overlapping, for further reporting
        baseAudio.tcb.runlast = running;
    }
    baseAudio.tcb.timeLaunch = getTime() - time;
#endif
} }

//used to transfer samples back and forth with the audio task, to fill sample array
#pragma unsafe arrays
static void transferSamplesFromAvsdp(chanend c){ unsafe {
    asm volatile("#transferSamplesFromAvsdp:");
#if (AVDSP_TRANSFER_MODE == 2) //use channel
    //verify slave tasks readiness and
    int offset = AVDSP_MAX_SAMPLES - baseLocal.samplesOfs;
    #pragma loop unroll
    for (int i=0; i < AVDSP_NUM_ADCIN; i++) baseLocal.samples[ offset + i ] = inuint(c);
    #pragma loop unroll
    for (int i=0; i < AVDSP_NUM_USBOUT; i++) baseLocal.samples[ offset + AVDSP_NUM_ADCIN + i ] = inuint(c);
    #pragma loop unroll
    for (int i=0; i < AVDSP_NUM_DACOUT; i++) outuint(c,baseLocal.samples[ offset + AVDSP_NUM_ADCIN + AVDSP_NUM_USBOUT + i ]);
    #pragma loop unroll
    for (int i=0; i < AVDSP_NUM_USBIN; i++) outuint(c,baseLocal.samples[ offset + AVDSP_NUM_ADCIN + AVDSP_NUM_USBOUT + AVDSP_NUM_DACOUT + i ]);
    outct(c,XS1_CT_END);
    baseLocal.samplesOfs = offset;
    chkct(c,XS1_CT_END);
    if (baseLocal.tcb.runable.hl.lo & 0xFFFFFF00)
        asm volatile("msync res[%0]"::"r"(baseLocal.tcb.synchronizer));
    baseLocal.tcb.running = baseLocal.tcb.runable;
    //START Slave tasks.
#endif
} }

//used to potentially empty the audio channel
static void flushAudioChannel(chanend c){ unsafe {
    if (baseLocal.tcb.ready) {
        //try to avoid audio task to start all tasks and sending token
        baseLocal.tcb.ready = 0;
        timerafterTicks(baseLocal.tcb.timeLaunch);
        //verify if the audio task finally started the main tasks
        if (baseLocal.tcb.running.hl.lo & 0xFF) {
            //yes, capture samples data and/or token as the select case will not be handled.
            int ctval;
            testGetToken(c, ctval);
            if (ctval == 0) transferSamplesFromAvsdp(c);
            //wait end of all slave tasks
            do { asm volatile("nop":::"memory"); } while (baseLocal.tcb.running.ll & 0xFFFFFFFFFFFFFF00);
            //force first task to 0
            baseLocal.tcb.running.hl.lo = 0;
        }
    }
} }


//allocate dsp task to the standard runtime or to weak user tasks
#if defined( AVDSP_RUNTIME ) && ( AVDSP_RUNTIME > 0 )
#define AVDSPTASK(_n) avdsp_rt_task(_n)
#else
//template for a generic while loop with the ssync function and time gathering. calling the weak function.
#define AVDSPTASK(_n) while(1) {                            \
        asm volatile("ssync":::"memory");                   \
        int oldTime = getTime();                            \
        avdspTask ## _n();                                  \
        baseTasks.tcb.inf[_n].time = getTime() - oldTime;   \
        baseTasks.tcb.running.c[_n] = 0; }
#endif

#pragma unsafe arrays
static int mainTask(server interface avdsp_if i, chanend caudio){
    debug_printf("lavdsp : mainTask\n");
    asm volatile("#avdspmainTask:");
    baseLocal.tcb.runable.ll = 0;
    for (int i=1; i<=baseLocal.tasks; i++)
        baseLocal.tcb.runable.ll |= 1ULL<<((i-1)*8);
    while (1) {
        int ctval;
        baseLocal.tcb.ready = baseLocal.started;
        select {
        //verify channel content to extract samples and/or launch dsp taks1
        case testGetToken(caudio, ctval) : {
            if (ctval == 0)  transferSamplesFromAvsdp(caudio);
            baseLocal.tcb.ready = 0;
            avdspTask1();
            break; }

        //this interface is used by the avdsp runtime to dynamically load user created dsp codes in xmos memory
#if defined( AVDSP_RUNTIME ) && ( AVDSP_RUNTIME > 0 )
        case i.loadCodePage(unsigned page, unsigned buf[16]) -> int result : {
            flushAudioChannel(caudio);
            unsigned localBuf[16];
            memcpy(localBuf,buf,sizeof(localBuf));
            result = avdsp_rt_loadCodePage(page, localBuf);
            break; }
#endif

        //to be launched at the very begining of the user application
        case i.init(unsigned min, unsigned max) -> int res : {
            debug_printf("lavdsp : init\n");
            baseLocal.program = 0;
            baseLocal.started = 0;
            baseLocal.fs = 0;
            baseLocal.fsMin = min;
            baseLocal.fsMax = max;
            res = 1;
            break; }

        case i.start() -> int res : {
            debug_printf("lavdsp : start\n");
            flushAudioChannel(caudio);
            baseLocal.started = 1;
            res = 1;
            break; }

        case i.stop() -> int res : {
            debug_printf("lavdsp : stop\n");
            flushAudioChannel(caudio);
            baseLocal.started = 0;
            res = 1;
            break; }

        case i.changeFS(int newFS) -> int res : {
            flushAudioChannel(caudio);
            if ( (newFS >= baseLocal.fsMin) && (newFS <= baseLocal.fsMax)) {
                res = 1;
                baseLocal.fs = newFS;
                int val;
#if defined( AVDSP_RUNTIME ) && ( AVDSP_RUNTIME > 0 )
                val = avdsp_rt_changeFS(newFS);
#else
                val = avdspChangeFS(newFS);
#endif
                if ( val && (val != baseLocal.tasks)) return val;   //change number of tasks
            } else
                res = 0;
            break; }

        case i.changeProgram(unsigned newProg) -> int res : {
            debug_printf("lavdsp : changeProgram(%d)\n",newProg);
            flushAudioChannel(caudio);
            res = 1;
#if defined( AVDSP_RUNTIME ) && ( AVDSP_RUNTIME > 0 )
            avdsp_rt_setProgram(int prog);
#else
            int val = avdspSetProgram(newProg);
            baseLocal.program = newProg;
            if ( val && (val != baseLocal.tasks)) return val;   //change number of tasks
#endif
            break; }

        //return key informations related to running program
        case i.getInfo(avdsp_info_t info) -> int res : { unsafe {
            debug_printf("lavdsp : getInfo\n");
            flushAudioChannel(caudio);
            info.tasksLaunched = baseLocal.tasks;
            for (int i=0; i<8; i++) info.time[i] = baseLocal.tcb.inf[i].time;
            res = 1;
            break; } }

        case i.setVolume(unsigned num, const float vol) -> int res : {
            //avdspSetVolume(num, q31( vol) );
            res = 1;
            break; }

        case i.setVolumeDB(unsigned num, const float volDB) -> int res : {
            res = 1;
            //avdspSetVolume(num, q31db(volDB));
            break; }

        case i.setBiquadCoefs(unsigned num, const float F, const float Q, const float G) -> int res : {
            res = avdspSetBiquadCoefs(num, F, Q, G);
            break; }

        case i.writeMemory(unsigned address, unsigned array[nwords], unsigned nwords ) -> int res : {

            break; }

        case i.readMemory(unsigned address, unsigned array[nwords], unsigned nwords ) -> int res : {

            break; }

        }
    }
    return 1;   //never happens
}


// main task in charge of dispatching interfaces requests
// or executing main DSP task triggerd by a channend token from audio task.
#pragma unsafe arrays
void lavdspMain(server interface avdsp_if avdspif, chanend caudio, const int tasksMax) {
    asm(".linkset baseAudio,avdspBase");   //create another symbol to avoid paralell rules usage...
    asm(".linkset baseLocal,avdspBase");   //create another symbol to avoid paralell rules usage...
    asm(".linkset baseTasks,avdspBase");   //create another symbol to avoid paralell rules usage...
    tcbInit();
    asm volatile ("#lavdspMain:");
    unsafe {
        baseLocal.tcb.caudio = (unsigned)caudio;
        asm volatile("stw %0,dp[u_avdspif]" :: "r"(avdspif)); }
    baseLocal.tasksMax = tasksMax;
    //can be overloaded by user program. default code returns 1
    baseLocal.tasks = avdspInit();
    while(1) {
        if (baseLocal.tasks > tasksMax) {
            debug_printf("lavdsp : not enough cores (%d) to launch expected tasks (%d)\n",tasksMax,baseLocal.tasks);
            baseLocal.tasks = tasksMax; }
        asm volatile ("#lavdspMainSwitch:");
        //launch all required tasks, not more, not less
        switch (baseLocal.tasks) {
        case 1 : {
            baseLocal.tcb.synchronizer = 0;
            baseLocal.tasks = mainTask(avdspif, caudio);
            break; }
//voluntary creating "par" statement dynamically with a maximum number of tasks depending on AVDSP_TASKS_MAX
#if (AVDSP_TASKS_MAX >= 2)
        case 2 : { par {
            AVDSPTASK(2);
            { asm volatile("stw r5,dp[baseLocal+28]":::"memory");
              baseLocal.tasks = mainTask(avdspif, caudio); }
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 3)
        case 3 : { par {
            AVDSPTASK(2); AVDSPTASK(3);
            { asm volatile("stw r5,dp[baseLocal+28]":::"memory");
              baseLocal.tasks = mainTask(avdspif, caudio); }
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 4)
        case 4 : { par {
            AVDSPTASK(2); AVDSPTASK(3); AVDSPTASK(4);
            { asm volatile("stw r5,dp[baseLocal+28]":::"memory");
              baseLocal.tasks = mainTask(avdspif, caudio); }
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 5)
        case 5 : { par {
            AVDSPTASK(2); AVDSPTASK(3); AVDSPTASK(4); AVDSPTASK(5);
            { asm volatile("stw r5,dp[baseLocal+28]":::"memory");
              baseLocal.tasks = mainTask(avdspif, caudio); }
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 6)
        case 6 : { par {
            AVDSPTASK(2); AVDSPTASK(3); AVDSPTASK(4); AVDSPTASK(5); AVDSPTASK(6);
            { asm volatile("stw r5,dp[baseLocal+28]":::"memory");
              baseLocal.tasks = mainTask(avdspif, caudio); }
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 7)
        case 7 : { par {
            AVDSPTASK(2); AVDSPTASK(3); AVDSPTASK(4); AVDSPTASK(5); AVDSPTASK(6); AVDSPTASK(7);
            { asm volatile("stw r5,dp[baseLocal+28]":::"memory");
              baseLocal.tasks = mainTask(avdspif, caudio); }
            } break; }
#endif
#if (AVDSP_TASKS_MAX >= 8)
        case 8 : { par {
            AVDSPTASK(2); AVDSPTASK(3); AVDSPTASK(4); AVDSPTASK(5); AVDSPTASK(6); AVDSPTASK(7); AVDSPTASK(8);
            { asm volatile("stw r5,dp[baseLocal+28]":::"memory");
              baseLocal.tasks = mainTask(avdspif, caudio); }
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
