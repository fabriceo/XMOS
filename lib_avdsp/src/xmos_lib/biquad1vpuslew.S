// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

/*
int32_t avdsp_biquadvpuslew(
  int32_t new_sample,
  q2_30 coeffs[8],
  q2_30 target_coeffs[8],
  int32_t state[8],
  left_shift_t lsh,
  int32_t slew_shift)
*/

#define FUNCTION_NAME avdsp_biquadvpuslew

#define NSTACKVECS (1)
#define NSTACKWORDS     (2 + 8 * NSTACKVECS)
#define STACK_VEC_TMP   (NSTACKWORDS - 8)

#define sample      r0
#define coeffs      r1
#define target		r2
#define state       r3

.text
.issue_mode dual
.globl FUNCTION_NAME;
.type FUNCTION_NAME,@function
.align 8
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

FUNCTION_NAME:
  { dualentsp NSTACKWORDS       ; mov r11, coeffs             }
  std r4, r5, sp[0]

  { vldr r11[0]                 ; ldc r11, 0                  }
  { vsetc r11                   ;                             }

  // first we slew the coeffs to target coeffs
  // coeffs += (target_coeffs - coeffs) >> slew_shift

  { vlsub r2[0]                 ; ldaw r2, sp[STACK_VEC_TMP]  }

  { ldw r11, sp[NSTACKWORDS + 2];                             }
  { vstr r2[0]                  ;                             }
  
  vlashr r2[0], r11

  { vladd coeffs[0]             ;                             }
  { vstr coeffs[0]              ;                             }

  { vclrdr                      ;                             }

  // state has x[n - 1] x[n - 2] x[n - 3] y[n - 1] y[n - 2] ....

  ldd r5, r4, state[0]
  std r4, sample, state[0]
  { stw r5, state[2]            ;                             }

  // state has x[n] x[n - 1] x[n - 2] y[n - 1] y[n - 2] ....

  { vldc coeffs[0]              ;                             }
  { vlmaccr state[0]            ;                             }
  // vR[0] has y[n]

  { vstr r2[0]                  ;                             }
  { ldw sample, r2[0]           ;                             }
  { ldw r4, state[3]            ;                             }
  { stw sample, state[3]        ;                             }

  { ldw r1, sp[NSTACKWORDS + 1] ;                             }
  { stw r4, state[4]            ; shl sample, sample, r1      }

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
