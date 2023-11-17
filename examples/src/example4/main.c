/*
 * main.xc
 *
 *  Created on: 8 nov. 2023
 *      Author: Fabrice
 */

#include "ArduinoAPI.h"

int main() {

            setup();
            while (1) {
                loop();
                yield();
            }

}
