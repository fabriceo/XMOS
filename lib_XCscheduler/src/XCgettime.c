/*
 * XCgettime.c
 *
 *  Created on: 27 oct. 2023
 *      Author: Fabrice
 */

#include "XCgettime.h"
#include <XS1.h>


typedef __attribute__((aligned(8))) union XC64_u {
    long long ll;
    unsigned long long ull;
    struct lh_s {
        long lo;
        long hi; } lh;
} XC64_t;

typedef XC64_t XCtime_t;

static volatile XCtime_t XCtime;


//this inline function can replace some of the xmos lib_swlock function
inline unsigned XCswlock( unsigned * lock )  {
    volatile unsigned * l = lock;
    unsigned ID = get_logical_core_id() + 1;
    do { while ( *l ) { };
         *l = ID; //wait for a free lock and try to acquire
         //delay to cope with potential low-priority threads
         asm volatile("nop;nop;nop;nop;nop;nop;nop");
       } while( *l != ID );
    return ID;
}

//macro to create a thread-safe code section { } with setting/waiting a given volatile mutex
#define XC_SWLOCK(lock) for ( int x=1, y=XCswlock((unsigned*)&lock); x && (y==y) ; x=0, lock=0 )


//read XC time counter and increment anoter 32 bit msb when the timer rolls over
long long XCgetTime() { asm volatile("#XCgetTime:");
    static volatile unsigned lock;
    XCtime_t previous;
    int newtime;
    XC_SWLOCK(lock) {
        newtime  = XC_GET_TIME();
        previous = XCtime;
        if ((previous.lh.lo - newtime) > 0 ) previous.lh.hi++;
        XCtime.ll = previous.ll; }
    return previous.ll;
}

#define MUL_2P32_DIV_100    42949673
#define MUL_2P40_DIV_100000 10995116

//return the real time timer value divided by 100.
//return as "int" is a choice in order to be abble to compare futur and actual easily
int XCmicros(){ asm volatile ("#XCmicros:");
    XCtime_t local = { .ll = XCgetTime() };
    unsigned msb, reg, res, z = 0;
    asm("lmul %0,%1,%2,%3,%4,%5":"=r"(msb),"=r"(reg):"r"(local.lh.lo),"r"(MUL_2P32_DIV_100),"r"(z),"r"(z));
    asm("lmul %0,%1,%2,%3,%4,%5":"=r"(reg),"=r"(res):"r"(local.lh.hi),"r"(MUL_2P32_DIV_100),"r"(z),"r"(msb));
    return res;
}


int XCmillis(){ asm volatile ("#XCmillis:");
    XCtime_t local = { .ll = XCgetTime() };
    unsigned msb, reg, res, z = 0;
    local.ull >>= 8;
    asm("lmul %0,%1,%2,%3,%4,%5":"=r"(msb),"=r"(reg):"r"(local.lh.lo),"r"(MUL_2P40_DIV_100000),"r"(z),"r"(z));
    asm("lmul %0,%1,%2,%3,%4,%5":"=r"(reg),"=r"(res):"r"(local.lh.hi),"r"(MUL_2P40_DIV_100000),"r"(z),"r"(msb));
    return res;
}

long long XCgetTime();
int XCmicros();
int XCmillis();

