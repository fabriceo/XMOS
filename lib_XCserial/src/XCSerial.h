#ifndef XCSERIAL_H
#define XCSERIAL_H

    enum SMstates {
        SM_STOP = 0,
        SM_WAITING_START,
        SM_SENDING_SOH,
        SM_SENDING_BLOCK,
        SM_WAITING_ACK,
        SM_FAILED,
        SM_COMPLETED,

        SM_SENDING,
        SM_WAITING,
        SM_RECEIVING,
        SM_RECEIVED,
        SM_TIMEOUT,
    };

#ifdef __XC__
interface XCSerial_if {
    int numCharIn(); // number of charaters in the RX buffer
    int numCharOut(); // number of charater in the TX buffer, still to be sent
    int sizeLeftOut(); // space left in the TX buffer
    unsigned char readChar(); // get a charachter from the fifo RX (not blocking)
    unsigned char peekChar(); // look at the char available in the fifo RX
    void writeChar(unsigned char ch);
    void flushIn(); // reset the RX buffer
    void setBaud(int baudrate); // change the uart bit rate
    int  gpioRX(); // read all bits on the RX input port (relevant if 4 or 8 bits)
    void gpioTX(int bit, int value); // set a bit on the TX port (if 4 or 8 bits)

    void setMaskIR(int mask);
    int numFrameIR();

    void xmodemSend(unsigned char * alias buf, const int size, const int timeoutMax );
    void xmodemStop();
    int  xmodemStatus();

    void answerStart(int timeout, int max, int end);
    int  answerStatus();
} ;

#define XCSERIAL_IF(x) interface XCSerial_if x


[[combinable]]
void XCSerial(port ?pRX, port ?pTX,
              unsigned maskRX, unsigned maskTX,unsigned maskRTS,
              unsigned bauds,
              static const unsigned sizeRX, static const unsigned sizeTX
              ,server XCSERIAL_IF(?uif)
              );

#endif

#ifdef __cplusplus

class SoftwareSerial {
public:
       SoftwareSerial(); // constructor
       ~SoftwareSerial() {  }; // destructor

       void init(unsigned uartIF);
       void begin(int baudRate);
       void end();
       int available();
       int read();
       int peek();
       void flush();
       void write(unsigned char byte);
private:
       unsigned refuartIF;
};

#ifdef CDC
class usbSerial {
public:
       usbSerial(); // constructor
       ~usbSerial() {  }; // destructor

       void init(unsigned IF);
       void begin(int baudRate);
       void end();
       int available();
       int read();
       int peek();
       void flush();
       void write(unsigned char byte);
private:
       unsigned usbIF;
};

#endif


#endif

#endif
