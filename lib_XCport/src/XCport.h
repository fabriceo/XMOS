/*
 * XCPort.h
 *
 *  Created on: 21 juin 2019
 *      Author: Fabriceo
 *
 *      library for accessing XMOS ports in an arduino style
 *      from both .xc or .cpp code
 *      and aross tile0/tile1
 *
 */

#ifndef XCPORT_H_
#define XCPORT_H_

//#define XCPORT_DEBUG

#include <XS1.h>
#include <platform.h>
#include <xccompat.h>
#include <swlock.h>
/*********** Some wrapper for asserting and printing in debug mode *************/
#ifdef XCPORT_DEBUG
#include <stdio.h>
#define XCPORT_PRINTF(...) { printf(__VA_ARGS__); }
#else
#define XCPORT_PRINTF(...) { }
#endif
#ifndef XCASSERT
#define XCASSERT(e) do { if (!(e)) __builtin_trap(); } while(0)
#define XCASSERT_PRINTF(e,...) do { if (!(e)) { XCPORT_PRINTF(__VA_ARGS__); __builtin_trap(); } } while(0)
#endif
/*********** some predefine value or macro can be used in user program ********/
// value possible for pinMode
#define INPUT 0x0
#define OUTPUT_PULLUP 0x1
#define INPUT_PULLUP OUTPUT_PULLUP // XS2 architecture : activating pullup will also activate DRIVE mode !
#define OUTPUT 0x2 // by default the port output will be 0 in this drive mode
#define OUTPUT_PULLDOWN 0x3
#define INPUT_PULLDOWN OUTPUT_PULLDOWN
#define DRIVE_LOW OUTPUT_PULLUP // same meaning
#define DRIVE_HIGH OUTPUT_PULLDOWN

// same value as per Arduino library ...
#define CHANGE 1
#define FALLING 2
#define RISING 3

// adding "SHARED" as a suffix will force the port-access to go trough dedicated core
// via interface, this is the only possible solution for sharing multibit ports across cores
#define INPUT_SHARED 0x4
#define OUTPUT_PULLUP_SHARED 0x5
#define INPUT_PULLUP_SHARED OUTPUT_PULLUP_SHARED
#define OUTPUT_SHARED 0x6
#define OUTPUT_PULLDOWN_SHARED 0x7
#define INPUT_PULLDOWN_SHARED OUTPUT_PULLDOWN_SHARED
#define DRIVE_LOW_SHARED OUTPUT_PULLUP_SHARED // same meaning
#define DRIVE_HIGH_SHARED OUTPUT_PULLDOWN_SHARED

// macro used to create a list of portMode(x,y) values. list MUST end with 0 !
#define XCPORT_MODE(name) const unsigned int name[]

// macro to define an XCport interface used in main for cross-tile communication
#define XCPORT_TILE_IF(name)  interface XCportTile_if name
// macro to define an XCport interface used in main for cross-core communication
#define XCPORT_CORE_IF(name)  interface XCportCore_if name

// this macro add bits forcing the library to transfer port access to another task on same tile, via interface
#define PORT_SHARED(x) (x | 4) // force bit 2
#define PORT_NO_SHARED(x) (x & 3)
#define PORT_IS_SHARED(x)  (x & 4) // for checking bit 2

// this macro add bits forcing the library to transfer actions to a task on other tile, via interface
#define PORT_ON_TILE(p,t) (p | (1<<10) | (t << 9)  ) // force bit 10 and set bit 9 according to tile
#define PORT_NO_TILE(p)   (p & ((1 << 9)-1)) // keep only bits 0..8
#define PORT_ON_TILE0(p) PORT_ON_TILE(p,0)
#define PORT_ON_TILE1(p) PORT_ON_TILE(p,1)
#define PORT_COND_TILE PORT_ON_TILE1(0) // used by library to recognize if access should be done from other tile


// this macro is used to create a 32bit mask & value from 2 16bits words
#define PORT_AND_OR(a,b) ((a<<16) | b)
// this macro is used to create a special 32 bit word from a 16bit word,
#define PORT_XOR(x) (x * 0x10001) // same result but faster than PORT_AND_OR(x,x)
// this macro forces bit 8 of a port-index from the enum table, in order to force the libray to only access memory, not HW
#define PORT_MEM(x) (x | (1<<8)) // bit 8 force reading the XCportOutVal table , only in digital read
/*
 * this is the name and coding for the digitalRead/Write port value, when not using standard XS1_PORT_xx
 */
