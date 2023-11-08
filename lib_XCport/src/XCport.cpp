/*
 * XCPort.cpp

 *
 *  Created on: 21 juin 2019
 *      Author: Fabriceo
 */


#include "XCport.h"

// method for class XCport

void XCport::write(int val){
    if (pp>=0) digitalWrite(pp, val);
}

int XCport::read() {
    if (pp>=0) {
        prevValue = value;
        value = digitalRead(pp);
        return value;
    } else
        return 0;
}
void XCport::mode(int val){
    if (pp<0) pp = -pp;
    pinMode(pp, val);
}

//1 = change, 0 = no change
int XCport::change(){
    if (read() != prevValue) return 1;
    else return 0;

}

//2 = falling, 3 = rising, 0 = no change
int XCport::falling(){
    if (value != prevValue)
        if (value == 0) return 2;
     return 0;

}
//2 = falling, 3 = rising, 0 = no change
int XCport::rising(){
    if (value != prevValue)
        if (value) return 3;
    return 0;
}
