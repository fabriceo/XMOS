/*
 * test_xcport.cpp

 *
 *  Created on: 23 juin 2019
 *      Author: Fabrice
 */

#include "XCport.h"

#define EXAMPLE0


#ifdef EXAMPLE1
XCport LED(P1L_0);

EXTERNALC void toggleLED();

void toggleLED(){
    int tog = 0;
    LED.mode(OUTPUT);
    while (1) {
        LED = tog;
        tog = 1-tog;
    }
}
#endif
