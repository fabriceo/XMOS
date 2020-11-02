/*

 * XCSerial.cpp
 *
 *  Created on: 14 juin 2019
 *      Author: Fabriceo
 *      this provides a wrapper for using the XCSerial.xc from a cpp app
 */

#include "XCSerial.h"   //header file
/*
 * this is a wrapper for creating an object class typically compatible with the original Arduino library.
 */

SoftwareSerial::SoftwareSerial() { }; // constructor


void SoftwareSerial::init(unsigned uartIF){
    refuartIF = uartIF;
}

extern "C" { void XCSerial_setBaud(unsigned IF, int baudRate); }

void SoftwareSerial::begin(int baudRate) {
    XCSerial_setBaud(refuartIF,baudRate);
}

void SoftwareSerial::end(){
}

extern "C" { int XCSerial_available(unsigned IF); }

int SoftwareSerial::available(){
    return XCSerial_available(refuartIF);
}

extern "C" { unsigned char XCSerial_read(unsigned IF); }

int SoftwareSerial::read(){
    return XCSerial_read(refuartIF);
}

extern "C" { unsigned char XCSerial_peek(unsigned IF); }

int SoftwareSerial::peek(){
    return XCSerial_peek(refuartIF);
}

extern "C" { void XCSerial_flush(unsigned IF); }

void SoftwareSerial::flush(){
    XCSerial_flush(refuartIF);
}

extern "C" { void XCSerial_write(unsigned IF, unsigned char); }

void SoftwareSerial::write(unsigned char byte){
    XCSerial_write(refuartIF, byte);
}

// SoftwareSerial Serial;

#ifdef CDC

usbSerial::usbSerial() { }; // constructor


void usbSerial::init(unsigned IF){
    usbIF = IF;
}


void usbSerial::begin(int baudRate) {

}

void usbSerial::end(){
}


int usbSerial::available(){

}


int usbSerial::read(){

}


int usbSerial::peek(){

}


void usbSerial::flush(){

}


void usbSerial::write(unsigned char byte){

}

// usbSerial Serial;
#endif


