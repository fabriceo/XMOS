/*
 * avdsp_filter.c
 *
 *  Created on: 18 févr. 2025
 *      Author: fabriceo
 */


#include "lavdsp_base.h"



void avdspSetFilter(int num, int type, float freq, float Q, float gain, float freq2, float Q2) {

}

void avdspCalcFilters(int num, int count) {

}


//compute all filters and return total number of sections
int avdsp_calcFiltersFS(avdsp_filter_def_t * bqdef, avdsp_biquad_coef_t * bqcoef, unsigned sections, unsigned FS){
    return 0;   //number of effective sections
}
