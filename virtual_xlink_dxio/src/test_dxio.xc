/*
 * test_dxio.xc
 *
 *  Created on: 6 janv. 2023
 *      Author: Fabrice
 */

#include <xs1.h>
//#include <xs2a_registers.h>
#include <platform.h>
#include <stdio.h>
#include "trycatch.h"

#if 1
#ifdef XSCOPE
#include <xscope.h>
void xscope_user_init()
{   xscope_register(0, 0, "", 0, "");
    xscope_config_io(XSCOPE_IO_BASIC); }
#endif

#define CREDIT8  (0x1E0)
#define CREDIT64 (0x1E1)
#define CREDIT16 (0x1E4)
#define HELLO    (0x1E6)
#define WIRES    2

unsigned link_config_read(int link_num) {
    unsigned x;
    read_sswitch_reg(get_local_tile_id(), XS1_SSWITCH_XLINK_0_NUM + link_num, x);
    return x;
}

void link_config_write(int link_num, unsigned x){
    write_sswitch_reg(get_local_tile_id(), XS1_SSWITCH_XLINK_0_NUM + link_num, x);
}

void link_disable(int link_num) {
  // disable link by resetting ENABLE bit in link's control register
  unsigned x = link_config_read(link_num);
  x &= ~XS1_XLINK_ENABLE_MASK;
  link_config_write(link_num, x);
}


void link_enable(int link_num, int inter, int intra) {
  unsigned x = link_config_read(link_num);
  // configure link by writing to link's control register:
  // set intertoken and intratoken delays
  // set ENABLE bit
  // set WIDE bit if 5-bit link required
  x |= XS1_XLINK_INTER_TOKEN_DELAY_SET(x, inter?inter:5);
  x |= XS1_XLINK_INTRA_TOKEN_DELAY_SET(x, intra?intra:5);
  x |= XS1_XLINK_ENABLE_MASK;
  if (WIRES == 5) x |= XS1_XLINK_WIDE_MASK;
  link_config_write(link_num, x);
}

void link_reset(int link_num) {
  // reset link by setting RESET bit in link's control register
  unsigned x = link_config_read(link_num);
  x |= XS1_XLINK_RX_RESET_MASK;
  link_config_write(link_num, x);
}

void link_hello(int link_num) {
  // send a hello by setting HELLO bit in link's control register
  unsigned x = link_config_read(link_num);
  x |= XS1_XLINK_HELLO_MASK;
  link_config_write(link_num, x);
}

int link_got_credit(int link_num) {
  unsigned x =link_config_read(link_num);
  return XS1_TX_CREDIT(x);
}

void link_set_static(int link_num, int stat, int processor, int channel){
    unsigned x = ((stat?1:0) << XS1_XSTATIC_ENABLE_SHIFT) | (channel & 0x1F) | ((processor & 1) << 8);
    write_sswitch_reg(get_local_tile_id(), XS1_SSWITCH_XSTATIC_0_NUM + link_num, x);
}

void link_set_direction(int link_num, int dir){
    unsigned x;
    read_sswitch_reg(get_local_tile_id(), XS1_SSWITCH_SLINK_0_NUM + link_num, x);
    x = XS1_LINK_DIRECTION_SET(x,dir);
    printf("link %d direction = %8x\n",link_num,x);
    write_sswitch_reg(get_local_tile_id(), XS1_SSWITCH_SLINK_0_NUM + link_num, x);
}

void bits_set_direction(unsigned bits, int dir){
    // dir between 0..15;
    // bits is a 16bit mask to force the given direction for each of these bits
    for (int reg =  XS1_SSWITCH_DIMENSION_DIRECTION0_NUM;
             reg <= XS1_SSWITCH_DIMENSION_DIRECTION1_NUM;
             reg++) {
        unsigned x;
        read_sswitch_reg(get_local_tile_id(), reg, x);
        if (bits & 0xFF) {
            for (unsigned mask=0b1111, dirmask=dir;
                          mask!=0;
                          mask<<=4,dirmask<<=4, bits >>= 1) {
                //printf("mask = %x, dirmask = %x, bits = %x\n",mask,dirmask,bits);
                if (bits & 1) {  x &= ~mask; x |= dirmask; }
            }
            write_sswitch_reg(get_local_tile_id(), reg, x);
        } else
            bits >>= 8;
        printf("reg %x <= %8x\n",reg,x);
    } // for reg
}