enum XCportEnum {
// these 29+2 first value can be used for accessing portInd table
    P1A = 1, P1B, P1C, P1D, P1E, P1F, P1G, P1H, P1I, P1J, P1K, P1L, P1M, P1N, P1O,  P1P,
    P4A, P4B, P4C, P4D, P4E, P4F,
    P8A, P8B, P8C, P8D,
    P16A, P16B,
    P32A, // only 16 lower bits on P32A
    P16X, P16Y, // scratch mem location for sharing data across tile/core

// 44 below value are alligned with XDxx (P1A0 @ 30)
    P1A0, XCmaxPortMulti = P1A0, P1B0,
    P4A0, P4A1, P4B0, P4B1, P4B2, P4B3, P4A2, P4A3,
    P1C0, P1D0, P1E0, P1F0,
    P4C0, P4C1, P4D0, P4D1, P4D2, P4D3, P4C2, P4C3,
    P1G0, P1H0, P1I0, P1J0,
    P4E0, P4E1, P4F0, P4F1, P4F2, P4F3, P4E2, P4E3,
    P1K0, P1L0, P1M0, P1N0, P1O0,  P1P0,
    P8D4, P8D5, P8D6, P8D7,

// 60 additional indexes to be able to conveniently access single bit on port.
    P8A0, P8A1, P8A2, P8A3, P8A4, P8A5, P8A6, P8A7,
    P8B0, P8B1, P8B2, P8B3, P8B4, P8B5, P8B6, P8B7,
    P8C0, P8C1, P8C2, P8C3, P8C4, P8C5, P8C6, P8C7,
    P8D0, P8D1, P8D2, P8D3,

    P16A0, P16A1, P16A2, P16A3, P16A4, P16A5, P16A6, P16A7,
    P16A8, P16A9, P16A10, P16A11, P16A12, P16A13, P16A14, P16A15,
    P16B0, P16B1, P16B2, P16B3, P16B4, P16B5, P16B6, P16B7,
    P16B8, P16B9, P16B10, P16B11, P16B12, P16B13, P16B14, P16B15,

    P16X0, P16X1, P16X2, P16X3, P16X4, P16X5, P16X6, P16X7,
    P16X8, P16X9, P16X10, P16X11, P16X12, P16X13, P16X14, P16X15,
    P16Y0, P16Y1, P16Y2, P16Y3, P16Y4, P16Y5, P16Y6, P16Y7,
    P16Y8, P16Y9, P16Y10, P16Y11, P16Y12, P16Y13, P16Y14, P16Y15,

// this define the size of the tables declared in XCport.c
    XCmaxPortTable,

// additional indexes to point on the specific Tile 0 by just adding _0 at the end
    P1A_0 = PORT_ON_TILE(P1A,0), P1B_0, P1C_0, P1D_0, P1E_0, P1F_0,P1G_0, P1H_0,
    P1I_0, P1J_0, P1K_0, P1L_0, P1M_0, P1N_0, P1O_0,  P1P_0,
    P4A_0, P4B_0, P4C_0, P4D_0, P4E_0, P4F_0,
    P8A_0, P8B_0, P8C_0, P8D_0,
    P16A_0, P16B_0,
    P32A_0,
    P16X_0, P16Y_0, // scratch mem location for sharing data across tile/core

    P1A0_0 , P1B0_0,
    P4A0_0, P4A1_0, P4B0_0, P4B1_0, P4B2_0, P4B3_0, P4A2_0, P4A3_0,
    P1C0_0, P1D0_0, P1E0_0, P1F0_0,
    P4C0_0, P4C1_0, P4D0_0, P4D1_0, P4D2_0, P4D3_0, P4C2_0, P4C3_0,
    P1G0_0, P1H0_0, P1I0_0, P1J0_0,
    P4E0_0, P4E1_0, P4F0_0, P4F1_0, P4F2_0, P4F3_0, P4E2_0, P4E3_0,
    P1K0_0, P1L0_0, P1M0_0, P1N0_0, P1O0_0,  P1P0_0,
    P8D4_0, P8D5_0, P8D6_0, P8D7_0,

     P8A0_0, P8A1_0, P8A2_0, P8A3_0, P8A4_0, P8A5_0, P8A6_0, P8A7_0,
     P8B0_0, P8B1_0, P8B2_0, P8B3_0, P8B4_0, P8B5_0, P8B6_0, P8B7_0,
     P8C0_0, P8C1_0, P8C2_0, P8C3_0, P8C4_0, P8C5_0, P8C6_0, P8C7_0,
     P8D0_0, P8D1_0, P8D2_0, P8D3_0,

     P16A0_0, P16A1_0, P16A2_0, P16A3_0, P16A4_0, P16A5_0, P16A6_0, P16A7_0,
     P16A8_0, P16A9_0, P16A10_0, P16A11_0, P16A12_0, P16A13_0, P16A14_0, P16A15_0,
     P16B0_0, P16B1_0, P16B2_0, P16B3_0, P16B4_0, P16B5_0, P16B6_0, P16B7_0,
     P16B8_0, P16B9_0, P16B10_0, P16B11_0, P16B12_0, P16B13_0, P16B14_0, P16B15_0,

