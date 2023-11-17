/**
 * @file XCScheduler.h

 *
 * @version XMOS 2.0 by fabriceo https://github.com/fabriceo
 *
 * @section License
 * Copyright (C) 2023, fabrice oudert
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#ifndef XCSCHEDULER_H
#define XCSCHEDULER_H

#define xcsprintf(...) // printf(__VA_ARGS__)
#include <stdio.h>      //for printf

#define XC_GET_FUNC_ADDRESS(_f,_n)     asm ("ldap r11," #_f " ; mov %0,r11" : "=r"(_n) :: "r11")
#define XC_GET_FUNC_NSTACKWORDS(_f,_n) asm ("ldc %0,  " #_f ".nstackwords"  : "=r"(_n) )


//helper macros to automatically get the address of the task function AND its stack size.
//works in both XC and cpp. For cpp task, the name of the task function MUST be declared "extern C"
//second parameter is used to pass a value to the task.
//the task name is stored and also passed to the task as an optional parameter
#define XCSchedulerCreateTaskParam(_x,_y) \
        { const char name[] = #_x; \
          unsigned addr;  XC_GET_FUNC_ADDRESS(_x,addr); \
          unsigned stack; XC_GET_FUNC_NSTACKWORDS(_x,stack); \
          XCSchedulerCreate( addr, stack, (unsigned)&name, (_y) ); }

#define XCSchedulerCreateTask(_x) XCSchedulerCreateTaskParam(_x,0)

#ifndef EXTERN
#ifdef __cplusplus
#define EXTERN extern "C"
#else  //stdc or xc
#define EXTERN
#endif
#endif

//prototypes
//add a task function in the list for the current thread, and allocate a stack
EXTERN unsigned XCSchedulerCreate(const unsigned taskAddress, const unsigned stackSize, const unsigned name, const unsigned param);
//switch to the next task into the list
EXTERN unsigned XCSchedulerYield();
//switch to the next task into the list during max cpucycles
EXTERN unsigned XCSchedulerYieldDelay(int max);

//shortcuts
static inline unsigned yield()                  {  return XCSchedulerYield(); }
static inline unsigned yieldDelay(int max)      {  return XCSchedulerYieldDelay(max); }

#ifndef XC_GET_TIME_
#define XC_GET_TIME_
static inline int XC_GET_TIME()      { int time; asm volatile("gettime %0":"=r"(time)); return time; }
static inline int XC_SET_TIME(int x) { int time; asm volatile("gettime %0":"=r"(time)); time+= x; return time;}
static inline int XC_END_TIME(int x) { int time; asm volatile("gettime %0":"=r"(time)); time-= x; return (time>=0); }
#endif



//test presence of a token or data in a given channel, non blocking code
static inline int XCStestChan(unsigned res)
{ asm volatile (
        "\n\t   ldap r11, .Levent%= "          //get address of temporary label below
        "\n\t   setv res[%0], r11 "            //set resource vector
        "\n\t   eeu  res[%0]"                  //default result to 1 and enable resource event
        "\n\t   ldc %0, 1"                     //default result to 1 and enable resource event
        "\n\t   setsr 1"                       //enable events in our thread
        "\n\t   nop"                           //same as in XC select default case
        "\n\t   ldc %0, 0"                     //result forced to 0 if no events
        "\n\t   clre"                          //clear all enable flags
        "\n  .Levent%=:"                       //event entry point
        : "=r"(res) : : "r11" );               //return result
    return res; }
#endif

#ifdef __XC__
static inline int XCStestStreamingChanend( streaming chanend ch ) {
    //this extra mov x,y instruction simplifies type casting...
    unsigned uch; asm ("mov %0,%1":"=r"(uch):"r"(ch)); return XCStestChan(uch); }

static inline int XCStestChanend( chanend ch ) {
    unsigned uch; asm ("mov %0,%1":"=r"(uch):"r"(ch)); return XCStestChan(uch); }
#else
static inline int XCStestChanend( unsigned ch ) { return XCStestChan(ch); }
#endif


