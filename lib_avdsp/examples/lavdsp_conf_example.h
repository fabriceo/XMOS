/*
 * lavdsp_conf.h
 *
 *  Created on: 20 févr. 2025
 *      Author: fabriceo
 */

#ifndef LAVDSP_CONF_H_
#define LAVDSP_CONF_H_

//total number of tasks allowed for dsp processes
#define AVDSP_TASKS_MAX 6

//value for precision in fixed point math aritmetic
#define AVDSP_MANT (28)

//include or not the avdsp assembly runtime to use opcodes programs
#define AVDSP_RUNTIME 0
#define AVDSP_RUNTIME_SIZE 1024

#endif /* LAVDSP_CONF_H_ */
