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
#define AVDSP_MANT64 (AVDSP_MANT+31)


//transfer mode for samples
#define AVDSP_TRANSFER_SAMPLES (0)  //0: do nothing, 1 transfer from memory to memory(same tile), 2 use channel (across tiles)

//number of 32 bits samples exchanged between the audio task and the avdsp main task
#define AVDSP_NUM_DACOUT    (2)     //samples to be sent to the DAC
#define AVDSP_NUM_USBIN     (2)     //samples to be sent to the USB host
#define AVDSP_NUM_ADCIN     (2)     //samples coming from the ADC
#define AVDSP_NUM_USBOUT    (2)     //samples coming from the usb host
#define AVDSP_NUM_TOTAL     (8)     //sum of the 4 categories above
//minimum sample array size per each of the 4 categories above
#define AVDSP_MIN_SAMPLES   (8)     //zero means no minimum size required

//computing number of samples in the sample array (will be multiplied by 2 to switch A/B)
#define AVDSP_MAX_SAMPLES (AVDSP_MAX( AVDSP_NUM_TOTAL , 4*AVDSP_MIN_SAMPLES ) )
//include or not the avdsp assembly runtime to use user opcodes programs
//1 means both user dsp program + avdsp runtime. 2 means only avdsp
#define AVDSP_RUNTIME 0
//#define AVDSP_RUNTIME_SIZE 1024

#endif /* LAVDSP_CONF_H_ */
