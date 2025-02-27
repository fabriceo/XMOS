// Copyright 2020-2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.



#if defined(__XS3A__)

//#include "../asm_helper.h"

/*  

typedef struct {
    unsigned biquad_count;
    int32_t state[2][9]; // state[j][k] is the value x_k[j], i.e.  x[n-j] of the kth biquad. x[j][8] are outputs of 8th biquad
    int32_t coef[5][8];  // coefficients. coef[j][k] is for the kth biquad. j maps to b0,b1,b2,-a1,-a2.
} filter_biquad_s32_t;

int32_t xmos_biquad_s32(
    filter_biquad_s32_t* filter,
    const int32_t new_sample);
*/

#define FUNCTION_NAME xmos_biquad_s32

#define NSTACKVECS      (0)
#define NSTACKWORDS     (4+8*NSTACKVECS)

#define FILT_N          0
#define FILT_STATE      1
#define FILT_COEF       19

#define COEF_START      32
#define STATE_START     10


#define state       r0      // ![0x%08X]
#define sample      r1      // ![%d]
#define coef        r2      // ![0x%08X]
#define tmp         r3      // ![%d]
#define _32         r4      // ![%d]
#define _36         r5      // ![%d]
#define filter      r10     // ![0x%08X]
    
.text
.issue_mode dual
.globl FUNCTION_NAME;
.type FUNCTION_NAME,@function
.align 16
.cc_top FUNCTION_NAME.function,FUNCTION_NAME

FUNCTION_NAME:
        dualentsp NSTACKWORDS
        std r4, r5, sp[1]
    {   ldc r11, 0                              ;   stw r10, sp[1]                          }
    {   mov filter, r0                          ;   vsetc r11                               }
    {   ldc tmp, FILT_STATE + STATE_START       ;                                           }
        ldaw state, filter[tmp]          // state <-- &(filter->state[1][1])
    {   ldc tmp, FILT_COEF + COEF_START         ;   vclrdr                                  }
        ldaw coef, filter[tmp]            // coef <-- &(filter->coef[4][0])

    {   ldc _36, 36                             ;   ldc _32, 32                             }

// Deal with the b2 and -a2 coefficients before b1 and -a1, so we can overwrite them easily.

    {   sub state, state, _36                   ;   vldc state[0]    /* y[n-2][] */         }
    {   sub coef, coef, _32                     ;   vlmacc coef[0]   /* -a2[] */            }
    {   add state, state, _32                   ;   vldc state[0]    /* y[n-1][] */         }
    {   sub coef, coef, _32                     ;   vlmacc coef[0]   /* -a1[] */            }
    {   sub state, state, _36                   ;   vldc state[0]    /* x[n-2] */           }
    {   sub coef, coef, _32                     ;   vlmacc coef[0]   /*  b2[] */            }
    {                                           ;   vldc state[0]    /* x[n-1] */           }
    {   sub coef, coef, _32                     ;   vlmacc coef[0]   /*  b1[] */            }

    // Now acc[k] =  b1[k] * x[n-1][k] + b2[k] * x[n-2][k] - a1[k] * y[n-1][k] - a2[k] * y[n-2][k]
    // state = &(filter->state[0][0])
    // coef = &(filter->coef[0][0])

#undef _36
#define N       r5      // ![%d]

    // Move filter->state[0][:] to filter->state[1][:]

    {   add r11, state, r5                      ;   vldc state[0]                           }
    {   add tmp, state, _32                     ;   ldw N, filter[FILT_N]                   }
    {   add r11, r11, _32                       ;   vstc r11[0]                             }
    {   shl N, N, 1                             ;   ldw tmp, tmp[0]                         }
    {   ldc tmp, 16                             ;   stw tmp, r11[0]                         }

    // Place the newest input sample in state[0][0]
    {   sub N, tmp, N                           ;   stw sample, state[0]                    }

    // Overwrite state[0][1:9] with 0's
    	ldap r11,.L_zero
    //    ldaw r11, cp[vpu_vec_zero]
    {   add r11, state, 4                       ;   vldc r11[0]                             }
    {                                           ;   vstc r11[0]                             }

    // vC[:] <-- coef[b0][:]
    {                                           ;   vldc coef[0]                            }

    // Every element in r11[0:8] except for r11[0] is zero, so a VLMACC shouldn't affect them.
    // Subsequent VLMACCs will corrupt the accumulators, but The Mask will stop that from being a
    // problem. Smokin'!

    // Let's make this more clear. We still haven't MACCed in the terms corresponding to b0,
    // but we can't do all of those simultaneously as we did with the others because the x[n-0]
    // for one section IS the output of the previous section, which we haven't finished calculating
    // yet. So we need to go up the chain of filter sections, computing the output of each to get
    // the input to the next. Because we've set the state[0][1:0] to zeros, when we're working on
    // the k'th filter section, MACCing against that will not affect accumulators > k. Then we write
    // out the output of section k. We do the MACC again, **which will corrupt the accumulators 
    // which are LESS THAN k.... but that's FINE because we're not going to write them out again.   
    {   mkmsk tmp, 4                            ;   vlmacc state[0]                         }
        vstrpv r11[0], tmp
    {                                           ;   bru N /* Do N-1 remaining biquads */    }

    {   shl tmp, tmp, 4                         ;   vlmacc state[0]                         }
        vstrpv r11[0], tmp
    {   shl tmp, tmp, 4                         ;   vlmacc state[0]                         }
        vstrpv r11[0], tmp
    {   shl tmp, tmp, 4                         ;   vlmacc state[0]                         }
        vstrpv r11[0], tmp
    {   shl tmp, tmp, 4                         ;   vlmacc state[0]                         }
        vstrpv r11[0], tmp
    {   shl tmp, tmp, 4                         ;   vlmacc state[0]                         }
        vstrpv r11[0], tmp
    {   shl tmp, tmp, 4                         ;   vlmacc state[0]                         }
        vstrpv r11[0], tmp
    {   shl tmp, tmp, 4                         ;   vlmacc state[0]                         }
        vstrpv r11[0], tmp

    // Final vstrpv should have written the output to filt->state[0][N]. filt->state should
    // still be pointing at filt->state[0][0]

    {                                           ;   ldw N, filter[FILT_N]                   }
    {                                           ;   ldw r0, state[N]                        }


.L_done:
    {                                           ;   ldw r10, sp[1]                          }
        ldd r4, r5, sp[1]
        retsp NSTACKWORDS

	.align 32
.L_zero:
	.word 0,0,0,0,0,0,0,0

.cc_bottom FUNCTION_NAME.function; 
.set FUNCTION_NAME.nstackwords,NSTACKWORDS;     .global FUNCTION_NAME.nstackwords; 
.set FUNCTION_NAME.maxcores,1;                  .global FUNCTION_NAME.maxcores; 
.set FUNCTION_NAME.maxtimers,0;                 .global FUNCTION_NAME.maxtimers; 
.set FUNCTION_NAME.maxchanends,0;               .global FUNCTION_NAME.maxchanends; 
.L_size_end:
    .size FUNCTION_NAME, .L_size_end - FUNCTION_NAME

#undef FUNCTION_NAME



#endif //defined(__XS3A__)