void print_link_config(int link_num){
    unsigned x =link_config_read(link_num);
    printf("xlink %d : configured in %d wires mode\n", link_num, (x & (1<<30) ? 5 : 2 ));
    if (x & (1<<27)) printf(" Rx buffer overflow or illegal token encoding received\n");
    if (x & (1<<26)) printf(" This link has sent credit\n"); else printf(" This link has not sent credit yet\n");
    if (x & (1<<26)) printf(" Credit received.\n"); else printf(" No credit received yet.\n");
    printf(" Intra token delay: %d\n",XS1_XLINK_INTRA_TOKEN_DELAY(x));
    printf(" Inter token delay: %d\n",XS1_XLINK_INTER_TOKEN_DELAY(x));
}

void print_link_status(int link_num) {
    unsigned x;
    read_sswitch_reg(get_local_tile_id(), XS1_SSWITCH_SLINK_0_NUM + link_num, x);

}

void print_plink_status(int link_num) {
    unsigned x;
    read_sswitch_reg(get_local_tile_id(), XS1_SSWITCH_PLINK_0_NUM + link_num, x);
    printf("link %d on nework %d\n",link_num,(x>>4)&3);
    if (x & 1) printf(" Source side in use\n"); else printf(" Source side not in use\n");
    if (x & 2) printf(" Dest side in use\n"); else printf(" Dest side not in use\n");
    if (x & 1<<2) printf(" Junk packet\n");
    printf(" link direction %x\n",(x>>8)&15);
    printf(" link destination number %d\n",(x>>16)&255);
    printf(" SRC_TARGET %d\n",(x>>24)&3);
}
#undef outt
#define outt(c, x)   asm ("outt res[%0], %1" :: "r" (c), "r" (x))
#undef outct
#define outct(c, x)   asm ("outct res[%0], %1"  :: "r" (c), "r" (x))

#define asmtestct(c,x) asm("testct %0,res[%1]":"=r"(x):"r"(c))
#define asminct(c,x)   asm("inct %0,res[%1]":"=r"(x):"r"(c))
#define asmintuchar(c,x)   asm("int %0,res[%1]":"=r"(x):"r"(c))

#pragma select handler
void testct_byref(chanend c, unsigned &returnVal) {
    if(testct(c)) returnVal = 1;
    else returnVal = 0;
}
#pragma select handler
void testct_byref2(unsafe streaming chanend c, unsigned &returnVal) {
    unsigned test;
    asmtestct(c,test);
    if(test) returnVal = 1;
    else returnVal = 0;
}


void t0(tileref t, int link, int intra, int inter )  // setup link (including rx) and send patern data
{ unsafe {

    unsafe streaming chanend c;
    exception_t exception;

    printf("task1 started on local tile id %x\n",get_local_tile_id());
    delay_milliseconds(10);

    printf("link disabled\n");
    link_disable(link);

    // get channel resource and configure its destination on the other side of the link
    asm("getr %0, 2" : "=r"(c));
    asm("setd res[%0], %1" :: "r"(c), "r"(0x40000002)); // destination tile + channel number + always 0x02 for chanend
    printf("channel get and setd %x\n",(int)c);

    printf("bit set direction\n");
    bits_set_direction(0x8000,3);   // bit 15 difference will force direction 3

    printf("link set static\n");    // set receiving channel for data received on this link and avoid sending routing bytes
    link_set_static(link, 1, 0, (((int)c)&0xFF00)>>8); // static, processor , chanend

    link_set_direction(link, 3);    // attach link to a direction

    printf("enabling link %d\n",link);
    link_enable(link, intra, inter);    //inter = 500 => 200khz

    print_link_config(link);

    do {
      delay_milliseconds(10);
      link_reset(link);
      link_hello(link);
      delay_milliseconds(10);
    } while (!link_got_credit(link));
    printf("Got credit link %d \n",link);
    delay_milliseconds(50);
    timer t; int tt,now;
    int token=0;
    unsigned ct;
  TRY {
    while (1) {
        select {
            case testct_byref2(c, ct):   //reception from link 7
                if(ct) {    //check if this is a control token
                    asminct(c,ct); ct |= 256;
                } else asmintuchar(c,ct);
                printf("<%x>\n",ct);
                break;
            case t when timerafter(tt+1000000) :> tt: //10ms
                outt(c,token); //send trough link 7
                token++;
                if (token==256) token = 0;
                break;
        }

        }
  CATCH (exception) { printf("Got exception: 0x%x\n", exception); }
  } // try
  printf("task1 ended %d\n",now);
} }

