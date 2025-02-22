/*
 * lavdsp_runtime.h
 *
 *  Created on: 21 févr. 2025
 *      Author: fabrice
 */

#ifndef LAVDSP_RUNTIME_H_
#define LAVDSP_RUNTIME_H_

#include "lavdsp.h"

#ifdef __lavdsp_conf_h_exists__
#include "lavdsp_conf.h"
#endif

#ifdef __XC__

#endif


extern int  avdsp_rt_loadCodePage(unsigned page, unsigned buf[16]);
extern void avdsp_rt_task(const int N);

#endif /* LAVDSP_RUNTIME_H_ */
