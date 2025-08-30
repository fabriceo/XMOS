// Copyright (c) 2017-2019, XMOS Ltd, All rights reserved
// Version: May 1st 2020
// revisited by Fabriceo


//including main header to get AVDSP_MANT and  AVDSP_TASKS_MAX
#include "lavdsp.h"

#ifndef AVDSP_MANT
#define AVDSP_MANT (28)
#endif

#ifndef AVDSP_MANT64
#define AVDSP_MANT64 (AVDSP_MANT+31)
#endif

#if defined(__XS2A__) || defined(__XS3A__)

#define NSTACKWORDS 8
//.syntax default
//.syntax architectural

    .issue_mode  dual
	.text
	.globl	avdsp_biquads
	.type	avdsp_biquads,@function
	.cc_top avdsp_biquads.function,avdsp_biquads

	/*
extern  avdsp_biquads(
        int xn,
        avdsp_biquad_coef_s   * XCUNSAFE pbqc,	//coefficients
        avdsp_biquad_states_s * XCUNSAFE pbqs,	//states
        unsigned sections) {					//number of second order sections (>=1)
}
	*/
	#define s0 r0
	#define coeffs r1		/* pointer on the coeficients */
	#define states r2			/* pointer on state data */
	#define sections r3			/* number of section */
	#define h r4
	#define l r5
	#define s1 r6
	#define s2 r7
	#define c0 r8
	#define c1 r9

	.align	16
avdsp_biquads:
	{ ldc r11,24 ; dualentsp NSTACKWORDS }
    std r4, r5, sp[1]
    std r6, r7, sp[2]
    std r8, r9, sp[3]
	{ sub sections, sections, 1 ; bt sections, .L_entry }
	{ ldc r0,0 ; ldc r1,0 }
	retsp NSTACKWORDS

.L_entry2:
	{ add coeffs, coeffs, r11 ;	add states, states, r11 }	 // point on next biquad coeff and states


.L_entry:
	ldd h,l,states[2]			// get "remainder" from last calculation (otherwise would be 0)
	ldd c1, c0, coeffs[0] 		// c0=b0 and c1=b1
	maccs h, l, s0, c0 			// x[n] * b0
	ldd s2, s1, states[0]	 	// s1=x[n-1] and s2=x[n-2]
	maccs h, l, s1, c1			// x[n-1] * b1
	std s1, s0, states[0]		// put x[n-1] into x[n-2] and x[n] into x[n-1]
	ldd c0, c1, coeffs[1] 		// c1=b2 and c0=a1
	maccs h, l, s2, c1 			// x[n-2] * b2
	ldd s2, s1, states[1]	 	// s1=y[n-1] and s2=y[n-2]
	maccs h, l, s1, c0			// y[n-1] * a1	this coef is reduced by 1.0 due to remainder integration
	{ ldw c1, coeffs[4] ; ldc c0,(AVDSP_MANT) }			// c1=a2
	maccs h, l, s2, c1			// y[n-2] * a2
	lsats h, l, c0				// saturate
	std h,l,states[2]			// save accumulator for reminder re-integration at next cycle
	lextract s0, h, l, c0, 32	// remove BQ coefs precision and get back to sample precision (4.28)
	std s1, s0, states[1]		// store into y[n-1] and y[n-2]

	{ sub sections,sections,1 ; bt sections, .L_entry2 }	// leave if finished

    ldd r6, r7, sp[2]
    ldd r8, r9, sp[3]
	{ mov r2,l ; mov r1,s0 }
    ldd r4, r5, sp[1]
#if (AVDSP_MANT != 16) && (AVDSP_MANT<24)
//if the value is not compatible with the shl instruction, using a register
    ldc r3,(32-(AVDSP_MANT))
    { shl r0,r2,r3 ;  retsp NSTACKWORDS }
#else
	{ shl r0,r2,(32-(AVDSP_MANT)) ; retsp NSTACKWORDS }
#endif

	.cc_bottom avdsp_biquads.function
	.set	avdsp_biquads.nstackwords,NSTACKWORDS
	.globl	avdsp_biquads.nstackwords
	.set	avdsp_biquads.maxcores,1
	.globl	avdsp_biquads.maxcores
	.set	avdsp_biquads.maxtimers,0
	.globl	avdsp_biquads.maxtimers
	.set	avdsp_biquads.maxchanends,0
	.globl	avdsp_biquads.maxchanends
.Ltmp3:
	.size	avdsp_biquads, .Ltmp3 - avdsp_biquads

#else
#error
#endif