#define queue_size 100
typedef unsigned short queue_item_t;
typedef struct {
    queue_item_t buff[queue_size];
    unsigned qin;
    unsigned qout;
} queue_t;

queue_t queue_tx = { {0},0,0 };
queue_t queue_rx = { {0},0,0 };

#pragma unsafe arrays
inline int queue_put(volatile queue_t * unsafe q, queue_item_t data){ unsafe {
    unsigned qin  = q->qin;     //read index
    unsigned next = qin + 1;    //prepare to be next
    if (next >= queue_size) next = 0;   //check over size
    if (next != q->qout) {      // check overlap
        q->buff[qin] = data;    //store
        q->qin = next;          //store new index prepared
        return 1; }
    return 0;
} }

inline int queue_items(volatile queue_t * unsafe q){ unsafe {
    int count = q->qin - q->qout;
    if (count<0) count += queue_size;
    return count;
} }


#pragma unsafe arrays
inline int queue_get(volatile queue_t * unsafe q, queue_item_t &data){ unsafe {
    unsigned next = q->qout;            //read index
    if (q->qin == next) return 0;       //check empty condition
    queue_item_t res = q->buff[next];   //get value
    next++;                             //move index
    if (next >= queue_size) next = 0;   //check over size
    q->qout = next;                     //store index
    data = res;
    return 1;
} }


