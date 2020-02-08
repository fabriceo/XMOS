/**
 * @file Scheduler.cpp
 * @version 1.5
 *
 * @section License
 * Copyright (C) 2015-2017, Mikael Patel
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


#include "XCscheduler.h"
#include <print.h>
#include <stdio.h>

// Stack magic pattern
const unsigned SP_PATERN = 0xa55a5aa5;

SchedulerClass Scheduler;


// Main task descriptor and run queue list
SchedulerClass::task_t SchedulerClass::s_main = {
  &SchedulerClass::s_main,
  &SchedulerClass::s_main,
  { 0 },
  NULL
};

// Reference running task
SchedulerClass::task_t* SchedulerClass::s_running = &SchedulerClass::s_main;

size_t SchedulerClass::s_top = 0;

size_t SchedulerClass::initialStack = 0;

size_t SchedulerClass::lowerStack = 0;

void SchedulerClass::init(unsigned stackPtr, const unsigned stackSize){
    initialStack = stackPtr;
    lowerStack  = stackPtr - stackSize;
}

bool SchedulerClass::setup(const unsigned stackSize)
{
  if (initialStack == 0) return false; // no initial stackPtr given, shauld call init at first
  s_top = stackSize; // next stack allocation will be sized for the main setup/loop program.

  unsigned var;
  var = (unsigned)&var - stackSize - sizeof(task_t); // on xcore, the SP pointer decrease when entering in function, to alocate space for local data
  return (lowerStack < var); // returns true if the minimum stack pointer for the other xcore is below the requirement for main.
}

bool SchedulerClass::start(const func_t taskFunction, const unsigned stackSize)
{
  // Check called from main task and valid task loop function and at least 512 byte of stack (typically to cover a basic printf)
  if ((s_running != &s_main) || (taskFunction == NULL) || (stackSize < 512) || (s_top == 0) ) return (false);

  // firmly Allocate data space on the stack to secure the PREVIOUS task working area,
  // or the main progam (setup+Loop) if entering here for the first time
  unsigned stack[ (s_top - sizeof(task_t))/sizeof(unsigned)-1];

  // Check that the upcoming task stack can be safely allocated
  if (((unsigned)&stack - stackSize) <= lowerStack) return (false);

  // initialize the main stack start adress and patern
  if (s_main.stack == NULL) {  // first time enterring here
      s_main.stack =  stack;   // this will point on the lower end of the stack of the main program
      initStack(stack, sizeof(stack)); } // initialize the main stack

  s_top += stackSize;

  // create the task context onto the stack and insert this new taskFunction into the chain.
  return initTask(taskFunction, &stack[-(stackSize/sizeof(unsigned))]);
}

/*
 * by calling an extra function for the initialization, we are sure that the task context will be stored before the stack (at a lower memory adress)
 */
bool SchedulerClass::initTask(func_t taskFunction, unsigned* stackAddr) {

    // create a task context, located below the latest stack creation
     task_t task;

     task.next = &s_main;          // point on the main program by default
     task.prev = s_main.prev;
     s_main.prev->next = &task;    // point the previous task pointer onto this new task
     s_main.prev = &task;
     task.stack = stackAddr;       // the real space will be allocated in next call but we know the adress

     // set context for new task, caller will return
     if (setjmp(task.context)) {
          // initialize stack space with a patern to allow detect of stack usage
          initStack(stackAddr, (unsigned)&task - (unsigned)stackAddr - 8); // but protect the task_t record
          taskFunction(); // will be executed at the next longjump
          while (1) { yield(); }; // should not arrive here, unless the task returns, which then will continue to run all other
        }
     return true;
}

void SchedulerClass::initStack(unsigned* stack, unsigned size) {
    int i;
    //printf("initStack %d, size = %d bytes\n", (unsigned)stack, size);
    return;
    size /= sizeof(unsigned);
    for (i=0; i < size ; i++) {
        *(stack++) = SP_PATERN; }
}


void SchedulerClass::yield()
{
  if (s_main.stack == NULL) return; // no any other task initiated yet
  // Caller will continue here on yield
  if (setjmp(s_running->context)) // store current context and leave the "if" statement.
     return; // coming back here after some other task being run, and "return" to the original yield caller.

  // Next task in run queue will continue
  s_running = s_running->next;
  longjmp(s_running->context, true);
}

size_t SchedulerClass::stackLeft()
{
  const unsigned * sp = s_running->stack;
  size_t words = 0;
  while (*sp++ == SP_PATERN) words++;
  return (words * 4); // convert in bytes
}

/* this is a wrapper function for calling the XCScheduler object from a task defined in an .xc file
 *  it first reads the stack pointer position and then call the scheduler init method.
 */
void XCSchedulerInit(const unsigned stackSize) {
    int val;
    asm volatile("ldaw %0, sp[0]":"=r"(val)::"memory"); // load value of sp register into the val memory
    XCScheduler.init(val, stackSize);
}

/* this is the yield procedure to be called in each cpp task in order to fluently
 * switch to another task on a regular basis, and preferably quickly as possible.
 * if used concurently with XCduino library then it will owerite the original weak yield function
 * This can be called either from a .cpp or .xc function
 */
void yield() {  XCScheduler.yield(); }

