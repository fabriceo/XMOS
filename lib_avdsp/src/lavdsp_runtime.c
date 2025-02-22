/*
 * lavdsp_runtime.c
 *
 *  Created on: 21 févr. 2025
 *      Author: fabriceo
 */

#include "lavdsp_runtime.h"
#include <string.h>     //get memcpy
#include <xs1.h>        //get set_core_high_priority_off, set_thread_fast_mode_off
#include <xclib.h>        //get set_core_high_priority_off, set_thread_fast_mode_off

//extern void dspRunMasterXS2( );  //launch runtime on Master task
//extern void dspRunSlaveXS2(struct lavdsp_tcb8_s * p);  //loop runtime waiting for a msync instruction by the master task, or mjoin in the end
//extern int  dspRunPatchMant(int mant, int mant2);


void dspRunMasterXS2( )  __attribute__ ((weak));
void dspRunMasterXS2( ) { }
void dspRunSlaveXS2(struct lavdsp_tcb8_s * p)  __attribute__ ((weak));
void dspRunSlaveXS2(struct lavdsp_tcb8_s * p) { }

void avdsp_rt_task(const int N) {
    asm volatile("#avdsp_rt_task:");
    if (N == 1) dspRunMasterXS2( );
    else {
        //set_core_high_priority_off();
        //set_thread_fast_mode_off();
        dspRunSlaveXS2(&avdspBase.tcb.inf[N-1]);
    }
}

#ifndef AVDSP_RUNTIME_SIZE
#define AVDSP_RUNTIME_SIZE (1024) /* default size for the dsp opcodes in words */
#endif

//array to hold the dspprogram (list of opcodes) + data array
unsigned long long avdspBuf[AVDSP_RUNTIME_SIZE/2];

static int codeSize;
int avdsp_rt_loadCodePage(unsigned page, unsigned buf[16]){
    static int size = 0;
    static int nextPage = 0;
    int result = -1; // error by default
    if (page == nextPage) {
        if (size == 0) {
            //dspResetProg();
            codeSize = 0;
        }
        if ( ( size + page*16 ) < AVDSP_RUNTIME_SIZE ) {
            char * p = (char *)avdspBuf;
            p += size*4;
            memcpy(p, (char *)buf, 4*16);
            size += 16; nextPage = page + 1;
            result = 0; // success
         }
    } else
    if (page == 0) {
        //end of process : check what was loaded
        codeSize = size; result = 0;
    }
    return result;
}


