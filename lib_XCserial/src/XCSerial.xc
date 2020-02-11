/*
 * XCSerial.xc


 *
 *  Created on: 13 juin 2019
 *      Author: Fabriceo
 */

#include "XS1.h"
#include "print.h"
#include "stdio.h"
#include "XCSerial.h"




enum uartfast_state {
  RX_NONE,
  RX_AWAITING,
  RX_DELAYING,
  RX_RECEIVING,
  TX_NONE,
  TX_AWAITING,
  TX_DELAYING,
  TX_SENDING
};

enum IR_state {
  IR_NONE,
  IR_AWAITING,
  IR_RECEIVING
};

[[combinable]]
void XCSerial(port ?pRX, port ?pTX,
              unsigned maskRX, unsigned maskTX, unsigned maskRTS,
              unsigned bauds,
              static const unsigned sizeRX, static const unsigned sizeTX
              ,server XCSERIAL_IF(?uif)
          ) {

    /** fifo buffers declaration **/
    unsigned char bufRX[sizeRX];
    unsigned char bufTX[sizeTX];
    unsigned finRX=0, finTX=0, foutRX=0,foutTX=0;


    int port1bitRX = 0, port1bitTX = 0, samePortRXTX = 0;
    int oldRX; // previous value of pRX, to detect changes and start bit

    enum uartfast_state stateRX = RX_NONE;
    if (!isnull(pRX)) {
        pRX :> oldRX;
        stateRX = RX_AWAITING;
        unsafe { port1bitRX = (((unsigned) pRX >>16) == 1); }
        if (port1bitRX) maskRX = 1;
    }

    enum uartfast_state stateTX = TX_NONE;
    if (!isnull(pTX)) {
        stateTX = TX_AWAITING;
        unsafe { port1bitTX = (((unsigned) pTX >>16) == 1); }
        if (port1bitTX) { maskTX = 1; pTX <: 1; }
    } else samePortRXTX = (maskTX != 0);

    enum uartfast_state nextInt = stateTX;

    int dt  = XS1_TIMER_HZ/bauds; // one bit time in core clock count

    timer timeRXTX; // the single timer used to trigger all RX or TX I/O
    int tRX,tTX, nextT; // used to control timing event
    timeRXTX :> nextT;
    nextT += dt; // initialize for next timer event in onebit of now

    int bitNumRX = 0;
    int dataRX;
    int bitNumTX = 0;
    int dataTX;

    /* infrared handling
     * storing each pin change timestamp into an array
     * */

#ifndef MUL_2P32_DIV_100
#define MUL_2P32_DIV_100 42949673
#endif

    int maskIR = 0; // bit where we potentially have an IR receiver like 38khz
#define bufIRsize 100
    int bufIR[bufIRsize];
    int bufIRin = 0, bufIRout = 0;
    enum IR_state stateIR = IR_NONE;
    int timeIRprev;
#define timeIRmax 500000 // 5 milisecond
    int frameIR = 0;

    // XMODEM definitions
    #define XM_BLK_SIZE    128
    #define XM_MAX_RETRIES 5
    #define SOH         0x01
    #define EOT         0x04
    #define ACK         0x06
    #define NAK         0x15
    #define CAN         0x18
    #define START       'C'

    enum SMstates SMstate = SM_STOP;

    int SMtimeoutMax = 0;
    int SMtimeout;
    int SMblockNum;
    int SMretries;
    int SMindex;
    int SMcrc16;
    int SMblockSize;
    int SMansMax;
    int SMansEnd;
    unsigned char * alias SMbufPtr;

    while (1) {

        select{

            case timeRXTX when timerafter(nextT) :> void: // event here every uart bit time


                if (nextInt == RX_RECEIVING){

                    int p;
                    pRX :> p; oldRX = p; // read and remember current port value

                    tRX  = nextT + dt; // move our RX timer for producing next event in one bit of now

                    if ((bitNumRX < 8) && ((p & maskRX) != 0)) {
                         dataRX |= (1<<bitNumRX); }

                    bitNumRX++;
                    if (bitNumRX >= 9) { // stop bit received previously
                        //store in fifo
                        unsigned newIn = finRX + 1;
                        if (newIn >= sizeRX) newIn = 0;
                        if (newIn != foutRX) {
                             bufRX[finRX] = dataRX;
                             finRX = newIn; }

                         stateRX = RX_AWAITING;
                         if (stateTX == TX_DELAYING) {// we where delaying transmission due to halfduplex
                             stateTX = TX_AWAITING;
                             tTX = tRX; }
                    }

                    // end of cycle, lets check the next timer event principle
                    if ( ( (stateTX == TX_NONE) || (stateTX == TX_DELAYING) ) && (stateRX == RX_RECEIVING) ) {
                        nextT = tRX; // just comme back here in one bit time
                    } else {
                        nextInt = stateTX;
                        nextT = tTX; }

                } // RX_RECEIVING
                else {

                if (stateTX == TX_AWAITING){
                    tTX = nextT + dt;
                    /** check if available data in fifo **/
                    if (finTX != foutTX){
                        /** lets go for a start bit **/
                        if (samePortRXTX) {
                            stateRX = RX_DELAYING; //  we are halfduplex because TX RX using the same port
                            int val;
                            val = peek(pRX); val &= (~maskTX) | maskRTS; // the TX bit will become 0
                            pRX  <: val;
                            oldRX = val;
                        } else {
                            if (port1bitTX) {
                                pTX  <: 0 ;
                            } else {
                                int val;
                                val = peek(pTX); val &= (~maskTX);
                                pTX  <: val ;
                            }
                        }
                        /** get data from fifo buffer **/
                        dataTX = bufTX[foutTX];
                        bitNumTX = 0;
                        stateTX = TX_SENDING;
                    }
                } else
                if (stateTX == TX_SENDING){
                    tTX = nextT + dt;
                    int bit;
                    if (bitNumTX < 8) {
                        bit = (dataTX & (1<< bitNumTX));
                    } else { bit = maskTX; } // stop bit
                    if (samePortRXTX) {
                        int val;
                        pRX :> val; val &= (~maskTX);
                        if (bit) val |= maskTX;
                        pRX <: val ;
                        oldRX = val;
                    } else {
                        if (port1bitTX) {
                            if (bit) pTX <: 1 ; else pTX <:0 ;
                        } else {
                            int val;
                            pTX :> val; val &= (~maskTX);
                            if (bit) val |= maskTX;
                            pTX <: val ;
                        }
                    }
                    bitNumTX++;
                    if (bitNumTX > 8){
                        foutTX++; if (foutTX >= sizeTX) foutTX = 0;
                        stateTX = TX_AWAITING;
                        if (stateRX == RX_DELAYING) stateRX = RX_AWAITING;
                    }

                } // if TX_SENDING

                // end of cycle, lets check for potential receiving state
                if (stateRX == RX_RECEIVING) {
                    nextInt = stateRX;
                    nextT = tRX;
                } else nextT = tTX; // rearm the event timer in one bit of now

                } // if nextInt = RX_RECEIVING


                if (stateIR == IR_RECEIVING) {

                    int temp = nextT - timeIRprev;
                    if (temp > timeIRmax) {
                        frameIR ++;
                        stateIR = IR_AWAITING; }
                }

                // xmodem
                switch (SMstate) {

                case SM_WAITING_START:{
                    if ((nextT - SMtimeout) > SMtimeoutMax)  SMstate = SM_FAILED;
                    else
                    if (finRX != foutRX){   // char received
                        if (bufRX[foutRX] == START) {
                            SMretries = XM_MAX_RETRIES;
                            SMblockNum = 0;
                            SMblockSize -= XM_BLK_SIZE;
                            SMcrc16 = 0; //? or poly
                            SMstate = SM_SENDING_SOH;
                        } else
                            if (bufRX[foutRX] == NAK)
                                SMstate = SM_FAILED;
                            else {
                                foutRX++;
                                if (foutRX >= sizeRX) foutRX = 0; }
                    }
                } break;

                case SM_SENDING_SOH : {
                    if (SMretries--) {
                        bufTX[0] = SOH;
                        bufTX[1] = SMblockNum;
                        bufTX[2] = ~SMblockNum;
                        finTX  = 3;
                        foutTX = 0;
                        SMindex = 0;
                        SMstate = SM_SENDING_BLOCK;
                    } else
                        SMstate = SM_FAILED;
                } break;

                case SM_SENDING_BLOCK:{
                    if ( foutTX == 3 ) {    // header sent
                        if (SMindex < XM_BLK_SIZE) {
                            int pos = SMblockNum*XM_BLK_SIZE + SMindex++;
                            unsigned char ch;
                            asm("ld8u %0, %1[%2]":"=r"(ch):"r"(SMbufPtr),"r"(pos));
                            bufTX[2] = ch;
                            foutTX = 2;
                            //addcrc
                        } else {
                            bufTX[4] = SMcrc16 & 0xFF;
                            bufTX[5] = SMcrc16 >>8;
                            foutTX = 4;
                            finTX = 6;
                        }
                    } else
                        if ( foutTX == 6 ) {    // crc sent
                            SMtimeout = nextT;
                            SMstate = SM_WAITING_ACK;
                        }
                } break;

                case SM_WAITING_ACK:{
                    if ((nextT - SMtimeout) > SMtimeoutMax)
                         SMstate = SM_SENDING_SOH;
                    else
                    if (finRX != foutRX){
                        if (bufRX[foutRX] == ACK) {
                            if (SMblockSize > 0){
                                SMblockSize -= XM_BLK_SIZE;
                                SMblockNum++;
                                SMretries = XM_MAX_RETRIES;
                                SMstate = SM_SENDING_SOH;
                            } else
                                if (finTX == 6) {
                                    bufTX[6] = EOT;
                                    finTX = 7;
                                    SMretries = 0;
                                    SMtimeout = nextT;
                                } else
                                    SMstate = SM_COMPLETED;
                        } else
                            if (bufRX[foutRX] == NAK)
                                SMstate = SM_SENDING_SOH;
                            else {
                                foutRX++;
                                if (foutRX >= sizeRX) foutRX = 0; }
                    }
                } break;

                // sending receiving commands
                case SM_SENDING: {
                    if (finTX == foutTX) {
                        finRX = 0; foutRX = 0;
                        SMtimeout = nextT;
                        SMstate = SM_WAITING;
                    }
                } break;

                case SM_WAITING: {
                    if (finRX != foutRX) SMstate = SM_RECEIVING;
                    if ((nextT - SMtimeout) > SMtimeoutMax)
                         SMstate = SM_TIMEOUT;
                } break;

                case SM_RECEIVING: {
                    if (finRX >= SMansMax) SMstate = SM_RECEIVED;
                    if (finRX)
                        if (SMansEnd>=0)
                            if (bufRX[finRX] == SMansEnd) SMstate = SM_RECEIVED;
                    if ((nextT - SMtimeout) > SMtimeoutMax) SMstate = SM_TIMEOUT;
                } break;
                }

                break; // case timerRXTX

            /** detect changes on pRX and store pRX in p **/
            case pRX when pinsneq(oldRX) :> int p : {

                int change = (p ^ oldRX); oldRX = p;

                /** check if the change is related to the pin RX **/
                    if (port1bitRX | (change & maskRX)) {
                        /** is this a 0 start bit **/
                        if ((stateRX == RX_AWAITING) && (( p & maskRX) == 0)) {
                            timeRXTX :> tRX; // our reference time starts now
                            tRX += dt + (dt>>1); // next event at middle of next bit
                            bitNumRX = 0; dataRX = 0;
                            stateRX = RX_RECEIVING;

                            if (stateTX == TX_SENDING) {
                                // a transmission is ongoing and will trigger an event soon
                                // the nextT value will be calculated by TX section based on our RX status
                            } else
                            if (stateTX == TX_AWAITING) {
                                if (samePortRXTX) stateTX = TX_DELAYING; // we are halfduplex due to same port
                                else { tTX = tRX + (dt>>1); }  // next transmission to be checked in 2 bits of now
                                // then the next timer event is for us in 1.5 bit
                                nextT = tRX;
                                nextInt = stateRX;  // now we are sure the next int is for us
                            } // stateTX
                        } // startbit
                    } else { // this is not a RX change

                        // treat other input changes here

                        if (stateIR != IR_NONE){ // IR reception is activated

                            if (change & maskIR) { // changes detected on the IR pin
                                p &= maskIR; // new value of the IR
                                int temp;
                                timeRXTX :> temp;
                                if (stateIR == IR_AWAITING) {
                                    if (p == 0){ // new MARK received ( = 0volt)
                                        stateIR = IR_RECEIVING; }
                                }
                                // compute time in microseconds by using 64 bit mul and keeping upper 32 bits only
                                timeIRprev = temp;
                                {temp, p} = lmul(temp, MUL_2P32_DIV_100, 0, 0);
                                if (temp == 0) { temp = 1; } // because 0 is marking the end of a frame in our buffer
                                bufIR[bufIRin] = temp;

                                temp = bufIRin +1;
                                if (temp > bufIRsize) temp = 0;
                                if (temp != bufIRout) bufIRin = temp;
                                bufIR[bufIRin] = 0; // End of Frame placeholder
                                } // IR change detected
                            } // stateIR

                    } // RX changes

            } break;


            case (!isnull(uif)) => uif.xmodemSend(unsigned char * alias buf, const int size, const int timeoutMax ): {
                SMbufPtr = buf;
                SMtimeoutMax = timeoutMax;
                SMblockSize = size;
                SMtimeout = nextT;
                finRX = 0; foutRX = 0;
                SMstate = SM_WAITING_START;    // wait for start
            } break;

            case (!isnull(uif)) => uif.xmodemStatus() -> int res: {
                res = SMstate;
            } break;

            case (!isnull(uif)) => uif.xmodemStop() : {
                SMstate = SM_STOP;
                finTX = 0; foutTX = 0; finRX = 0; foutRX = 0;
            } break;

            case (!isnull(uif)) => uif.answerStart(int timeout, int max, int end) : {
                SMtimeoutMax = timeout;
                SMansMax = max;
                SMansEnd = end;
                SMstate = SM_SENDING;
            } break;

            case (!isnull(uif)) => uif.answerStatus() -> int res : {
                res = SMstate;
            } break;


            /** interface handling **/
            case (!isnull(uif)) => uif.gpioRX() -> int res:
                if (!isnull(pRX)) {
                int val;
                pRX :> val;
                res = val; }
                else res = 0;
                break;


            case (!isnull(uif)) => uif.gpioTX(int bit, int value) :
                if (stateTX == TX_NONE) break;
                int val, mask;
                mask = 1<<bit;
                if (samePortRXTX) {
                    val = peek(pRX);
                    if (value) { val |= mask; } else { val &= ~mask; }
                    pRX <: val;
                } else {
                    if (port1bitTX) {
                        if (value) pTX <: 1; else pTX <:0;
                    } else {
                        val = peek(pTX);
                        if (value) { val |= mask; } else { val &= ~mask; }
                        pTX <: val;
                    }
                }
                break;


            case (!isnull(uif)) => uif.writeChar(unsigned char ch):
                if (SMstate) break;
                unsigned newIn = finTX + 1;
                if (newIn >= sizeTX) newIn = 0;
                if (newIn == foutTX) break; // buf full, exit
                bufTX[finTX] = ch;
                finTX = newIn;
                break;

            case (!isnull(uif)) => uif.readChar() -> unsigned char ch:
                if (SMstate) break;
                if (finRX == foutRX) break; // nothing to read
                ch = bufRX[foutRX];
                foutRX++; if (foutRX >= sizeRX) foutRX = 0;
                break;

            case (!isnull(uif)) => uif.peekChar() -> unsigned char ch:
                ch = bufRX[foutRX];
                break;

            case (!isnull(uif)) => uif.numCharIn() -> int res:
                if (SMstate) res = 0;
                else
                if (finRX >= foutRX) { res = finRX - foutRX;
                } else { res = sizeRX-(foutRX-finRX)-1; }
                break;

            case (!isnull(uif)) => uif.flushIn():
                if (SMstate) break;
                foutRX = finRX;
                break;

            case (!isnull(uif)) => uif.numCharOut() -> int res:
                if (SMstate) res = 0;
                else
                 if (finTX >= foutTX) { res = finTX - foutTX;
                 } else { res = sizeTX-(foutTX-finTX)-1; }
                 break;

            case (!isnull(uif)) => uif.sizeLeftOut() -> int res:
                if (SMstate) res = 0;
                else
                 if (finTX >= foutTX) { res = sizeTX - (finTX - foutTX);
                 } else { res = (foutTX-finTX) - 1; }

                 break;

            case (!isnull(uif)) => uif.setBaud(int baudrate):
                dt  = XS1_TIMER_HZ/baudrate;
                timeRXTX :> nextT; nextT += dt;
                break;

            case (!isnull(uif)) => uif.setMaskIR(int mask):
                if (mask) {
                    maskIR = mask;
                    bufIRin = 0;
                    bufIRout = 0;
                    bufIR[0] = 0;
                    stateIR = IR_AWAITING;
                } else stateIR = IR_NONE;
                break;

            case (!isnull(uif)) => uif.numFrameIR() -> int res:
                res = frameIR;
                break;

            case (!isnull(uif)) => uif.taskReady() -> int res:
                res = 1;
                break;
        }  // select
    }  // while 1
}

