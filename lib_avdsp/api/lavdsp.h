/*
 * lavdsp.h
 *
 *  Created on: 17 févr. 2025
 *      Author: Fabriceo
 */

#ifndef LAVDSP_H_
#define LAVDSP_H_

#ifdef __lavdsp_conf_h_exists__
#include "lavdsp_conf.h"
#endif


#ifdef __XC__

#define XCUNSAFE unsafe

// interfaces definition for accessing dsp configuration

typedef struct avdsp_info_s {
    unsigned short tasksLaunched;
    unsigned short time[8];
} avdsp_info_t;

interface avdsp_if {
#if defined( AVDSP_RUNTIME ) && ( AVDSP_RUNTIME > 0 )
    int loadCodePage(unsigned page, unsigned buf[16]);
#endif
    int getInfo(avdsp_info_t info);
    int start();
    int stop();
};

extern void lavdspMain(server interface avdsp_if avdspif, const int tasksMax);

//used to declare a weak statement within .xc files
#define AVDSP_WEAK(x) asm(".weak " #x ""); asm(".weak " #x ".nstackwords"); asm(".weak " #x ".maxcores"); asm(".weak " #x ".maxtimers"); asm(".weak " #x ".maxchanends")

#else

#define XCUNSAFE

#endif


#ifndef AVDSP_TASKS_MAX
#define AVDSP_TASKS_MAX 1
#endif
// maximum number of samples in the samples array
// will be multiplied by 2 to handle virtual switching
#ifndef AVDSP_SAMPLES_MAX
#define AVDSP_SAMPLES_MAX 8
#endif
//by default, any coefficient or gain value is code as 32bit signed Q4.28 = s3.28
//64bits accumulator is signed and coded as Q4.60 or s3.60
#ifndef AVDSP_MANT
#define AVDSP_MANT (28)
#endif


// base structure supporting the dsp control function.
typedef struct avdsp_base_s {
    unsigned fs;                //current sampling rate
    unsigned samplesOfs;        //current offset in the samples table (second dimension)
    unsigned started;           //0 when dsp treatment are is stoped
    unsigned program;           //number of the current DSP program in use
    unsigned tasks;             //number of running tasks with this program
    unsigned tasksMax;          //max number of supported cores/tasks on the tile
    int      samples[2*AVDSP_SAMPLES_MAX]; //table of samples I/O, twice size
}avdsp_base_t;

//record of data is declared in lavdsp.xc
extern avdsp_base_t avdspBase;

typedef union u64_u {
    unsigned
    long long  ull;
    long long  ll;
    double     d;
    float      f[2];
    unsigned
    int        ui[2];
    int        i[2];
    char       c[8];
    struct   { unsigned lo; int hi;  } hl;
    struct bfp { int mant;    int exp; } bfp;
} u64_t;

typedef union u32_u {
    unsigned   u;
    int        i;
    float      f;
    char       c[4];
} u32_t;


typedef struct lavdsp_tcb_s {
    u64_t runable;     //pattern of task required for the program. 8 bits per core required
    u64_t runlast;     //last known value of running
    u64_t running;     //status of the tasks . 8bits per core
    struct {
        unsigned * XCUNSAFE codePtr;   //adress of the code start for this core
        unsigned time;                 //number of cpu instructions taken in the core for one sample
        char * XCUNSAFE addr;          //adress of the task status (8bits access only) inside dspTasks.running
    } inf[8];
} lavdp_tcb_t;


//send a token to launch the dsp main task, if ready
static inline void avdspTrigMain(){
    unsigned ready;
    asm volatile("ldw %0,dp[avdspReady]":"=r"(ready));
    if (ready) {
        asm volatile("stw %0,dp[avdspReady]"::"r"(2));
        unsigned res;
        asm volatile("ldw %0,dp[uc_audio]":"=r"(res));
        asm volatile("outct res[%0],1"::"r"(res));
    }
}


#ifdef __cplusplus
extern "C" {
int avdsp_loadPage(unsigned page, unsigned buf[16]);
int avdsp_getInfo(unsigned buf[16]);
}

#endif

#endif /* LAVDSP_H_ */
