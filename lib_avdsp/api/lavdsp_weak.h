/*
 * lavdsp_weak.h
 *
 *  Created on: 25 févr. 2025
 *      Author: fabriceo
 */

#ifndef LAVDSP_WEAK_H_
#define LAVDSP_WEAK_H_

extern void avdspTask1();
extern void avdspTask2();
extern void avdspTask3();
extern void avdspTask4();
extern void avdspTask5();
extern void avdspTask6();
extern void avdspTask7();
extern void avdspTask8();

extern int  avdspInit();

extern int  avdspSetProgram(unsigned prog);

extern int  avdspChangeFS(unsigned newFS);

extern int avdspSetVolume(unsigned num, const int vol);

extern int avdspSetBiquadCoefs(unsigned num, const float F, const float Q, const float G);

extern unsigned avdspGetChanend();
extern void avdspTimeOverlap();

#endif /* LAVDSP_WEAK_H_ */