void virtual_link(
        port ptx0, port ?ptx1, // tx port. either one 4bits port and null, or two 1bit ports
        port prx0, port ?prx1, // rx port. either one 4bits port and null, or two 1bit ports
        int intra, int inter,  // intra and inter token delay, in cpu cycles (pll/5)
        chanend ?ctx, chanend ?crx ) { // optional channels for transmiting to tx or receiving data from rx
unsafe {
    unsigned tx = 0;
    ptx0 <: tx;
    if (!isnull(ptx1)) ptx1 <: tx;
    unsigned rx, orx, rx0, rx1;
    prx0 :> rx; rx &= 3; orx = rx; rx0 = rx & 1;
    if (!isnull(prx1)) prx1 :> rx1; else rx1 = (rx >> 1);
    if (inter < intra) inter = intra;
    volatile queue_t * unsafe qtx = &queue_tx;
    volatile queue_t * unsafe qrx = &queue_rx;

    unsigned statetx = 9;   //force return to zero as a start
    unsigned staterx = 0;   //await receiving a first transition
    queue_item_t tokenrx = 0, tokentx  = 0;
    int four_bit_time_rx = 1600, t_bit_rx, t_bit0_rx;   //used to measure rx frequency and define a 4 bits timeout
    int t_bit_tx;
    const unsigned credit_min = 8; // treshold before requesting credit with sending Hello token
    unsigned credit_counter = 0, credits_issued = 0;
    unsigned linklevel_token = 0; // used to send linklevel token (0xE0..0xFF not requiring credit) in top priority

    printf("virtual link task : intra character = %fus, inter = %fus\n", intra/100.0, inter/100.0);
    timer t; int last_t; t:>last_t; //initialize timer for select timerafter
    while (1) {
        unsigned bit = 2;   //will be set to 0 or 1 if valid bits are received otherwise 2
        unsigned ct;        //temporary token from channel
        select {
            // if we receive data from a channel, then put value in the tx queue
            case !isnull(ctx) => testct_byref(ctx, ct):
                if(ct) {    //check if this is a control token
                    ct = inct(ctx);
                    if (ct == 1) { outct(ctx,1); break; } // test if CT_END then handshake closing route
                    ct |= 256;
                } else ct = inuchar(ctx);
                queue_put(qtx,ct);  //we do not check overload
                break;

            // check transition on rx (1 or 4 or 8 bits)
            case prx0 when pinsneq(rx) :> rx :  // transition on prx0 means we receive a 0
                // code below is compatible with 1,4,8 bits ports
                if (isnull(prx1)){
                    rx &= 3;                        // keep 2 lsb
                    rx0 = rx & 1; rx1 = rx >> 1;
                    switch ((rx ^ orx)) {           //chek variation
                    case 0:staterx = 0; break;      // most likely not possible unless ultra fast glitches
                    case 1:
                        if (staterx || rx0) bit=0;  // check state 0 special case as rx0 should be rising 1 only
                        break;
                    case 2:
                        if (staterx || rx1) bit=1;  // check state 0 special case as rx1 should be rising 1 only
                        break;
                    case 3:staterx = 0; break; }    // most likely not possible unless ultra fast glitches
                    orx = rx;
                } else {
                    rx0 = rx;
                    if (staterx || rx0) bit = 0;     // check state 0 special case as rx0 should be rising 1 only
                }
                break;
            //check transition on rx1 (if used)
            case !isnull(prx1) => prx1 when pinsneq(rx1) :> rx1 :
                if (staterx || rx1) bit = 1;        // check state 0 special case as rx1 should be rising 1 only
                break;

            //tick based on intra token delay (every bit)
            case t when timerafter( last_t + intra ) :> last_t:     //store real value of t within last_t

                // check intra token bit time once a first bit has been received
                if (staterx) {
                    if ((last_t - t_bit_rx) > four_bit_time_rx){    //timeout if time > 4 bits measured earlier
                        staterx = 10;       //simulate end of reception
                        tokenrx = 0x1E5;    //force pushing special token
                        bit = 0;            //simulate bit reception
                    }
                }

                // transmission state machine
                switch (statetx) {
                case 9:     // return to 0 on both lines
                    tx = 0;
                    ptx0 <: tx; if (!isnull(ptx1)) ptx1 <: tx;
                    //time stamp at the end of the transmission
                    t_bit_tx = last_t;
                    statetx++;
                    break;
                case 10:    //waiting one or more inter character
                    if (tokentx == HELLO) {
                        // test if we received some credit or if timeout is reached
                        if ((credit_counter >= credit_min) || ((last_t - t_bit_tx) >= (20*four_bit_time_rx) ) ) statetx++;
                    } else
                        // wait time for one inter character.
                        if ((last_t - t_bit_tx) >= inter) statetx++;
                    break;
                case 11:    //token received
                    // check if a linklevel token is awaiting in fasttrack register
                    if (linklevel_token) {
                        tokentx = linklevel_token;
                        linklevel_token = 0;
                        //printf("sending 0x%x\n",tokentx);
                        statetx = 7;            //sending first msb bit at next timerafter
                        break;
                    }
                    // check status of our counter (ability to send further)
                    if (credit_counter < credit_min ) {
                        if (credit_counter) printf("requesting credit (%d<%d)\n",credit_counter,credit_min);
                        tokentx = HELLO;
                        credit_counter = 0;     //reset our counter as per specification
                        statetx = 7;            //sending first msb bit at next timerafter
                        break;
                    }
                    //check if we have enough credit for sending data
                    if (credit_counter > 0) {
                        //check if we have pending data in the queue
                        if (queue_items(qtx)) {
                            queue_get(qtx, tokentx);
                            if (tokentx >= 0x1E0) {     // no need for credit for linklevel token
                                //printf("application sending linklevel token 0x%x\n",tokentx);
                                if (tokentx == HELLO) credit_counter = 0;
                            } else {
                                printf("sending data 0x%x, %d\n",tokentx,credit_counter);
                                credit_counter -= 1; }
                            statetx = 7;        //sending first msb bit at next timerafter
                            break; }
                    }
                    break; // case 11

                default: // case 0..8
                    if (tokentx & (1 << statetx))  tx ^= 2; else tx ^= 1;
                    if (isnull(ptx1)) ptx0 <: tx;    // 4bits port
                    else { ptx0 <: (tx & 1); ptx1 <: (tx >>1); }
                    if (statetx & 8) statetx++; // next one is return to zero
                    else
                        if (statetx == 0) statetx |= 8; // send control token bit
                        else statetx--;
                    break;
                }//switch state
                break;
        } // select

        // check if real bits where received
        if ((bit & 2)==0) {
            t:> t_bit_rx;   // time stamp bit reception
            //if (bit) printf("1"); else printf("0");
            switch (staterx) {           // rx state machine
            case 0:
                //very first bit received
                t_bit0_rx = t_bit_rx;    // store current time base when first transition arrives
                staterx++;
                tokenrx = bit;          // store this bit in the token. this is bit 7
                break;
#pragma fallthrough
            case 1: case 2: case 3: case 4: case 5: case 6: case 7:
                //receive bit 6 to bit 0 of the token
                staterx++;
                tokenrx = (tokenrx << 1) | bit;
                break;
            case 8:
                //9th bit received represents data(0) or control(1) token
                t_bit0_rx = t_bit_rx - t_bit0_rx;
                four_bit_time_rx = t_bit0_rx >>1; // divide by 2 => number of ticks for 4 bits.
                staterx++; tokenrx |= (bit << 8);
                break;
#pragma fallthrough
            case 9:
                //last bit (return to zero state)
                // for the 10th bit, the 2 lines should be back to 0
                if (rx0 || rx1) { staterx = 0; break;}  //else we have a problem and restart from state 0
                //staterx++;
            case 10:
                 staterx = 0;
                 switch(tokenrx) {
                 case HELLO:
                     //printf("received HELLO\n");
                     if (linklevel_token == 0) {
                         //prepare to send credit
                         linklevel_token = CREDIT16 ;
                         credits_issued = 16;
                     } else
                         credits_issued = 0;
                     break;
                 case CREDIT8:
                     if (credit_counter < 120) {
                         (credit_counter) += 8; //printf("got CREDIT8 -> cpt= %d\n",credit_counter);
                     } else printf("got CREDIT8 too much %d\n",credit_counter);
                     break;
                 case CREDIT16:
                     if (credit_counter < 112) {
                         (credit_counter) += 16; //printf("got CREDIT16 -> cpt= %d\n",credit_counter);
                     } else printf("got CREDIT16 too much %d\n",credit_counter);
                     break;
                 case CREDIT64:
                     if (credit_counter < 64) {
                         credit_counter += 64; //printf("got CREDIT64 -> cpt= %d\n",credit_counter);
                     } else printf("got CREDIT64 too much %d\n",credit_counter);
                     break;
                 default:
                     //other control or data token received
                     //printf("[%x]-",tokenrx);
                     if (!isnull(crx)) {        //send token received over channel, if given
                         if (tokenrx >= 0x100)
                              outct(crx,tokenrx & 0xFF);
                         else outt(crx,tokenrx);
                     } else
                         queue_put(qrx,tokenrx);//otherwise store in receiving queue
                     if (tokenrx < 0x1E0) credits_issued -= 1;
                     if (credits_issued < 8) {
                         //send additional credit
                         linklevel_token = CREDIT16;
                         credits_issued += 16;
                     } // enough credit
                     break;
                 } // switch tokenrx
                break; // case staterx == 10
            } // switch staterx
        } // if bit < 2
    } // while 1
} }


