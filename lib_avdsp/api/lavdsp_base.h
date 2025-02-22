/*
 * lavdsp_inc.h
 *
 *  Created on: 19 févr. 2025
 *      Author: fabriceo
 */

#ifndef LAVDSP_INC_H_
#define LAVDSP_INC_H_

#include "lavdsp.h"


//structure defining a filter with its type and the parameters
typedef struct avdsp_filter_def_s {
    unsigned type;  //according to filter list
    float    freq;
    float    Q;     //only for second orders
    float    gain;
    float    freq2; //only for LTs
    float    Q2;
} avdsp_filter_def_t;


//structure defining one raw of biquad coefficients for one second order filter. Require allignement 8bytes
typedef union avdsp_biquad_coef_u {
    long long padding8;
    int c[6];   //b0,b1,b2,a1,a2,spare for 8bytes allignement (a1 reduced by 1.0)
} avdsp_biquad_coef_t;


//structure defining storage area for the biquad states xn-1,xn-2,yn-1,yn-2,
//and eventually yn as 64bits. require allignement 8bytes
typedef union avdsp_biquad_states_u {
    long long padding8;
    int s[6];   //xn-1,xn-2,yn-1,yn-2,yn(64)
} avdsp_biquad_states_t;


//structure defining a delay line, with its max size,
//current index and offset of delayline memory area
typedef struct avdsp_delay_s {
    unsigned max;           //max number of samples in delay line buffer
    unsigned micros;        //user value in microseconds
    unsigned value;         //corresponding number of samples
    unsigned index;         //current position of the index in the delayline array
} avdsp_delay_t;


//virtually switch the two samples array. To be done at each samples at fs
static inline void avdsp_switchSampleOfs(avdsp_base_t * ptr) {
    ptr->samplesOfs = (AVDSP_SAMPLES_MAX) - ptr->samplesOfs;
}


//return sample value from sample array at position "n"
static inline int avdsp_loadSample(avdsp_base_t * ptr, const int n){
    return ptr->samples[ ptr->samplesOfs + n ];
}


//load sample "n" into 64 bit accumulator, in Qx.y format
static inline void avdsp_load(u64_t * accu, avdsp_base_t * ptr, const int n) {
    asm volatile("#avdsp_load:");
    int sample  = avdsp_loadSample( ptr, n );
    accu->ll = 0;
    asm("linsert %0,%1,%2,%3,32":"+r"(accu->hl.hi),"+r"(accu->hl.lo):"r"(sample),"r"(AVDSP_MANT));
}



//load a 32 bit value coded Q1.31 (as a sample) into 64 bit accumulator, in Qx.y format
static inline void avdsp_valueSample(u64_t * accu, int sample) {
    asm volatile("#avdsp_valueQ31:");
    accu->ll = 0;
    asm("linsert %0,%1,%2,%3,32":"+r"(accu->hl.hi),"+r"(accu->hl.lo):"r"(sample),"r"(AVDSP_MANT));
}



//load sample "n" and add it to the 64 bit accumulator, in Qx.y format
static inline void avdsp_loadAdd(u64_t * accu, avdsp_base_t * ptr, const int n) {
    asm volatile("#avdsp_loadAdd:");
    int sample  = avdsp_loadSample( ptr, n );
    u64_t accu2 = { .ll = 0 };
    asm("linsert %0,%1,%2,%3,32":"+r"(accu2.hl.hi),"+r"(accu2.hl.lo):"r"(sample),"r"(AVDSP_MANT));
    accu->ll += accu2.ll;
}


//store a 32bit value in the sample array
static inline void avdsp_storeSample(int sample, avdsp_base_t * ptr, const int n) {
    ptr->samples[ ptr->samplesOfs + n ] = sample;
}


//store accu long long into sample array at position "n"
static inline void avdsp_store(u64_t * accu, avdsp_base_t * ptr, const int n) {
    int sample;
    asm("lsats %0,%1,%2":"+r"(accu->hl.hi),"+r"(accu->hl.lo):"r"(AVDSP_MANT));
    asm("lextract %0,%1,%2,%3,32":"=r"(sample):"r"(accu->hl.hi),"r"(accu->hl.lo),"r"(AVDSP_MANT));
    avdsp_storeSample(sample, ptr, n);
}


