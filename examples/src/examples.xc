/*
 * examples.xc
 *
 *  Created on: 8 févr. 2020
 *      Author: fabrice
 */

#include <stdio.h>
#include <stdlib.h>
#include <xs1.h>

void task1(){
    printf("hello world\n");
    exit(1);
}

int main(){

    par {
        task1();
    }
    return 0;
}
