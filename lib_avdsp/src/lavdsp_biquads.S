// Copyright (c) 2017-2019, XMOS Ltd, All rights reserved
// Version: May 1st 2020
// revisited by Fabriceo


#if defined(__XS2A__) || defined(__XS3A__)

#ifndef AVDSP_MANT
#define AVDSP_MANT (28)
#endif

#define NSTACKWORDS 8
//.syntax default
//.syntax architectural

    .issue_mode  dual
	.text
	.globl	avdsp_biquads
	.align	4
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

avdsp_biquads:
	{ zext sections,8 ; dualentsp NSTACKWORDS } // keep sections lsb only
	{ sub sections, sections, 1 ; bt sections, .L_entry }
	retsp NSTACKWORDS

.L_entry:
    std r4, r5, sp[1]
    std r6, r7, sp[2]
    std r8, r9, sp[3]
	//r0 contains xn as q4.28. Maccs will produce q8.56, then reduced to 4.60
    bu .L_entry3

.L_entry2:
	ldaw coeffs, coeffs[6]		 // point on next biquad coeff and states
	ldaw states, states[6]

.L_entry3:
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
	std h,l,states[2]			// save accumulator for reminder re-integration at next cycle
	lsats h, l, c0				// saturate
	lextract s0, h, l, c0, 32	// remove BQ coefs precision and get back to sample precision (4.28)
	{ shl l,l,(32-(AVDSP_MANT)) ; mov h,s0 }	//reduce hl from q8.56 to q4.60
	std s1, s0, states[1]		// store into y[n-1] and y[n-2]

	{ sub sections,sections,1 ; bt sections, .L_entry2 }	// leave if finished
	{ mov r0,l ; mov r1,h }
    ldd r4, r5, sp[1]
    ldd r6, r7, sp[2]
    ldd r8, r9, sp[3]
	retsp NSTACKWORDS

	.cc_bottom avdsp_biquads.function
	.set	avdsp_biquads.nstackwords,0
	.globl	avdsp_biquads.nstackwords
	.set	avdsp_biquads.maxcores,1
	.globl	avdsp_biquads.maxcores
	.set	avdsp_biquads.maxtimers,0
	.globl	avdsp_biquads.maxtimers
	.set	avdsp_biquads.maxchanends,0
	.globl	avdsp_biquads.maxchanends
.Ltmp3:
	.size	avdsp_biquads, .Ltmp3-avdsp_biquads

	.typestring avdsp_biquads, "f{sll}(si,p(u(avdsp_biquad_coef_u){m(c){a(6:si)},m(padding8){sll}}),p(u(avdsp_biquad_states_u){m(padding8){sll},m(s){a(6:si)}}),ui)"

#else
#error
#endif
