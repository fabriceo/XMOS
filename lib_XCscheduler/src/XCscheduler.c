/**
 * @file XCScheduler.c
 * @version 2.0
 *
 */

#include "XCscheduler.h"
#include <XS1.h>        //for get_logical_core_id()
#include <stdlib.h>     //for malloc


typedef struct s_task {
//dont change the structure order, some offset are used in assembly routines
/* 0 */    unsigned       sp;      //latest Stack pointer in tcb[0]. if null, task will be deallocated
/* 1 */    unsigned       pc;      //adress of the task entrypoint
/* 2 */    unsigned       param;   //value of the param given at task entry in r0
/* 3 */    char *         name;    //name of the task given at task entry in r1
/* 4 */    struct s_task* next;
/* 5 */    struct s_task* prev;
} task_t;


typedef struct s_thread {
//dont change the structure order, some offset are used in assembly routines
/* 0 */    task_t*        current;
/* 1 */    task_t*        main;
} thread_t;

//one task-list per thread/core id, predefined for max 8 core-id
thread_t threadArray[8] = {
        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} };

//create a task by allocating stack and context and adding context at the end of main thread list
unsigned XCSchedulerCreate(const unsigned taskAddress, const unsigned stackSize, const unsigned name, const unsigned param)
{
    thread_t * thread = &threadArray[ get_logical_core_id() ];
    if ( thread->main == 0 ) {    //first creation for this thread
            //allocate a standard task context for the main task
            task_t * tcb = (task_t*)malloc(sizeof(task_t));
            if (tcb == 0) __builtin_trap();
            //initialize main task to point on itself
            thread->main = thread->current = tcb->next = tcb->prev = tcb;
            tcb->param = tcb->pc = 0; tcb->name = "main";
            xcsprintf("main creation @ %x = %d\n",(unsigned)thread->main,(unsigned)thread->main);
    }
    //convert stacksize to bytes and add tcb size
    int alloc = (stackSize+1) * 4 + sizeof(task_t);
    task_t * tcb = (task_t *)malloc( alloc + 4);    //add 4 as SP points on the highest word where lr is stored before stack pointer is changed
    if (tcb == 0) __builtin_trap();
    //add new task at the end of the thread list
    tcb->next = thread->main;
    tcb->prev = thread->main->prev; //equivalent to last
    thread->main->prev = tcb;       //update equivalent to last
    tcb->prev->next = tcb;
    tcb->name = (char*)name;
    tcb->param= param;
    tcb->pc   = taskAddress;
    //compute top of the stack address, pointing on the last word allocated
    int SP = (unsigned)tcb + alloc;
    tcb->sp = SP & ~7;   //force allignement 8. this may reduce SP by one but alloc includs one more
    xcsprintf("create %s, tcb @ %x = %d\n",tcb->name,(unsigned)tcb,(unsigned)tcb);
    return (unsigned)tcb;
}

unsigned XCSchedulerYieldDelay(int max) {
    unsigned res;
    int time = XC_SET_TIME(max);
    do  res  = XCSchedulerYield();
    while  ( ! XC_END_TIME(time) );
    return res;
}

inline unsigned yield();
inline unsigned yieldDelay(int max);