      P16X0_0, P16X1_0, P16X2_0, P16X3_0, P16X4_0, P16X5_0, P16X6_0, P16X7_0,
      P16X8_0, P16X9_0, P16X10_0, P16X11_0, P16X12_0, P16X13_0, P16X14_0, P16X15_0,
      P16Y0_0, P16Y1_0, P16Y2_0, P16Y3_0, P16Y4_0, P16Y5_0, P16Y6_0, P16Y7_0,
      P16Y8_0, P16Y9_0, P16Y10_0, P16Y11_0, P16Y12_0, P16Y13_0, P16Y14_0, P16Y15_0,

// additional indexes to point on the specific Tile 1 by just adding _1 at the end
     P1A_1 = PORT_ON_TILE(P1A,1), P1B_1,P1C_1, P1D_1, P1E_1, P1F_1,P1G_1, P1H_1,
     P1I_1, P1J_1, P1K_1, P1L_1, P1M_1, P1N_1, P1O_1,  P1P_1,
     P4A_1, P4B_1, P4C_1, P4D_1, P4E_1, P4F_1,
     P8A_1, P8B_1, P8C_1, P8D_1,
     P16A_1, P16B_1,
     P32A_1,
     P16X_1, P16Y_1, // scratch mem location for sharing data across tile/core

    P1A0_1, P1B0_1,
    P4A0_1, P4A1_1, P4B0_1, P4B1_1, P4B2_1, P4B3_1, P4A2_1, P4A3_1,
    P1C0_1, P1D0_1, P1E0_1, P1F0_1,
    P4C0_1, P4C1_1, P4D0_1, P4D1_1, P4D2_1, P4D3_1, P4C2_1, P4C3_1,
    P1G0_1, P1H0_1, P1I0_1, P1J0_1,
    P4E0_1, P4E1_1, P4F0_1, P4F1_1, P4F2_1, P4F3_1, P4E2_1, P4E3_1,
    P1K0_1, P1L0_1, P1M0_1, P1N0_1, P1O0_1,  P1P0_1,
    P8D4_1, P8D5_1, P8D6_1, P8D7_1,

    P8A0_1, P8A1_1, P8A2_1, P8A3_1, P8A4_1, P8A5_1, P8A6_1, P8A7_1,
    P8B0_1, P8B1_1, P8B2_1, P8B3_1, P8B4_1, P8B5_1, P8B6_1, P8B7_1,
    P8C0_1, P8C1_1, P8C2_1, P8C3_1, P8C4_1, P8C5_1, P8C6_1, P8C7_1,
    P8D0_1, P8D1_1, P8D2_1, P8D3_1,

    P16A0_1, P16A1_1, P16A2_1, P16A3_1, P16A4_1, P16A5_1, P16A6_1, P16A7_1,
    P16A8_1, P16A9_1, P16A10_1, P16A11_1, P16A12_1, P16A13_1, P16A14_1, P16A15_1,
    P16B0_1, P16B1_1, P16B2_1, P16B3_1, P16B4_1, P16B5_1, P16B6_1, P16B7_1,
    P16B8_1, P16B9_1, P16B10_1, P16B11_1, P16B12_1, P16B13_1, P16B14_1, P16B15_1,

    P16X0_1, P16X1_1, P16X2_1, P16X3_1, P16X4_1, P16X5_1, P16X6_1, P16X7_1,
    P16X8_1, P16X9_1, P16X10_1, P16X11_1, P16X12_1, P16X13_1, P16X14_1, P16X15_1,
    P16Y0_1, P16Y1_1, P16Y2_1, P16Y3_1, P16Y4_1, P16Y5_1, P16Y6_1, P16Y7_1,
    P16Y8_1, P16Y9_1, P16Y10_1, P16Y11_1, P16Y12_1, P16Y13_1, P16Y14_1, P16Y15_1,

// definition of the pin packages, limited to "43"
    XD00 = P1A0,
    XD01,XD02,XD03,XD04,XD05,XD06,XD07,XD08,XD09,XD10,XD11,XD12,XD13,XD14,XD15,XD16,XD17,XD18,XD19,XD20,XD21,
    XD22,XD23,XD24,XD25,XD26,XD27,XD28,XD29,XD30,XD31,XD32,XD33,XD34,XD35,XD36,XD37,XD38,XD39,XD40,XD41,XD42,XD43,

    X0D00 = PORT_ON_TILE(P1A0,0),
    X0D01,X0D02,X0D03,X0D04,X0D05,X0D06,X0D07,X0D08,X0D09,X0D10,X0D11,X0D12,X0D13,X0D14,X0D15,X0D16,X0D17,X0D18,X0D19,X0D20,X0D21,
    X0D22,X0D23,X0D24,X0D25,X0D26,X0D27,X0D28,X0D29,X0D30,X0D31,X0D32,X0D33,X0D34,X0D35,X0D36,X0D37,X0D38,X0D39,X0D40,X0D41,X0D42,X0D43,

