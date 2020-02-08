/**
 * @file Scheduler.h

 *
 * @version XMOS 1.0 by fabriceo https://github.com/fabriceo
 * ALL credits to mikael patel! https://github.com/mikaelpatel
 *
 * @section License
 * Copyright (C) 2015-2017, Mikael Patel
 * Copyright (C) 2019, fabrice oudert
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

#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus

class SchedulerClass {
public:

   /**
   * Function prototype for tasks.
   */
  typedef void (*func_t)();

  static void init(unsigned stackPtr, const unsigned stackSize);

  /**
   * Initiate scheduler and main program with given stack size. Should
   * be called at first by the main setup function. Returns true if stack
   * allocation was successful regarding the minimum stack pointer provided
   * @param[in] stackSize in bytes.
   * @param[in] optional (otherwise NULL) value of minimum StackPointer.
   * @return bool.
   */
  static bool setup(const unsigned stackSize);

  /**
   * Start a task with given stack size. Should be
   * called from main program (in setup). Returns true if
   * stack properly allocated  otherwise false.
   * @param[in] task function
   * @param[in] stackSize in bytes.
   * @return bool.
   */
  static bool start(const func_t taskFunction, const unsigned stackSize);


  /**
   * Context switch to next task in run queue.
   */
  static void yield();

  /**
   * Return minimum remaining stack in bytes for running task.
   * The value depends on executed function call depth and interrupt
   * service routines during the execution (so far).
   * @return bytes
   */
  static size_t stackLeft();

protected:

  static bool initTask(func_t taskFunction, unsigned* stackAddr);

  static void initStack(unsigned* stack, unsigned size);

  /**
   * Task run-time structure.
   */
  struct task_t {
    task_t* next;       //!< Next task.
    task_t* prev;       //!< Previous task.
    jmp_buf context;    //!< Task context.
    unsigned* stack;    //!< Task stack botom.
  };

  /** Main task. */
  static task_t s_main;

  /** Running task. */
  static task_t* s_running;

  /** Task stack allocation top. */
  static unsigned s_top;

  /** upper value of the sp register as given when calling the SchedulerInit() from xc program. */
  static unsigned initialStack;

  /** lower value acceptable for the stack pointer within the scheduled tasks. */
  static unsigned lowerStack;
};

/** Scheduler single-ton. */
extern SchedulerClass XCScheduler;

/**
 * Syntactic sugar for scheduler based busy-wait for condition;
 * yield until condition is valid. May require volatile condition
 * variable(s).
 * @param[in] cond condition to await.
 */
#define await(cond) while (!(cond)) Scheduler.yield()

extern "C" {
void yield();
void XCSchedulerInit(const unsigned stackSize);
}
#endif //__cplusplus

#ifdef __XC__
/*
 * this is the key function for switching the cooperative scheduler to another task
 * can be called from either a cpp function or a xc function (not task)
 */
//extern void yield();

/*
 * this procedure is to be called imedialtely from the xc task dedicated to this cooperative scheduler
 * the parameter is the size of the stack for all the cooperative tasks, and has to be equal to the
 * #pramga stackfunction value expected just above (*4 as there is 4 bytes by words)
 */
extern void XCSchedulerInit(const unsigned stackSize);
#endif

#endif
