
#include <math.h>


int __attribute__((aligned(8)))  tonecoef[6] = { 1<<28, 0, 0, 1<<28, 0, 0 };
int __attribute__((aligned(8)))  tonestate[6*2];


// coefs ordering : b0 b1 a1, b0 b1 a1
// lo shelf filter first, hi shelf filter second
void dspToneCaclCoefs(float fs, float freqLo, float gainLo, float freqHi, float gainHi, const int mant)
{
    int q_format = 1 << mant;
    float a0, a1, b0, b1, w0, tw, A;

    if ( (gainLo > 0.1) || (gainLo < -0.1) ) {
        gainLo = pow( 10.0 , gainLo / 20.0);
        w0 = M_PI * 2.0 * freqLo / fs;
        tw = tan( w0 / 2.0 );
        A = sqrt( gainLo );
        a0 = tw + A;
        a1 = ( A - tw ) / a0;
        b0 = ( gainLo * tw + A ) / a0;
        b1 = ( gainLo * tw - A ) / a0;
    } else {
        a1 = 0.0;
        b0 = 1.0;
        b1 = 0.0;
    }
    a1 -= 1.0; // due to mantissa reintegration
    a1 *= q_format;
    b0 *= q_format;
    b1 *= q_format;
    tonecoef[0] = b0; tonecoef[1] = b1; tonecoef[2] = a1;

    if ( (gainHi > 0.1) || (gainHi < -0.1) ) {
        gainHi = pow( 10.0, gainHi / 20.0);
        w0 = M_PI * 2.0 * freqHi / fs;
        tw = tan( w0 / 2.0 );
        A = sqrt( gainHi );
        a0 = A * tw + 1.0;
        a1 = -( A * tw - 1.0  ) / a0;
        b0 =  ( A * tw + gainHi ) / a0;
        b1 =  ( A * tw - gainHi ) / a0;
    } else {
        a1 = 0.0;
        b0 = 1.0;
        b1 = 0.0;
    }
    a1 -= 1.0; // due to mantissa reintegration
    a1 *= q_format;
    b0 *= q_format;
    b1 *= q_format;
    tonecoef[3] = b0; tonecoef[4] = b1; tonecoef[5] = a1;
}


// return a new gain base on params, all value in decibell
float dspToneLoudness(float volref,     // reference volume
                      float range,      // say 20db
                      float vol,        // actual volume on Knob
                      float gainmax ) { // bass or trebble maximum overshoot
    // vol and volref extepected to be negative (or zero)
    if (vol >= 0.0)     return 0.0;
    if (volref >= 0.0 ) return 0.0;
    if (range <= 0.0)   return 0.0;
    if ((volref + range) >= 0.0) range = - volref;  // another sanity check
    float delta = vol - volref;
    if (delta <= 0.0 )  return gainmax;       // simplest
    if (delta >= range) return 0.0;
    if (gainmax >= range) gainmax = range;   // sanity check
    float factor = (range - delta) / range;
    return gainmax * factor;
}


// need verification
static inline void dspToneSaturate(long long *a , int mant){
    long long satpos = (1ULL << (mant*2)) - 1;
    long long satneg = -satpos;
    if ((*a) >= satpos ) *a = satpos;
    else
    if ((*a) < satneg)   *a = satneg;
    (*a) >>= mant;
}

// to be verified
int dspToneCalc(int sample, int * coef, int * state, const int mant){
    sample >>= (32-mant);
    int b0 = coef[0]; int b1 = coef[1]; int a1 = coef[2];
    int Xn1 = state[0];
    int Yn1 = state[1];
    long long res = ((long long)state[5] << 32) | state[4];
    res = (long long)sample * b0;
    res += (long long)Xn1 * b1;
    res += (long long)Yn1 * a1;
    int Xn = (res >> mant);
    state[4] = res & 0xFFFFFFFF;
    state[5] = res >> 32;
    state[4] = res & 0xFFFFFFFF;
    state[0] = sample;
    state[1] = Xn;
    res += ((long long)state[7] << 32) | state[6];
    b0 = coef[3]; b1 = coef[4]; a1 = coef[5];
    Xn1 = state[2];
    Yn1 = state[3];
    res += (long long)Xn  * b0;
    res += (long long)Xn1 * b1;
    res += (long long)Yn1 * a1;
    dspToneSaturate(&res, mant);
    state[6] = res & 0xFFFFFFFF;
    state[7] = res >> 32;
    res >>= mant;
    state[2] = Xn;
    Xn = res & 0xFFFFFFFF;
    state[3] = Xn;
    return Xn;
}
