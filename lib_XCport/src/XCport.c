/*
 * XCport.c

 *
 *  Created on: 25 juin 2019
 *      Author: Fabriceo
 */

#include "XCport.h"
#include <stdint.h>


// define the bit masks associated with the port index defined in XCportEnum
const int XCportMask[XCmaxPortTable] = {

        0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0,
        0,
        0, 0, // scratch registers X Y

        1, 1,
        1, 2, 1, 2, 4, 8, 4, 8,
        1, 1, 1, 1,
        1, 2, 1, 2, 4, 8, 4, 8,
        1, 1, 1, 1,
        1, 2, 1, 2, 4, 8, 4, 8,
        1, 1, 1, 1, 1, 1,
        16, 32, 64, 128,

        1, 2, 4, 8, 16, 32, 64, 128,
        1, 2, 4, 8, 16, 32, 64, 128,
        1, 2, 4, 8, 16, 32, 64, 128,
        1, 2, 4, 8,
        1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
        1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
        1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
        1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768
};

const unsigned char XCportTableInd[XCmaxPortTable] = {

        0,
        P1A, P1B, P1C, P1D, P1E, P1F, P1G, P1H, P1I, P1J,P1K, P1L, P1M, P1N, P1O,  P1P,
        P4A, P4B, P4C, P4D, P4E, P4F,
        P8A, P8B, P8C, P8D,
        P16A, P16B,
        P32A,
        P16X,P16Y,

        P1A, P1B,
        P4A, P4A, P4B, P4B, P4B, P4B, P4A, P4A,
        P1C, P1D, P1E, P1F,
        P4C, P4C, P4D, P4D, P4D, P4D, P4C, P4C,
        P1G, P1H, P1I, P1J,
        P4E, P4E, P4F, P4F, P4F, P4F, P4E, P4E,
        P1K, P1L, P1M, P1N, P1O,  P1P,
        P8D, P8D, P8D, P8D,

        P8A, P8A, P8A, P8A, P8A, P8A, P8A, P8A,
        P8B, P8B, P8B, P8B, P8B, P8B, P8B, P8B,
        P8C, P8C, P8C, P8C, P8C, P8C, P8C, P8C,
        P8D, P8D, P8D, P8D,

        P16A, P16A, P16A, P16A, P16A, P16A, P16A, P16A,
        P16A, P16A, P16A, P16A, P16A, P16A, P16A, P16A,
        P16B, P16B, P16B, P16B, P16B, P16B, P16B, P16B,
        P16B, P16B, P16B, P16B, P16B, P16B, P16B, P16B,

        P16X, P16X, P16X, P16X, P16X, P16X, P16X, P16X,
        P16X, P16X, P16X, P16X, P16X, P16X, P16X, P16X,
        P16Y, P16Y, P16Y, P16Y, P16Y, P16Y, P16Y, P16Y,
        P16Y, P16Y, P16Y, P16Y, P16Y, P16Y, P16Y, P16Y
};
// store the pinMode value for a whole port
unsigned char  XCportMode[XCmaxPortMulti]   = { 0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
// store the pinMode value for each bit of each main port (0=input, 1=output)
uint16_t XCportModePin[XCmaxPortMulti]= { 0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
// store the latest value written on an output port
uint16_t XCportOutVal[XCmaxPortMulti] = { 0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

// define the XS2 resource adress associated to a port index defined in XCportEnum
const int XCportTable[XCmaxPortMulti] = {

        0,
        XS1_PORT_1A, XS1_PORT_1B, XS1_PORT_1C, XS1_PORT_1D, XS1_PORT_1E, XS1_PORT_1F, XS1_PORT_1G, XS1_PORT_1H,
        XS1_PORT_1I, XS1_PORT_1J, XS1_PORT_1K, XS1_PORT_1L, XS1_PORT_1M, XS1_PORT_1N, XS1_PORT_1O,  XS1_PORT_1P,
        XS1_PORT_4A,XS1_PORT_4B,XS1_PORT_4C,XS1_PORT_4D,XS1_PORT_4E,XS1_PORT_4F,
        XS1_PORT_8A,XS1_PORT_8B, XS1_PORT_8C, XS1_PORT_8D,
        XS1_PORT_16A, XS1_PORT_16B,
        XS1_PORT_32A,
        0,0 // scratch memory registers
};


// hard code for accessing port ressource, a is the port ressource adresse defined in XS1.h
#define setc(a,b)    {__asm__  __volatile__("setc res[%0], %1": : "r" (a) , "r" (b));}
#define portin(a,b)  {__asm__  __volatile__("in %0, res[%1]": "=r" (b) : "r" (a));}
#define portout(a,b) {__asm__  __volatile__("out res[%0], %1": : "r" (a) , "r" (b));}
#define portpeek(a,b){__asm__  __volatile__("peek %0, res[%1]": "=r" (b) : "r" (a));}

// these function provide elementary access to port ressources with p being XS1 ressource adresse

int portRead(unsigned p) {
    //XCPORT_PRINTF("portRead( %x\n",p);
   int res;
   if (p) {
       portin(p, res); return res;
   } else return 0;
}
int portPeek(unsigned p) {
    //XCPORT_PRINTF("portPeek( %x\n",p);
   int res;
   if (p) {
       portpeek(p,res); return res;
   } else return 0;
}
void portWrite(unsigned p, int val) {
    //XCPORT_PRINTF("portWrite( %x, 0x%x\n",p,val);
    if (p) portout(p,val);
}

// this will allocate the port to the calling core and will initiate its value (no memory access)
int portSetup(unsigned p, int mode, int val){
    XCPORT_PRINTF("portSetup( %x, 0x%x, 0x%x )\n", p, mode, val);
    setc(p, XS1_SETC_INUSE_ON);
    mode = PORT_NO_SHARED(mode); // safely keep only 2 lower bits
    switch (mode) { // only 4 modes in fact for XS2
        case OUTPUT_PULLUP:
            setc(p, XS1_SETC_DRIVE_PULL_UP);
            break;
        case OUTPUT_PULLDOWN:
            setc(p, XS1_SETC_DRIVE_PULL_DOWN);
            break;
        case OUTPUT:
            setc(p, XS1_SETC_DRIVE_DRIVE);
            break;
        case INPUT:
            setc(p,0x01); // not sure why , copy paste from compiler generated code when doing :>
            return portRead(p);
    }
    portout(p, val);
    return val;
}


//expected bit 8 to contain 0 or 1 representing the tile[0] or 1 where the I/O task is running
int XClocalTile = 0;
// contains the value of the interface (if defined) for calling port changes accross tiles
unsigned XCportOtherTileIF[8] = { 0,0,0,0,0,0,0,0 };

//unsigned XClocalCoreId = 0; // compiler bug, so not supported yet


void portInitTileIF(unsigned tileIF, unsigned n, int localTile){
    XClocalTile = localTile << 9;
    XCportOtherTileIF[n] = tileIF;
    XCportStep(0); // we are ok to accept the step 1
}

// wrappers in order to "wait" or ensure that the core and/or tile interfaces are setup
volatile unsigned *portOtherTileIF = &XCportOtherTileIF[0];

unsigned waitOtherTileIF(){
    while (!*portOtherTileIF) { } return *portOtherTileIF;
}

// this function is used to create a light state machine with steps form 0 to N.
// calling the function with value X will wait the lock to be X-1 and then put the lock value to X
// can be used to synchronize task launch in order to secure port setup before use!
// only works across cores on a same tile.
swlock_t XCportStepLock = SWLOCK_INITIAL_VALUE;

int XCportStep_ = 0;
volatile int *_XCportStep_ = &XCportStep_;

void XCportStep(int step){

    if (step) {
       XCPORT_PRINTF("waiting step %d\n",step);
       step--;
       while (*_XCportStep_ < (step)) { }
       swlock_acquire(&XCportStepLock);
       if (*_XCportStep_ == step) { // no change so this is ours
           (*_XCportStep_)++; // next step
       }
       swlock_release(&XCportStepLock);
       XCPORT_PRINTF("releasing step %d\n",step+1);
    } else { *_XCportStep_ = 0; }// defaut initialisation
}

// configure the whole port and the overhead tables in either input or output.
// p is the index from XCportEnum starting at P1A and ending at P16Y
void portModeBase(unsigned p, unsigned mode){
    XCPORT_PRINTF("portModeBase( %d, %d )\n",p,mode);
    int cond = (p & PORT_MEM(0));
    if (cond) p ^= cond; else cond = 0; // memorize if this is a memory access only

    XCASSERT_PRINTF(p <= P16Y, "value not acceptable for a plain port %d\n",p );

    int mask = 0;
    XCportMode[p] = mode; // store the port mode in global memory for history (required later)
    XCPORT_PRINTF("portMode = %d\n",mode);
    if (mode & 3) mask = 0xFFFF; // all bits are output
    XCportModePin[p] = mask;
    XCPORT_PRINTF("portModePin = 0x%x\n", mask);
    if (mode & OUTPUT) mask = 0; // output or output low implie 0 by default.
    XCportOutVal[p] = mask;     // store this default value in memory
    XCPORT_PRINTF("portOutVal = 0x%x\n", mask);
    if (cond) return;
    p = XCportTable[p]; // retreive XS1 adress port
    XCPORT_PRINTF("addrP = %x\n",p)
    portSetup(p, mode, mask );
}

// p can be an index on a whole port, or specific bit.
// a first (and normally unique) call to pinMode should be done for the whole port
void pinModeBase(unsigned p, unsigned mode){
    int mask = XCportMask[p];
    XCPORT_PRINTF("pinModeBase( %x-%d, %d ) mask= %d\n",p>>8,p & 0xFF,mode, mask);

     if (mask) { // this is a pinMode on a single bit of a multibit port

         int cond = (p & PORT_MEM(0));
         if (cond) p ^= cond; else cond = 0; // memorize if this is a memory access only

         int pBase = XCportTableInd[p]; // retreive port base
         int pBaseMode = PORT_NO_SHARED(XCportMode[pBase]);
         XCPORT_PRINTF("pBase = %d, pBaseMode = %d\n", pBase, pBaseMode);
         if (pBaseMode == 0) {// main port has not been setup (or is a straight input)
             portModeBase(pBase, mode); // lets initialise the whole port at first
             pBaseMode = mode; }

         if (PORT_NO_SHARED(mode) == 0) {// if this is an input
             XCASSERT_PRINTF(pBaseMode != OUTPUT,"cannot configure a bitwise input from a port declared drive output\n");

             XCportModePin[pBase] &= (~mask); // flag this bit as an input

             if ( pBaseMode == OUTPUT_PULLUP)    XCportOutVal[pBase] |= (mask);  // force non 0
             if ( pBaseMode == OUTPUT_PULLDOWN)  XCportOutVal[pBase] &= (~mask); // force non 1

             if (cond) return; // only access in memory required

             mask = XCportOutVal[pBase]; // retreive future value of the port
             p = XCportTable[pBase]; // XS1 resssource adress of the port
             portWrite(p, mask);

         } else // not an input
             XCASSERT_PRINTF(PORT_NO_SHARED(mode) == pBaseMode,"input_pullx shall be identical\n");

     } else  // this is a whole port
         portModeBase(p, mode);
}


// this is the main function to setup a port (whole port or specific bit)
// p is a port index based on XCportEnum definitions
void pinMode(unsigned p, unsigned mode){
    XCPORT_PRINTF("pinMode( %d, %d )\n", p, mode);
    int cond = ((p & PORT_ON_TILE1(0)) ^ XClocalTile);
    p = PORT_NO_TILE(p);
    // check if specific tile is requested
    if ( cond == PORT_ON_TILE1(0)) { // test if port on the other tile
        XCpinModeIF(waitOtherTileIF(), p, mode);
    } else {
        pinModeBase(p, mode); }
}


// write a value to a port or a bit on a port.
// for a bit, if val != 0 the bit will be forced to 1
// for a port, it is possible to use logical AND OR XOR:
// 16 lower bit are representing the bits to be SET to 1
// 16 higher bit is a mask for keeping original bit or not
// if 16 higher = 16 lower, then the resulting value is a XOR
swlock_t digitalWriteLock = SWLOCK_INITIAL_VALUE;
void digitalWriteBase(unsigned p, unsigned val){
    int cond = (p & PORT_MEM(0));
    if (cond) p ^= cond; else cond = 0; // memorize if this is a memory access only
    int  mask = XCportMask[p];
    XCPORT_PRINTF("digitalWriteBase( %d, 0x%x) mask = %d\n",p,val,mask);
    int portShared;
    if (mask) { // multibit port

        if (val) val = mask;

        p = XCportTableInd[p]; // retreive base port
        portShared = PORT_IS_SHARED(XCportMode[p]);
        // resuse port value stored in memory, to avoid using peek(p)
        if (portShared) swlock_acquire(&digitalWriteLock);
        val |= (XCportOutVal[p] & (~mask))  ;

    } else { // plain port

        portShared = PORT_IS_SHARED(XCportMode[p]);
        mask = (val >> 16);
        if (mask) { // check if we are given a AND mask
            val &= 0xFFFF; // then keep only the lower bits
            if (portShared) swlock_acquire(&digitalWriteLock);
            if (val != mask) {
                     val |= (XCportOutVal[p] & (~mask)) ;
            } else { val ^= XCportOutVal[p]; }
        }
    }
    XCportOutVal[p] = val; // store the new value in memory for next access
    if (portShared) swlock_release(&digitalWriteLock);
    XCPORT_PRINTF("portOutVal = 0x%x\n",val);
    if (cond) return;
    int pXS1 = XCportTable[p]; // retreive XS1 adress port
    XCPORT_PRINTF("pXS1 = %x\n", pXS1);
    portWrite(pXS1, val);
}

// this c version check if the access is expected on the same tile or not.
void digitalWrite(unsigned p, unsigned val){
    int cond = ((p & PORT_ON_TILE1(0)) ^ XClocalTile);
    p = PORT_NO_TILE(p);
    if ( cond == PORT_ON_TILE1(0)) { // requires bit 9 and bit 8 different from localone
        XCdigitalWriteIF(waitOtherTileIF(), p, val);
    } else digitalWriteBase(p, val);
}

int  digitalReadBase(unsigned p){
    int cond = (p & PORT_MEM(0));
    if (cond) p ^= cond; else cond = 0; // memorize if this is a memory access only
    int  mask = XCportMask[p];
    XCPORT_PRINTF("digitalReadBase( %d )\n", p);
    p = XCportTableInd[p]; // retreive port base
    XCPORT_PRINTF("pBase = %d, mask = 0x%x\n", p, mask);
    int res;
    if (cond){ // read port value in memory only
        res = XCportOutVal[p];
    } else {

        int pXS1 = XCportTable[p]; // retreive XS1 adress port

        if (PORT_NO_SHARED(XCportMode[p])) { // this is an output port
                res = portPeek(pXS1);
        } else  res = portRead(pXS1);
    }
    XCPORT_PRINTF("readRes = 0x%x\n",res);
    if (mask) res = ((res & mask) != 0);
    return res;
}

int  digitalRead(unsigned p){
    XCPORT_PRINTF("digitalRead( %d )\n", p);
    int cond = ((p & PORT_ON_TILE1(0)) ^ XClocalTile);
    p = PORT_NO_TILE(p); // remove tile related bits

    // check if specific tile is requested
    if (cond == PORT_ON_TILE1(0)) {
        return XCdigitalReadIF(waitOtherTileIF(), p);
    } else {
        return digitalReadBase(p); }
}
