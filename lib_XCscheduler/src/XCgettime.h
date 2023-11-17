/*
 * XC_gettime.h
 *
 *  Created on: 27 oct. 2023
 *      Author: Fabrice
 */

#ifndef XCGETTIME_H_
#define XCGETTIME_H_

#ifndef EXTERN
#ifdef __cplusplus
#define EXTERN extern "C"
#else  //stdc or xc
#define EXTERN
#endif // __cplusplus
#endif // ifndef EXTERN


#ifndef XC_GET_TIME_
#define XC_GET_TIME_
static inline int XC_GET_TIME()      { int time; asm volatile("gettime %0":"=r"(time)); return time; }
static inline int XC_SET_TIME(int x) { int time; asm volatile("gettime %0":"=r"(time)); time+= x; return time;}
static inline int XC_END_TIME(int x) { int time; asm volatile("gettime %0":"=r"(time)); time-= x; return (time>=0); }
#endif

EXTERN long long XCgetTime();
EXTERN int XCmicros();
EXTERN int XCmillis();


#endif /* XCGETTIME_H_ */
