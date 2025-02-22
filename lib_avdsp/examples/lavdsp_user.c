/*
 * lavdsp_user.c
 *
 *  Created on: 19 févr. 2025
 *      Author: fabriceo
 */


#include "lavdsp_base.h"        //basic libray functions for dealing with accumulator
#include "lavdsp_filters.h"     //functions to compute filters coefficients and number of sections
#include <string.h>             //for memset

avdsp_base_t * const pBase = &avdspBase;

/*
 * This example provides 2 programs with respectives data and code
 * first program is basic control 3 bands on stereo channels.
 * 1 single task handling all treatment on 2 channels up to 384k
 *
 * second program is running 8 biquad on a stereo channels, and a delay line and volume (or balance) control
 * 1 tasks is allocated for the 2 channels when FS <= 192k
 * 2 tasks are allocated when FS >=192000 to cope with 384k
 */

/***** FIRST PROGRAM *****/

//data area for the dsp user code , example here for tone controle stereo
//with 3 biquad, and gain on each
typedef struct p1data_s {
    avdsp_filter_def_t    bqdef[3];                 //filters original value definined by user application
    unsigned              bqsections;               //number of effective sections computed
    avdsp_biquad_coef_t   bqc[3];                   //resulting 3 coefficients, here identical for Left & Rigth
    avdsp_biquad_states_t bqs[2][3];                //placeholder for 6 biquads states
    u32_t                 volume[2];                //2 volumes (or to control balance)
} p1data_t;


//data area for the dsp user code , example here for stereo treatment
//with 8 biquad, one delayline and gain on each
typedef struct p2data_s {
    avdsp_filter_def_t    bqdef[8];                 //filters original value definined by user application
    unsigned              bqsections[2];            //number of effective sections computed
    avdsp_biquad_coef_t   bqc[8];                   //resulting 8 coefficients, here identical for Left & Rigth
    avdsp_biquad_states_t bqs[2][8];                //placeholder for 16 biquads states
    avdsp_delay_t         delay[2];                 //2 delay lines (master record)
    u32_t                 volume[2];                //2 volumes (or to control balance)
    u32_t                 delayLines[2][1000];      //scratch area for delay line
} p2data_t;


//consolidate all programs data area into a single record
typedef union pdata_u {
    p1data_t p1;
    p2data_t p2;
} pdata_t;

static pdata_t d;


static inline void program1(const int n){
    u64_t accu;
    int sample;
    //load sample into accumulator
    avdsp_load( &accu, pBase, n );
    //compute biquad
    accu.ll = avdsp_biquads( accu.hl.hi, d.p1.bqc, d.p1.bqs[n], d.p1.bqsections );
    avdsp_gain( & accu , d.p1.volume[n].i );
    sample = avdsp_saturateSample( accu.ll );
    avdsp_storeSample(sample, pBase, n+2);
}


//single task for treating the 2 samples with same program
void program1_task1(){
    asm volatile("#program1_task1:");
    if (pBase->started == 0) return;
    avdsp_switchSampleOfs( pBase );
    program1(0);
    program1(1);
}


//the routines required when frequency is changing
void program1_changeFS(int newFS) {
    pBase->fs = newFS;
    //recompute all biquads
    int sections = avdsp_calcFiltersFS( d.p1.bqdef, d.p1.bqc, sizeof(d.p1.bqc)/sizeof(d.p1.bqc[0]), newFS );
    d.p1.bqsections = sections;
    //clear biquads states
    memset(d.p1.bqs,0,sizeof(d.p1.bqs));
}

int program1_Init(){
    asm volatile("#program1_Init:");
    memset(&d.p1,0,sizeof(d.p1));
    return 1; //one task
}


/* SECOND PROGRAM */


static inline void program2(const int n){
    u64_t accu;
    int sample;
    //load sample into accumulator
    avdsp_load( &accu, pBase, n );
    //compute biquad
    accu.ll = avdsp_biquads( accu.hl.hi, d.p2.bqc, d.p2.bqs[n], d.p2.bqsections[n] );
    //apply gain on accumulator
    avdsp_gain( &accu, d.p2.volume[n].i);
    //convert to a 32bit saturated sample
    sample = avdsp_saturateSample( accu.ll );
    //apply delay
    sample = avdsp_delaySample( sample, &d.p2.delay[n], d.p2.delayLines[n]);
    avdsp_storeSample(sample, pBase, n+2);
}


void program2_task1(){
    asm volatile("#program2_task1:");
    if (pBase->started == 0) return;
    avdsp_switchSampleOfs( pBase );
    program2(0);
}

void program2_task2(){
    asm volatile("#program2_task2:");
    if (pBase->started == 0) return;
    program2(1);
}


//the routines required when frequency is changing
void program2_changeFS(int newFS) {
    pBase->fs = newFS;
    //update each delayline
    avdsp_calcDelayFS( &d.p2.delay[0], newFS);
    avdsp_calcDelayFS( &d.p2.delay[1], newFS);
    //recompute all biquads
    int sections = avdsp_calcFiltersFS( d.p2.bqdef, d.p2.bqc, sizeof(d.p2.bqc)/sizeof(d.p2.bqc[0]), newFS );
    d.p2.bqsections[0] = sections;
    d.p2.bqsections[1] = sections;
    //clear biquads states
    memset(d.p2.bqs,0,sizeof(d.p2.bqs));
    //clear delaylines buffer
    memset(d.p2.delayLines,0,sizeof(d.p2.delayLines));
}


int program2_Init(){
    asm volatile("#program2_Init:");
    memset(&d.p2,0,sizeof(d.p2));
    //compute maximum number of samples for each delay line.
    d.p2.delay[0].max = sizeof(d.p2.delayLines[0])/4;
    d.p2.delay[1].max = sizeof(d.p2.delayLines[1])/4;
    return 2; //2 tasks
}

//overload initialization symbol
void avdspInit(){
}


void avdspTask1(){
    switch (avdspBase.program ) {
    case 1: { program1_task1(); break; }
    case 2: { program2_task1(); break; }
    }
}

void avdspTask2(){
    switch (avdspBase.program ) {
    case 2: { program2_task2(); break; }
    }
}

//overload weak symbol
int avdspSetProgram(int prog){
    pBase->started = 0;       //stop all
    pBase->fs = 0;            //means that nothing is initialized yet
    pBase->program = prog;    //memorize program value (could test against number
    switch ( pBase->program ) {
    case 1: { pBase->tasks = program1_Init(); break; }
    case 2: { pBase->tasks = program2_Init(); break; }
    }
    return pBase->tasks;
}


//overload weak symbol
int avdspChangeFS(unsigned newFS) {
    if (newFS == avdspBase.fs) return 0;
    switch ( pBase->program ) {
    case 1: { program1_changeFS(newFS); return 0; }
    case 2: { program2_changeFS(newFS); return 0; }
    }
    return 0;
}