void treat_vlink(chanend ?crx, chanend ?ctx){ unsafe {
    queue_item_t data, prev=-1;
    unsigned countok=0,countbad=0;
    volatile queue_t * unsafe qrx = &queue_rx;
    while (1) {
        unsigned ct;
        select {
            // if we receive data from a channel, then put value in the tx queue
            case !isnull(crx) => testct_byref(crx, ct):
                if(ct) {
                    ct = inct(crx);
                    if (ct == 1) { outct(crx,1); break; } // handshake closing route
                    ct |= 256;
                } else ct = inuchar(crx);
                queue_put(qrx,ct);
                break;
            default:
                if (queue_items(qrx)) {
                    (queue_get(qrx, data));
                    if (data == 0x1E5) printf("*** timeout 4 bits ***\n");
                    else {
                        prev++; if (prev==256) prev=0;
                        if (data != prev) {
                            countbad++;
                            printf("prev %x, data %x, bad = %ld, ok %ld\n",prev,data,countbad,countok);
                            prev = data;
                        } else {
                            countok++;
                            if ((countok & 15)==0) printf("bad = %ld, ok %ld\n",countbad,countok);
                            outt(ctx,data);
                        }
                    }
                }
                break;
        }
    }
} }

port ptx  = on tile[1] : XS1_PORT_4F;    //out b0=tx0,b1=tx1,b2=nc,b3=nc
port prx0 = on tile[1] : XS1_PORT_1C;    // in rx0
port prx1 = on tile[1] : XS1_PORT_1D;    // in rx1
port prx  = on tile[1] : XS1_PORT_4C;

int main()  {
    //chan chanrx;
    chan chantx;
    par
    {

        on tile[0] : t0(tile[0], 7 , 500, 500); // prepare link 7 and push some data, lower speed (max = 2047)

        on tile[1] : virtual_link(  ptx,null,   // transmiting on one 4 bits port
                                    prx0,prx1,  // receiving on two 1 bit ports
                                    500/5,500/5, // intra inter (div by 5 as switch frequency is 5 times tile frequency)
                                    chantx, null);// using channels or queues

       on tile[1] : treat_vlink(null,chantx); // printing incoming data from virtual receiver link, via rx channel or rx queue
    }

return 0;
}
#endif
