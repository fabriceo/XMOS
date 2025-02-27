// Copyright 2024-2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.



/*
int32_t adsp_biquad(int32_t new_sample,
                    int32_t * coeffs,
                    int32_t * state,
                    const int32_t lshift)
*/


#define FUNCTION_NAME   avdsp_biquadvpu

#define NSTACKVECS      (1)
#define NSTACKWORDS     (2 + 8 * NSTACKVECS)
#define STACK_VEC_TMP   (NSTACKWORDS - 8)

#define sample      r0
#define coeffs      r1
#define state       r2
#define lshift      r3

.text
.issue_mode dual
.globl FUNCTION_NAME;
.type FUNCTION_NAME,@function
.align 8
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

FUNCTION_NAME:
    dualentsp NSTACKWORDS
    std r4, r5, sp[0]
    {   vclrdr                      ;   ldc r11, 0                  }
    {   vsetc r11                   ;                               }

    // state has x[n - 1] x[n - 2] x[n - 3] y[n - 1] y[n - 2] ....
    
    ldd r5, r4, state[0]
    std r4, sample, state[0]
    {   stw r5, state[2]            ;                               }
    
    // state has x[n] x[n - 1] x[n - 2] y[n - 1] y[n - 2] ....
    
    {   vldc coeffs[0]              ;                               }
    {   vlmaccr state[0]            ;   ldaw r1, sp[STACK_VEC_TMP]  }
    // vR[0] has y[n]
    
    {   vstr r1[0]                  ;                               }
    {   ldw sample, r1[0]           ;                               }
    {   ldw r4, state[3]            ;                               }
    {   stw sample, state[3]        ;                               }
    {   stw r4, state[4]            ;   shl sample, sample, lshift  }

    // state has x[n] x[n - 1] x[n - 2] y[n] y[n - 1] ....

    ldd r4, r5, sp[0]
    retsp NSTACKWORDS

.cc_bottom FUNCTION_NAME.function; 
.set FUNCTION_NAME.nstackwords,NSTACKWORDS;     .global FUNCTION_NAME.nstackwords; 
.set FUNCTION_NAME.maxcores,1;                  .global FUNCTION_NAME.maxcores; 
.set FUNCTION_NAME.maxtimers,0;                 .global FUNCTION_NAME.maxtimers; 
.set FUNCTION_NAME.maxchanends,0;               .global FUNCTION_NAME.maxchanends; 
.L_size_end:
    .size FUNCTION_NAME, .L_size_end - FUNCTION_NAME
