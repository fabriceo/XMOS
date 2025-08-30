/*
 * lavdsp.h
 *
 *  Created on: 17 févr. 2025
 *      Author: Fabriceo
 */

#ifndef LAVDSP_H_
#define LAVDSP_H_

//helper functions used in the conf file.
#define AVDSP_MAX(a,b) (( (a) > (b) ) ? (a) : (b) )
#define AVDSP_MIN(a,b) (( (a) < (b) ) ? (a) : (b) )

#ifdef __lavdsp_conf_h_exists__
#include "lavdsp_conf.h"
#else
#include "../examples/lavdsp_conf_example.h"
#endif


#ifndef AVDSP_TASKS_MAX
#error AVDSP_TASKS_MAX must be defined in the configuration file
#endif

// maximum number of samples in the samples array
// will be multiplied by 2 to handle virtual switching
#ifndef AVDSP_MAX_SAMPLES
#error AVDSP_MAX_SAMPLES must be defined in the configuration file
#endif

//by default, any coefficient or gain value is code as 32bit signed Q4.28 = s3.28 = -8.000...+8.000
#ifndef AVDSP_MANT
#define AVDSP_MANT (28)
#endif
#if (AVDSP_MANT < 16)
#error not recomended to have less than 16 bits of mantissa
#endif

//64bits accumulator is signed and coded as Q5.59 by default, to allow one more bit of headroom.
#ifndef AVDSP_MANT64
#define AVDSP_MANT64 (64-(32-AVDSP_MANT+1))
#endif

#define AVDSP_MANT_MAX ((float)(1ULL<<(31-AVDSP_MANT)))
#define AVDSP_MANT_MIN (-AVDSP_MANT_MAX)


#ifndef __ASSEMBLER__   //this file is included in lavdsp_asm.S, so need to hide following code

#include <math.h>           //include pow() for qnmdb

typedef struct avdsp_info_s {
    unsigned short tasksLaunched;
    unsigned short time[8];
} avdsp_info_t;


#ifdef __XC__

#define XCUNSAFE unsafe

// interfaces definition for accessing dsp configuration

interface avdsp_if {
#if defined( AVDSP_RUNTIME ) && (AVDSP_RUNTIME != 0)
    //possibility to load one page of 64bytes into dsp opcode buffer.
    int loadCodePage(unsigned page, unsigned buf[16]);
#endif
    int init(unsigned min, unsigned max);
    int start();
    int stop();
    int changeFS(int newFS);
    int changeProgram(unsigned newProg);
    int getInfo(avdsp_info_t info);
    int setVolume(unsigned num, const float vol);
    int setVolumeDB(unsigned num, const float volDB);
    int setBiquadCoefs(unsigned num, const float F, const float Q, const float G);
    int writeMemory(unsigned address, unsigned array[nwords], unsigned nwords );
    int readMemory(unsigned address, unsigned array[nwords], unsigned nwords );
};

extern void lavdspMain(server interface avdsp_if avdspif, chanend caudio, const int tasksMax);

#else

#define XCUNSAFE

#endif


static inline int getTime() {
    int res;
    asm volatile("gettime %0" : "=r"(res));
    return res;
}


//fixed point helpers

static inline const int qnm(const float x) {
    float val = x;
    if (val >= AVDSP_MANT_MAX) return 0x7FFFFFFF;
    else
    if (val <= AVDSP_MANT_MIN) return 0x80000001;
    else val *= (1ULL<<(AVDSP_MANT));
    return val;
}

//this will be optimized by compiler
static inline const int qnmdb(const float x) {
    float val = pow(10.0,x/20.0);
    return qnm(val);
}

//#define q31(x) ((int)(((float)(x))*(float)(1ULL<<31)))
static inline const int q31(const float x) {
    float val = pow(10.0,x/20.0);
    if (val >= 1.0) return 0x7FFFFFFF;
    else
    if (val <=-1.0) return 0x80000001;
    else val *= (1ULL<<31);
    return val;
}


static inline const int q31db(const float x) {
    float val = pow(10.0,x/20.0);
    return q31(val);
}


//using this extended structure gives an easy way to extract msb or lsb
typedef union u64_u {
    unsigned long long ull;
    long long ll;
    unsigned int ui[2]; char c[8];
    struct { unsigned lo; int hi; } hl;
} u64_t;

//using this structure simplify type casting
typedef union u32_u {
    unsigned   u;
    int        i;
    char       c[4];
} u32_t;

// dsp tasks context compatible with avdsp runtime tcb
struct lavdsp_tcb8_s {
     unsigned * XCUNSAFE codePtr;    //adress of the code start for this core
     unsigned time;                  //number of cpu instructions taken in the core for one cycle
     char     * XCUNSAFE addr;       //adress of the task status (8bits access only) inside dspTasks.running
};


typedef struct lavdsp_tcb_s {
    u64_t runable;                  //0 pattern of task required for the program. 8 bits per core required
    u64_t runlast;                  //2 last known value of running
    u64_t running;                  //4 running status of the tasks . 8bits per core
    unsigned caudio;                //6 ressource for the audio channel
    unsigned synchronizer;          //7 ressource for the synchronizer. Dont move keep it as 7th words from the begining
    unsigned ready;                 //8 authorization to start main task from audio with token
    int timeLaunch;                 //9 total ticks measured to organize task launch and eventually sample transfer
    struct lavdsp_tcb8_s inf[8];    //10 + n * 3
} lavdp_tcb_t;


// base structure supporting the dsp control function.
typedef struct avdsp_base_s {
    lavdp_tcb_t tcb;                //first record holding task low level information (used in assembly : do not change order)
    unsigned fs;                    //current sampling rate
    unsigned fsMin;                 //minimum supported frequency
    unsigned fsMax;                 //maximum supported
    unsigned samplesOfs;            //current offset in the samples table (second dimension)
    unsigned started;               //0 when dsp treatment are is stoped
    unsigned program;               //number of the current DSP program in use
    unsigned tasks;                 //number of running tasks for this program
    unsigned tasksMax;              //max number of supported dsp cores/tasks on the tile
    int *    XCUNSAFE samplePtr;             //pointer on the current sample array
} avdsp_base_t;

//record of data is declared in lavdsp.xc
extern avdsp_base_t avdspBase;


#ifdef __cplusplus
extern "C" {
int avdsp_loadPage(unsigned page, unsigned buf[16]);
int avdsp_getInfo(unsigned buf[16]);
}

#endif

#endif //__ASSEMBLER__

#endif /* LAVDSP_H_ */
