/*
 * lavdsp_runtime.h
 *
 *  Created on: 21 févr. 2025
 *      Author: fabrice
 */

#ifndef LAVDSP_RUNTIME_H_
#define LAVDSP_RUNTIME_H_

#include "lavdsp.h"

extern int  avdsp_rt_loadCodePage(unsigned page, unsigned buf[16]);
extern void avdsp_rt_task(const int N);

#ifndef __XC__



//list of supported frequencies as index numbers
enum avdspFreqs_e {
    F8000,   F16000, F24000,  F32000, F44100,  F48000,
    F88200,  F96000, F176400, F192000, F352800, F384000, F705600, F768000
};

//search a literal frequency in the list of possible supported frequencies (optimized by compiler)
static inline int avdspFrequencyToIndex(int freq){
    switch (freq) {
    case  8000  : return F8000; break;       case 16000  : return F16000; break;
    case 24000  : return F24000; break;      case 32000  : return F32000; break;
    case 44100  : return F44100; break;      case 48000  : return F48000; break;
    case 88200  : return F88200; break;      case 96000  : return F96000; break;
    case 176400 : return F176400; break;     case 192000 : return F192000; break;
    case 352800 : return F352800; break;     case 384000 : return F384000; break;
    case 705600 : return F705600; break;     case 768000 : return F768000; break; }
    if (freq < 8000) return F8000; else return F768000;
}

static inline int avdspFrequencyFromIndex(enum avdspFreqs_e freqIndex){
    switch (freqIndex) {
    case  F8000  : return 8000; break;      case F16000  : return 16000; break;
    case F24000  : return 24000; break;     case F32000  : return 32000; break;
    case F44100  : return 44100; break;     case F48000  : return 48000; break;
    case F88200  : return 88200; break;     case F96000  : return 96000; break;
    case F176400 : return 176400; break;    case F192000 : return 192000; break;
    case F352800 : return 352800; break;    case F384000 : return 384000; break;
    case F705600 : return 705600; break;    case F768000 : break; }
    return 768000;
}


typedef struct avdsp_rt_s {
    unsigned * dataPtr;         //location of the data space
    unsigned fsIndex;           //index of the current frequency, 0 at fsMin
    unsigned fsIndexMin;        //value of the minimum frequency, in the avdspFreqs_e
    unsigned fsIndexMax;        //value of the maximum frequency, in the avdspFreqs_e
    int      volumeMaster;      //represent the user selected master volume (q31 signed but positive)
    unsigned saturationVolume;
    unsigned saturationFlag;    //temporary 1 when a core is detecting saturation
    unsigned saturationGain;
    unsigned coreToBeUsed;      //bitmask of the cores to be used
    unsigned tpdfPoly;          //polynom for crc32
    unsigned tpdfValue;         //latest value of tpdf calculation
    unsigned tpdfRandom;        //lastet value of random calculation
    unsigned delayLineFactor;   //magic number to easily compute number of samples per microseconds at fs.
    unsigned biquadFreqOffset;  //for predefined coefficient provide a minimum displacement.
    unsigned biquadFreqSkip;    //for predefined coefficient at fs, provide a displacement to skip bulk of coefficients
} avdsp_rt_t;

#endif

#endif /* LAVDSP_RUNTIME_H_ */
