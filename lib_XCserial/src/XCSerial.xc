/*
 * XCSerial.xc


 *
 *  Created on: 13 juin 2019
 *      Author: Fabriceo
 */

#include <XS1.h>
#include <print.h>
#include <stdio.h>
#include "XCSerial.h"


#define SET_SHARED_GLOBAL(g, v) asm volatile("stw %0, dp[" #g "]"::"r"(v):"memory")
static inline int getTime() {
    int tmp;
    asm volatile("gettime %0":"=r"(tmp));   // lecture generic timer
    return tmp;
}

enum uartfast_state {
  RX_NONE,
  RX_AWAITING,
  RX_RECEIVING,
  RX_ONHOLD,
  TX_NONE,
  TX_AWAITING,
  TX_SENDING,
  TX_ONHOLD
};

enum IR_state {
  IR_NONE,
  IR_AWAITING,
  IR_RECEIVING
};

[[combinable]]
void XCSerial(port ?pRX, port ?pTX,
              unsigned maskRX, unsigned maskTX, unsigned maskRTS,
              static const int sizeRX, static const int sizeTX
              ,server XCSERIAL_IF(?uif)
          ) {
    //set_thread_fast_mode_on();

    /** fifo buffers declaration **/
    unsigned char bufRX[sizeRX];
    unsigned char bufTX[sizeTX];
    int finRX = 0, finTX = 0, foutRX = 0, foutTX = 0;


    int port1bitRX = 0, port1bitTX = 0, samePortRXTX = 0;
    int oldRX; // previous value of pRX, to detect changes and start bit
    int oldTX=0;  // used only if pTX is not 1 bit port

    enum uartfast_state stateRX = RX_NONE;
    if (!isnull(pRX)) {
        pRX :> oldRX;
        stateRX = RX_ONHOLD;
        unsafe { port1bitRX = (((unsigned) pRX >>16) == 1); }
        if (port1bitRX) maskRX = 1;
    }

    enum uartfast_state stateTX = TX_NONE;
    if (!isnull(pTX)) {
        stateTX = TX_ONHOLD;
        unsafe { port1bitTX = (((unsigned) pTX >>16) == 1); }
        if (port1bitTX) {
            maskTX = 1;
            pTX <: 1;
            set_port_drive(pTX);
        } else {
            unsafe { samePortRXTX =  ((unsigned)pTX == (unsigned)pRX); }
            if (samePortRXTX){
                 set_port_drive_low(pRX); // pullup mode
            } else {
                oldTX = maskTX;
                pTX <: oldTX;
                set_port_drive(pTX);
            }
        }
    } else {
        samePortRXTX = (maskTX != 0);
        if (samePortRXTX) {
             set_port_drive_low(pRX); // pullup mode
             stateTX = TX_ONHOLD; }
    }

    int dt  = XS1_TIMER_HZ/10000; // 10us by default

    timer timeRXTX; // the single timer used to trigger all RX or TX I/O
    int nextT; // used to control timing event

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

    // state machine for xmodem protocol
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

#if defined(PRINTF) && defined (XSCOPE)
    unsigned int localCoreId = get_logical_core_id();
    unsigned int localTileId = get_local_tile_id();
    printf("core %d:%d XCSERIAL task\n",localTileId & 1,localCoreId);
#endif


    timeRXTX :> nextT;
    nextT += dt; // initialize for next timer event in onebit of now

    int told = getTime();
    int tnew = 0;

    while (1) {

        select{

            case timeRXTX when timerafter(nextT) :> void: { // event here every uart bit time

            if (stateIR == IR_RECEIVING) {  // first of the interrupt sequence because using nextT

                int temp = nextT - timeIRprev;
                if (temp > timeIRmax) {
                    frameIR ++;
                    stateIR = IR_AWAITING; }
            }

            nextT += dt;    // by default always move on to the next bit time frame whatever the state machine is

            //switch (state) {
            //case RX_RECEIVING:

            if ( stateRX == RX_RECEIVING )  {

                int p;
                pRX :> p;
                oldRX = p; // read and remember current port value

                if (bitNumRX < 9)
                    if (p & maskRX)  dataRX |= (1<<bitNumRX);
                bitNumRX++;
                if (bitNumRX >= 9) { // stop bit received previously
                    if ((dataRX & 0x100)) { // check proper stopbit
                        //store in fifo
                        int newIn = finRX + 1;
                        if (newIn >= sizeRX) newIn = 0;
                        if (newIn != foutRX) {
                             bufRX[finRX] = dataRX;
                             //printf("x%X",bufRX[finRX]);
                             finRX = newIn; }
                    }
                    // reception complete
                    stateRX = RX_AWAITING;
                    if (stateTX != TX_NONE)  stateTX = TX_AWAITING;
                }

            } else

            //case TX_AWAITING:
            if (stateTX == TX_AWAITING) {

                /** check if available data in fifo **/
                if (finTX != foutTX) {

                    stateTX = TX_SENDING;
                    if (stateRX != RX_NONE) stateRX = RX_ONHOLD;    //force half dupex

                    /** lets go for a start bit **/
                    if (samePortRXTX) {
                        oldRX &= (~maskTX);
                        pRX  <: oldRX;
                    } else
                        if (port1bitTX)  pTX  <: 0 ;
                        else {
                            oldTX &= (~maskTX);
                            pTX  <: oldTX; }
                    //printf("[-");
                    /** get data from fifo buffer **/
                    dataTX = bufTX[foutTX];
                    bitNumTX = 0;
                }

            } else

            if (stateTX == TX_SENDING) {

                int bit;
                if (bitNumTX < 8)  bit = (dataTX & (1<< bitNumTX));
                else bit = maskTX;  // stop bit
                if (samePortRXTX) {
                    pRX :> oldRX;
                    if (bit) oldRX |= maskTX; else oldRX &= (~maskTX);
                    pRX <: oldRX ;
                } else
                    if (port1bitTX) {
                        if (bit) pTX <: 1 ; else pTX <:0 ;
                    } else {
                        if (bit) oldTX |= maskTX; else oldTX &= (~maskTX);
                        pTX <: oldTX ; }
                //if (bit) printf("+"); else printf("-");
                bitNumTX++;
                if (bitNumTX > 8){  // end of transmit
                    // move on in the fifo
                    int newOut = foutTX + 1;
                    if (newOut >= sizeTX) newOut = 0;
                    foutTX = newOut;
                    stateTX = TX_AWAITING;
                    if (stateRX != RX_NONE) stateRX = RX_AWAITING;
                    //printf("]"); timeRXTX :> tTX; tTX+=dt;
                }

            } // TX_SENDING


            // xmodem
            if ((stateRX == RX_AWAITING) && (stateTX == TX_AWAITING) ) {
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
                break;}

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
                break;}

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
                break;}

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
                break;}

            // sending receiving commands
            case SM_SENDING: {
                if (finTX == foutTX) {
                    finRX = 0; foutRX = 0;
                    SMtimeout = nextT;
                    SMstate = SM_WAITING;
                }
                break;}

            case SM_WAITING: {
                if (finRX != foutRX) SMstate = SM_RECEIVING;
                if ((nextT - SMtimeout) > SMtimeoutMax)
                     SMstate = SM_TIMEOUT;
                break; }

            case SM_RECEIVING: {
                if (finRX >= SMansMax) SMstate = SM_RECEIVED;
                if (finRX)
                    if (SMansEnd>=0)
                        if (bufRX[finRX] == SMansEnd) SMstate = SM_RECEIVED;
                if ((nextT - SMtimeout) > SMtimeoutMax) SMstate = SM_TIMEOUT;
                break; }
            }
            } // end if xmodem

            break; } // case timerRXTX

            /** detect changes on pRX and store pRX in p **/
            case ((stateRX == RX_AWAITING) || (stateIR != IR_NONE)) =>
                    pRX when pinsneq(oldRX) :> int p : {

                //printf("^");
                int change = (p ^ oldRX);
                oldRX = p;

                /** check if the change is related to the pin RX **/
                if ( change & maskRX ){
                    /** is this a 0 start bit **/
                    if ( ( p & maskRX ) == 0) {
                        timeRXTX :> nextT; // our reference time starts now
                        nextT += dt + (dt>>1); // futur half bit after start bit
                        bitNumRX = 0; dataRX = 0;
                        stateRX = RX_RECEIVING;
                        if (stateTX != TX_NONE) stateTX = TX_ONHOLD;
                    } // startbit
                }
                // treat other possible input changes here

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
                    if (temp == 0) { temp = 1; } // rounding up because 0 is marking the end of a frame in our buffer
                    bufIR[bufIRin] = temp;

                    temp = bufIRin +1;
                    if (temp >= bufIRsize) temp = 0;
                    if (temp != bufIRout) bufIRin = temp;
                    bufIR[bufIRin] = 0; // End of Frame placeholder
                } // IR change detected
                break;}


            case (!isnull(uif)) => uif.xmodemSend(unsigned char * alias buf, const int size, const int timeoutMax ): {
                SMbufPtr = buf;
                SMtimeoutMax = timeoutMax;
                SMblockSize = size;
                SMtimeout = nextT;
                finRX = 0; foutRX = 0;
                SMstate = SM_WAITING_START;    // wait for start
                break;}

            case (!isnull(uif)) => uif.xmodemStatus() -> int res: {
                res = SMstate;
                break;}

            case (!isnull(uif)) => uif.xmodemStop() : {
                SMstate = SM_STOP;
                finTX = 0; foutTX = 0; finRX = 0; foutRX = 0;
                break;}

            case (!isnull(uif)) => uif.answerStart(int timeout, int max, int end) : {
                SMtimeoutMax = timeout;
                SMansMax = max;
                SMansEnd = end;
                SMstate = SM_SENDING;
                break;}

            case (!isnull(uif)) => uif.answerStatus() -> int res : {
                res = SMstate;
                break;}


            /** interface handling **/
            case (!isnull(uif)) => uif.gpioRX() -> int res:{
                if (!isnull(pRX)) {
                int val;
                pRX :> val;
                res = val; }
                else res = 0;
                break;}


            case (!isnull(uif)) => uif.gpioTX(int bit, int value) : {
                if (stateTX == TX_NONE) break;
                int mask;
                mask = 1<<bit;
                if (samePortRXTX) {
                    pRX :> oldRX;
                    if (value) { oldRX |= mask; } else { oldRX &= ~mask; }
                    pRX <: oldRX;
                    pRX :> oldRX;
                } else {
                    if (port1bitTX) {
                        if (value) pTX <: 1; else pTX <:0;
                    } else {
                        if (value) oldTX |= mask;  else oldTX &= ~mask;
                        pTX <: oldTX;
                    }
                }
                break;}


            case (!isnull(uif)) => uif.writeChar(unsigned char ch): {
                if (SMstate) break;
                int newIn = finTX + 1;
                if (newIn >= sizeTX) newIn = 0;
                if (newIn == foutTX) break; // buf full, exit
                bufTX[finTX] = ch; //printf("buffTX[%d]=0x%X, finTX <= %d\n",finTX, ch, newIn);
                finTX = newIn;
                break;}

            case (!isnull(uif)) => uif.readChar() -> unsigned char ch:{
                //SET_SHARED_GLOBAL(testSerial2,getTime());
                if (SMstate) break;
                if (finRX == foutRX) break; // nothing to read
                ch = bufRX[foutRX];
                int newOut = foutRX +1;
                if (newOut >= sizeRX) newOut = 0;
                foutRX = newOut;
                break;}

            case (!isnull(uif)) => uif.peekChar() -> unsigned char ch:{
                ch = bufRX[foutRX];
                break;}

            case (!isnull(uif)) => uif.numCharIn() -> int res:{
                if (SMstate) res = 0;
                else
                if (finRX >= foutRX) { res = finRX - foutRX;
                } else { res = sizeRX-(foutRX-finRX); }
                break;}

            case (!isnull(uif)) => uif.flushIn(): {
                if (SMstate) break;
                foutRX = 0;
                finRX = 0;
                if (stateRX != RX_NONE) stateRX = RX_AWAITING;
                break;}

            case (!isnull(uif)) => uif.numCharOut() -> int res: {
                if (SMstate) res = 0;
                else
                 if (finTX >= foutTX) { res = finTX - foutTX;
                 } else { res = sizeTX-(foutTX-finTX)-1; }
                break;}

            case (!isnull(uif)) => uif.sizeLeftOut() -> int res: {
                if (SMstate) res = 0;
                else
                 if (finTX >= foutTX) { res = sizeTX - (finTX - foutTX);
                 } else { res = (foutTX-finTX) - 1; }

                break;}

            case (!isnull(uif)) => uif.setBaud(int baudrate): {
                if (baudrate){
                    dt  = XS1_TIMER_HZ/baudrate;
                    if (stateRX != RX_NONE) {
                        stateRX = RX_AWAITING;
                        pRX :> oldRX;
                    }
                    if (stateTX != TX_NONE) {
                        stateTX = TX_AWAITING;
                        if (!samePortRXTX){
                        if (port1bitTX) pTX <: 1;
                        else {
                            oldTX |= maskTX;
                            pTX <: oldTX; }
                        }
                    }
                } else {
                    dt = XS1_TIMER_HZ/10000; // = 100us
                    if (stateRX != RX_NONE) {
                        stateRX = RX_ONHOLD;
                        pRX :> oldRX;
                    }
                    if (stateTX != TX_NONE) {
                        stateTX = TX_ONHOLD;
                        if (!samePortRXTX){
                        if (port1bitTX) pTX <: 1;
                        else {
                            oldTX |= maskTX;
                            pTX <: oldTX; }
                        }
                    }
                }
                foutTX = finTX = foutRX = finRX = 0;
                SMstate = SM_STOP;
                timeRXTX :> nextT; nextT += dt;
                break;}

            case (!isnull(uif)) => uif.setMaskIR(int mask):{
                maskIR = mask;
                if (mask) {
                    bufIRin = 0;
                    bufIRout = 0;
                    bufIR[0] = 0;
                    stateIR = IR_AWAITING;
                } else stateIR = IR_NONE;
                break; }

            case (!isnull(uif)) => uif.numFrameIR() -> int res:{
                res = frameIR;
                break; }

            case (!isnull(uif)) => uif.taskReady() -> int res:{
                res = 1;
                break; }
#if 0
            default:{   // check if we come here at least every 20uS, to detect overload
                tnew = getTime();
                int delta = tnew - told;
                if ((delta)>2000) {
                    printf("PROBLEM %d\n", delta); }
                told = tnew;
            break; }
#endif
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
