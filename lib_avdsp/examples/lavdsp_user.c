/*
 * lavdsp_user.c
 *
 *  Created on: 19 févr. 2025
 *      Author: fabriceo
 */


#include "lavdsp_base.h"        //basic libray functions for dealing with accumulator
#include "lavdsp_filters.h"     //functions to compute filters coefficients and number of sections
#include <string.h>             //for memset

//create a pointer for syntax compatibility only
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
} p1data_t;


//data area for the dsp user code , example here for stereo treatment
//with 8 biquad, one delayline and gain on each
typedef struct p2data_s {
    avdsp_filter_def_t    bqdef[8];                 //filters original value definined by user application
    unsigned              bqsections[2];            //number of effective sections computed
    avdsp_biquad_coef_t   bqc[8];                   //resulting 8 coefficients, here identical for Left & Rigth
    avdsp_biquad_states_t bqs[2][8];                //placeholder for 16 biquads states
    avdsp_delay_t         delay[2];                 //2 delay lines (master record)
    u32_t                 delayLines[2][1000];      //scratch area for delay line
} p2data_t;


const int numberVolumes = 2;

//consolidate all programs data area into a single record
typedef union pdata_u {
    u32_t    volumes[numberVolumes];                  //same variable for 2 programs
    p1data_t p1;
    p2data_t p2;
} pdata_t;

static pdata_t d;


static inline void program1(const int n, const int source, const int dest){
    asm volatile(".issue_mode dual\n\t");
    u64_t accu;
    int sample;
    //load sample into accumulator
    avdsp_load( &accu, source );
    //compute biquad
    accu.ll = avdsp_biquads( accu.hl.hi, d.p1.bqc, d.p1.bqs[n], d.p1.bqsections );
    avdsp_gainQ31( & accu , d.volumes[n].i );
    sample = avdsp_saturateSample( accu.ll );
    avdsp_storeSample(sample, dest );
}

#define USBOUT_L 2
#define USBOUT_R 3
#define DAC_L 4
#define DAC_R 5
#define USBIN_L 6
#define USBIN_R 7

//single task for treating the Left and Right channel with same program
void program1_task1(){
    asm volatile("#program1_task1:");
    if (pBase->started == 0) return;
    //treat USB Left to DAC left
    program1( 0, USBOUT_L, DAC_L );
    //treat USB Right to DAC right
    program1( 1, USBOUT_R, DAC_R );
    //perform a loopback on USB side on left channel , for providing a reference during tests
    avdsp_copySample( USBOUT_L, USBIN_L );
    //perform a loopback on USB side on right channel , to provide visibility on the treatment done
    avdsp_copySample( DAC_R, USBIN_R );

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

int program1_init(){
    //clear our data record
    asm volatile("#program1_Init:");
    memset(&d.p1,0,sizeof(d.p1));
    return 1; //one task
}


/* SECOND PROGRAM */


static inline void program2(const int n, const int source, const int dest) {
    asm volatile(".issue_mode dual\n\t");
    u64_t accu;
    int sample;
    //load sample into accumulator
    avdsp_load( &accu, source );
    //compute biquad
    accu.ll = avdsp_biquads( accu.hl.hi, d.p2.bqc, d.p2.bqs[n], d.p2.bqsections[n] );
    //apply gain on accumulator with volume code q31
    avdsp_gainQ31( &accu, d.volumes[n].i);
    //apply gain on accumulator
    avdsp_gain( &accu, qnmdb( -6.0 ));
    //convert to a 32bit saturated sample
    sample = avdsp_saturateSample( accu.ll );
    //apply delay
    sample = avdsp_delaySample( sample, &d.p2.delay[n], d.p2.delayLines[n]);
    avdsp_storeSample(sample, dest);
}


void program2_task1(){
    asm volatile("#program2_task1:");
    if (pBase->started == 0) return;
    program2( 0, USBOUT_L, DAC_L );
    //perform a loopback on USB side on left channel , for providing a reference during tests
    avdsp_copySample( USBOUT_L, USBIN_L );
}

void program2_task2(){
    asm volatile("#program2_task2:");
    if (pBase->started == 0) return;
    program2( 1, USBOUT_R, DAC_R );
    //perform a loopback on USB side on right channel , to provide visibility on the treatment done
    avdsp_copySample( DAC_R, USBIN_R );
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


int program2_init(){
    asm volatile("#program2_Init:");
    memset(&d.p2,0,sizeof(d.p2));
    //compute maximum number of samples for each delay line.
    d.p2.delay[0].max = sizeof(d.p2.delayLines[0])/4;
    d.p2.delay[1].max = sizeof(d.p2.delayLines[1])/4;
    return 2; //2 tasks
}



void avdspTask1(){
    while (1) {
        switch (avdspBase.program ) {
        case 1: { program1_task1(); break; }
        case 2: { program2_task1(); break; }
        }
    }
}

void avdspTask2(){
    switch (avdspBase.program ) {
    case 2: { program2_task2(); break; }
    }
}

#if !defined( AVDSP_RUNTIME ) || ( AVDSP_RUNTIME == 0 )


//overload initialization symbol
int avdspInit(){
    return 1;
}

//overload weak symbol
// prepare for running the program "prog"
// and return the number of required tasks
int avdspSetProgram(unsigned prog){
    pBase->started = 0;       //stop all
    pBase->fs = 0;            //means that nothing is initialized yet
    pBase->program = prog;    //memorize program value (could test against number
    switch ( pBase->program ) {
    case 1: { pBase->tasks = program1_init(); break; }
    case 2: { pBase->tasks = program2_init(); break; }
    }
    return pBase->tasks;
}


//overload weak symbol
int avdspChangeFS(unsigned newFS) {
    if (newFS == avdspBase.fs) return 0;
    switch ( pBase->program ) {
    case 1: {
        if (newFS <= 96000) { program1_changeFS(newFS); return 1; }
        else return 0; //for example, fs limitaion due to a single dsp task
    }
    case 2: { program2_changeFS(newFS); return 1; }
    }
    return 0;
}

//overload weak symbol
//used to apply volumes from application into our data record.
int avdspSetVolume(unsigned num, const int vol) {
    //value expected in q31 format
    if (num == 0)
        for (int i=0; i< numberVolumes; i++) d.volumes[i].i = vol;
    else
        if (num <= numberVolumes) d.volumes[num-1].i = vol;
        else return 0;
    return 1;
}


int avdspSetBiquadCoefs(unsigned num, const float F, const float Q, const float G) {
    if (num < 3) {

        return 1;
    }
    return 0;
}

#endif