/*
 * interfaces function wrapper, so this can be called from cpp methods
 * maybe there is a better way to do this  ?
 * but the xcore c library doesnt provide solution for calling interface functions
 */

unsigned char XCSerial_read(client interface XCSerial_if uartIF){
    return uartIF.readChar();
}

unsigned char XCSerial_peek(client interface XCSerial_if uartIF){
    return uartIF.peekChar();
}

void XCSerial_write(client interface XCSerial_if uartIF, unsigned char ch){
    uartIF.writeChar(ch);
}

int XCSerial_available(client interface XCSerial_if uartIF){
    return uartIF.numCharIn();
}

void XCSerial_flush(client interface XCSerial_if uartIF){
    uartIF.flushIn();
}

void XCSerial_setBaud(client interface XCSerial_if uartIF, int baudRate){
    uartIF.setBaud(baudRate);
}

#if 0
#include "xud_cdc.h" // import interface definition
#ifndef CDC2

void XCSerial_usb_put_char(client interface usb_cdc_interface usbIF, char byte){

}

char XCSerial_usb_get_char(client interface usb_cdc_interface usbIF){

}

int XCSerial_usb_write(client interface usb_cdc_interface usbIF, unsigned char data[], REFERENCE_PARAM(unsigned, length)){

}

int XCSerial_usb_read(client interface usb_cdc_interface usbIF, unsigned char data[], REFERENCE_PARAM(unsigned, count)){

}

int XCSerial_usb_available_bytes(client interface usb_cdc_interface usbIF){

}

void XCSerial_usb_flush_buffer(client interface usb_cdc_interface usbIF){

}
#endif
#endif

#include "stddef.h"
/* ALREADY OVERLOADED BY XSCOPE ?! */
int unused_write(int fd, const unsigned char *data, size_t len) {
    for(unsigned int i = 0; i < len; i++) { unsigned char ch = data[i]; }
    return len;
}