    X1D00 = PORT_ON_TILE(P1A0,1),
    X1D01,X1D02,X1D03,X1D04,X1D05,X1D06,X1D07,X1D08,X1D09,X1D10,X1D11,X1D12,X1D13,X1D14,X1D15,X1D16,X1D17,X1D18,X1D19,X1D20,X1D21,
    X1D22,X1D23,X1D24,X1D25,X1D26,X1D27,X1D28,X1D29,X1D30,X1D31,X1D32,X1D33,X1D34,X1D35,X1D36,X1D37,X1D38,X1D39,X1D40,X1D41,X1D42,X1D43

};


// adapt the extern statement depending on the compiler
# ifdef __cplusplus
# define EXTERNALC extern "C"
# else // traditional c or xc
# define EXTERNALC extern
# endif // __cplusplus


// these function can be called either from cpp or c or xc programs so we use EXTERNAL macro
// the p parameter is one of the XCportEnum value declared above
EXTERNALC void pinMode(unsigned p, unsigned mode);
EXTERNALC void digitalWrite(unsigned p, unsigned val);
EXTERNALC int  digitalRead(unsigned p);
// to synchronize tasks one by one.
EXTERNALC void XCportStep(int step);

/******************************************************
 * functions below should not be used by user program
 ******************************************************/

// these ones are called by pinMode and digitalRead/Write from XCport.c and by interface
// the p parameter is a XS2 port resource address example XS1_PORT_8A
EXTERNALC void portWrite(unsigned p, int val);
EXTERNALC int  portRead(unsigned p);
EXTERNALC int  portPeek(unsigned p);
EXTERNALC int  portSetup(unsigned p, int mode, int val);
EXTERNALC const int XCportTable[XCmaxPortMulti];
EXTERNALC const int XCportMask[XCmaxPortTable];
EXTERNALC const unsigned char XCportTableInd[XCmaxPortTable];

// the p parameter is a member of exnum XCport enum
extern void pinModeBase(unsigned p, unsigned mode);
extern void digitalWriteBase(unsigned p, unsigned val);
extern int  digitalReadBase(unsigned p);

// expose the following function only to XC programs
#ifdef __XC__
// expose interface definition

interface XCportTile_if {
    void pinModeBase(unsigned p, unsigned mode);
    int  portTileIdIF();
    void digitalWriteBase(unsigned p, unsigned val);
    int  digitalReadBase(unsigned p);
};
// macro to define the interface cases inside a server task on the other tile
#define XCPORT_TILE_EVENTS(tileIF) \
         tileIF.pinModeBase(unsigned p, unsigned mode) : pinModeBase(p, mode); break; \
    case tileIF.portTileIdIF() -> int res: res = get_local_tile_id(); break; \
    case tileIF.digitalWriteBase(unsigned p, unsigned mode) : digitalWriteBase(p, mode); break; \
    case tileIF.digitalReadBase(unsigned p) -> int res: res = digitalReadBase(p); break // missing comma voluntary

// initialize global variables for routing calls to other tile and core interface
void XCportInitTileIF(client XCPORT_TILE_IF(?tileIF), unsigned n);
// call pinMode(x,y) for the declaration made in the list given
void XCportModeList(const unsigned int list[]);

[[combinable]]void XCportTileTask(server XCPORT_TILE_IF(tileIF)[n], static const unsigned n);

extern  int      XClocalTile;
extern  unsigned XCportOtherTileIF[8];
extern void portInitTileIF(unsigned tileIF, unsigned n, int localTile);
#else // not__XC__ start here
#ifdef __cplusplus
// this is a basic class to handle port in a seemless way, including opperator allocation
class XCport {
public:
    XCport(unsigned p) : pp(-p), value(0), prevValue(0) {}; // constructors
    ~XCport() { };          // destructor
    void write(int val);
    int read();
    int change(); // readPort and update values in memory
    int falling(); // only check values in memory.  returns 2 if falling
    int rising(); // only check values in memory.  returns 3 if rising
    void mode(int val);
    operator int() { return read(); }

     XCport& operator=(const int rhs)
     { write(rhs); return *this; }

private:
    // will contain the adress of the port.
    int pp, value, prevValue;
};
#else // not __cplusplus & not __XC__
// these global variables contains adress of interface calls and original tile number

// here we expose some XC function for XCport.c

extern void XCpinModeIF(unsigned interf, unsigned p, int mode);
extern void XCdigitalWriteIF(unsigned interf, unsigned p, int val);
extern int  XCdigitalReadIF(unsigned interf, unsigned p);

#endif //__cplusplus
#endif // __XC__


#endif /* XCPORT_H_ */