//multiply 64 bits accu by a signed gain coded Qx.y.
//96 bits result is resized to fit 64bits and original format is kept.
static inline void avdsp_gain(u64_t * accu, int gain){
    asm volatile("#avdsp_gain:");
    int sign,zero=0;
    asm volatile("# using %0 (sign)":"=r"(sign)); //used to force compiler to keep this register
    asm volatile(
            "ashr   %3,%2,32            \n\t"   //get sign
            "mul    %3,%1,%3            \n\t"
            "maccu  %3,%4,%1,%2         \n\t"
            "ashr   %1,%3,32            \n\t"   //get sign of result and use it as msb
            "maccs  %1,%3,%0,%2         \n\t"
            "lsats %1,%3,%5             \n\t"
            "lextract %0,%1,%3,%5,32    \n\t"   //restore original format.
            "lextract %1,%3,%4,%5,32"
            :"+r"(accu->hl.hi),"+r"(accu->hl.lo)
            : "r"(gain),"r"(sign),"r"(zero),"r"(AVDSP_MANT) );
}

//apply a delay to the given sample
static inline int avdsp_delaySample(int sample, avdsp_delay_t * dptr, u32_t * ptr ){
    asm volatile("#avdsp_delaySample:");
    int idx,val;
    asm volatile("# using %0 (idx) ,%1 (val)":"=r"(idx),"=r"(val));
    asm volatile(
            "ldw %1,%3[2]                   \n\t"   //load index
            "{ ldw %2,%3[1] ; add %1,%1,1 } \n\t"   //load delay, and increment index
            "lsu %2,%1,%2                   \n\t"   //check reaching end of buffer
            "neg %2,%2                      \n\t"   //create a 0 or FFFFFFFF mask depending on comparaison
            "and %1,%1,%2                   \n\t"   //clear index or let it ok
            "{ stw %1,%3[2] ; mov %2,%0 }   \n\t"   //store new index
            "ldw %0,%4[%1]                  \n\t"   //load old sample
            "stw %2,%4[%1]                  \n\t"   //store new sample
            :"+r"(sample):"r"(idx),"r"(val),"r"(dptr),"r"(ptr));
    return sample;
}

//apply a delay to the accumulator in 64 BITS : require twice size of delay line buffer
static inline void avdsp_delay64(u64_t * accu, avdsp_delay_t * dptr, int * ptr ) {
    asm volatile("#avdsp_delay64:");
    int idx,val;
    asm volatile("# using %0 (idx) ,%1 (val)":"=r"(idx),"=r"(val));
    asm volatile(
            "ldw %2,%4[1]                   \n\t"
            "{ ldw %3,%4[0] ; add %2,%2,1 } \n\t"
            "lsu %3,%2,%3                   \n\t"
            "neg %3,%3                      \n\t"
            "{ and %2,%2,%3 ; mov %3,%0 }   \n\t"
            "{ stw %2,%4[1] ; mov %2,%1 }   \n\t"
            "ldd %0,%1,%4[%1]               \n\t"
            "stw %3,%2,%4[%1]               \n\t"
            :"+r"(accu->hl.hi),"+r"(accu->hl.lo):"r"(idx),"r"(val),"r"(dptr),"r"(ptr));
}

//from lavdsp_biquads.S
//compute multiple biquads on the given sample xn and return a Qx.y 64bits result,
//original format kept where the original 32bits is simply extended to 64bits.
extern long long avdsp_biquads(
        int xn,
        avdsp_biquad_coef_t   * pbqc,
        avdsp_biquad_states_t * pbqs,
        unsigned sections);


//saturate accumulator so that 64bits value is between -1..+1 without changing format
static inline void avdsp_saturate(u64_t * accu) {
    asm("lsats %0,%1,%2":"+r"(accu->hl.hi),"+r"(accu->hl.lo):"r"(AVDSP_MANT));
}

//return a 32bits value between -1..+1 from a 64bit value
static inline int avdsp_saturateSample(long long accu) {
    int ah = (long long)(accu >> 32);
    unsigned al = accu & 0xFFFFFFFF;
    asm("lsats %0,%1,%2":"+r"(ah),"+r"(al):"r"(AVDSP_MANT));
    asm("lextract %0,%1,%2,%3,32":"=r"(ah):"r"(ah),"r"(al),"r"(AVDSP_MANT));
    return ah;
}

//compute number of samples considering given micro seconds value and sampling frequency FS
static inline int avdsp_calcDelayMicrosecondsFS(unsigned micros, unsigned FS) {
    unsigned long long val = (long long)FS * micros;
    val += 1000000/2;
    val /= 1000000;
    return val;
}

//set the delay line to the given delay in microseconds at FS
static inline void avdsp_setDelayMicrosecondsFS(avdsp_delay_t * ptr, unsigned micros, unsigned FS){
    int val = avdsp_calcDelayMicrosecondsFS(micros,FS);
    if (val < ptr->max) ptr->value = val; else ptr->value = ptr->max;
    ptr->micros = micros;
    ptr->index = 0;
}

static inline void avdsp_calcDelayFS(avdsp_delay_t * ptr, unsigned FS) {
    avdsp_setDelayMicrosecondsFS(ptr, ptr->micros, FS);
}

#endif /* LAVDSP_INC_H_ */
