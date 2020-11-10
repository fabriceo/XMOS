#ifndef DSP_TONE_H
#define DSP_TONE_H

#if defined(__XS1__) || defined(__XS2A__)

static inline int dspToneCalc(int sample, int coef[], int state[], const int mant) {
    // two first order biquad , for each left & right channel
    int h, l, res, a1, b0, b1, Xn1, Yn, Yn1, Xn = sample;
    sample >>= (32-mant);   // reduce resolution
    asm("ldd %0,%1,%2[2]":"=r"(h),  "=r"(l): "r"(state));
    asm("ldd %0,%1,%2[0]":"=r"(b1), "=r"(b0): "r"(coef));
    asm("ldd %0,%1,%2[0]":"=r"(Yn1),"=r"(Xn1):"r"(state));
    asm("maccs %0,%1,%2,%3":"=r"(h),"=r"(l):"r"(Xn), "r"(b0),"0"(h),"1"(l));
    asm("maccs %0,%1,%2,%3":"=r"(h),"=r"(l):"r"(Xn1),"r"(b1),"0"(h),"1"(l));
    asm("ldd %0,%1,%2[1]":"=r"(b0),"=r"(a1):"r"(coef));
    asm("maccs %0,%1,%2,%3":"=r"(h),"=r"(l):"r"(Yn1),"r"(a1),"0"(h),"1"(l));
    asm("std %0,%1,%2[2]"::"r"(h), "r"(l),"r"(state));
    asm("lextract %0,%1,%2,%3,32":"=r"(Yn):"r"(h),"r"(l),"r"(mant));
    asm("std %0,%1,%2[0]"::"r"(Yn), "r"(Xn),"r"(state));

    asm("ldd %0,%1,%2[3]":"=r"(h),"=r"(l):"r"(state));
    asm("maccs %0,%1,%2,%3":"=r"(h),"=r"(l):"r"(Yn),"r"(b0),"0"(h),"1"(l));
    asm("ldd %0,%1,%2[2]":"=r"(a1),"=r"(b1):"r"(coef));
    asm("ldd %0,%1,%2[1]":"=r"(Yn1),"=r"(Xn1):"r"(state));
    asm("maccs %0,%1,%2,%3":"=r"(h),"=r"(l):"r"(Xn1),"r"(b1),"0"(h),"1"(l));
    asm("maccs %0,%1,%2,%3":"=r"(h),"=r"(l):"r"(Yn1),"r"(a1),"0"(h),"1"(l));
    asm("lsats %0,%1,%2":"=r"(h),"=r"(l):"r"(mant),"0"(h),"1"(l));
    asm("std %0,%1,%2[3]"::"r"(h), "r"(l),"r"(state));
    asm("lextract %0,%1,%2,%3,32":"=r"(res):"r"(h),"r"(l),"r"(mant));
    asm("std %0,%1,%2[1]"::"r"(res), "r"(Yn),"r"(state));

return res;
}

#else

#if defined(__XC__)
extern int dspToneCalc(int sample, int coef[], int state[], const int mant);
#else
extern int dspToneCalc(int sample, int * coef, int * state, const int mant);
#endif

#endif


extern void dspToneCaclCoefs(float fs, float freqLo, float gainLo, float freqHi, float gainHi, const int mant);

extern float dspToneLoudness(
       float volref,     // reference volume
       float range,      // say 20db
       float vol,        // actual volume on Knob
       float gainmax );

extern int __attribute__((aligned(8)))  tonecoef[6];
extern int __attribute__((aligned(8)))  tonestate[6*2];


#endif
