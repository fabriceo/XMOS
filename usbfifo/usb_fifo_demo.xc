
#include "usb_fifo.h"

int * unsafe g_aud_from_host_buffer;
int * unsafe g_aud_to_host_buffer;
int * unsafe g_aud_from_host_dptr;
int * unsafe g_aud_to_host_dptr;


void usbbuffer() {

    // initialisation here ...

    while(1) {
        /* Wait for response from XUD  */
        select {

        /* Received Audio packet HOST -> DEVICE */
        case XUD_GetData_Select(c_aud_out, ep_aud_out, length, result): unsafe {

            // write effective length in the buffer[0] for latest write buffer.
            usbfifo_SetWriteSize(pusbfifoOUT, length);
            // get a new buffer for the futur USB packet
            int * unsafe newbuffer;
            if (usbfifo_Ready(pusbfifoOUT)) newbuffer = usbfifo_NextWriteBuff(pusbfifoOUT);
            else newbuffer = usbfifo_LastWriteBuff(pusbfifoOUT);
            // verify if we can request a new buffer to the host now
            // otherwise will be done by SOF event
            if (! usbfifo_Overflow(pusbfifoOUT))
                XUD_SetReady_OutPtr(ep_aud_out,  (int)newbuffer + 4);

        } break;

        /* XMOS has Sent audio packet DEVICE -> HOST */

        case XUD_SetData_Select(c_aud_in, ep_aud_in, result): unsafe {

            // get a new buffer containing data comming from ADC or SPDIF

            int * unsafe hostbuffer;
            int datalength;
            if (usbfifo_Ready(pusbfifoIN)) {
                hostbuffer = usbfifo_ReadNext(pusbfifoIN);
                datalength = hostbuffer[0];
            } else {
                hostbuffer = usbfifo_Zeroes(pusbfifoIN);
                datalength = 0;
            }

            if (usbfifo_IsEmpty(pusbfifoIN) || (datalength == 0)) { // then we have to compute our dynamic buffer size ourself
                // same code as in decouple_handler for calculating the number of samples needed according to our speed
                int speed;
                GET_SHARED_GLOBAL(speed, g_speed);
                speedRemain += speed;
                int samps = speedRemain >> 16; // because the format is 16:16, eg 0x00060000 for 48khz
                speedRemain &= 0xffff;

                // put an effective length of data in the buffer
                datalength = samps * numChansSubSlotIn;
                //xprintf("fifo empty %d\n",datalength);

                //usbfifo_SetWriteSize(pusbfifoIN, datalength);
            } //else xprintf("good %d 0x%X %d\n",datalength,hostbuffer[1],usbfifo_CheckReadZeroes(pusbfifoIN));

            if (datalength >= 1024) xprintf("**** ERROR datalength > 1024 = %d***\n",datalength);
            else
                // inform XUD that a packet is ready to be sent
                XUD_SetReady_InPtr(ep_aud_in, (int)hostbuffer + 4, datalength);

        } break;


        /* SOF notifcation from XUD_Manager() */

        case inuint_byref(c_sof, u_tmp): {

            if (usbfifo_Ready(pusbfifoOUT))
                if (usbfifo_Overflow(pusbfifoOUT)) {
                    int hostbuffer = (int)usbfifo_CheckEndOverflow(pusbfifoOUT);
                    if (hostbuffer) {
                        XUD_SetReady_OutPtr(ep_aud_out,  hostbuffer + 4);
                    }
            }

        } break;

        }   // select
    } // while 1
}


void decouple_out() {

    int * unsafe ptr = g_aud_from_host_dptr;

    switch(g_curSubSlot_Out) {
    case 2:
    case 3: // treating packet here
    case 4:
    }

    // check if we have read all sample from current buffer, then request a new one

    if (usbfifo_CheckReadMax(&usbfifo_OUT,ptr)) {
        g_aud_from_host_dptr = usbfifo_ReadNext(&usbfifo_OUT)+1;
        unpackState = 0;
    } else
        g_aud_from_host_dptr = ptr;

}


void decouple_in(){

    if (sampsToWrite == 0) { // usb packet finished or never started : prepare for new buffer

        if (usbfifo_Overflow(&usbfifo_IN)) {
            int * unsafe wr = usbfifo_CheckEndOverflow(&usbfifo_IN);
            if (wr)  g_aud_to_host_dptr = wr+1;
            else return;
        }

        packState = 0;
        int speed;
        GET_SHARED_GLOBAL(speed, g_speed);
        speedRemain += speed;
        sampsToWrite = speedRemain >> 16; // because the format is 16:16, eg 0x00060000 for 48khz
        speedRemain &= 0xffff;

        // put an effective length of data in the buffer
        int size = sampsToWrite * g_numChanSubSlot_In;

        usbfifo_SetWriteSize(&usbfifo_IN, size);
    }

    int * unsafe ptr = g_aud_to_host_dptr;

    /* Store samples from audio task or mixer into sample buffer for host */
    switch(g_curSubSlot_In) {
    case 2:
    case 3: // treating packet here
    case 4:
    }

    sampsToWrite--;

    if (sampsToWrite)
         g_aud_to_host_dptr = ptr;  // store pointer  which has changed due to writing sample above

    else
        // get another write buffer and set overflow in case of
        g_aud_to_host_dptr = usbfifo_NextWriteBuff(&usbfifo_IN) + 1;

}

// inits and others:
void aud_from_host_reset(){ unsafe {
    usbfifo_OUT.ready = 0;
    delay_ticks(5000);  // wait 50us to let time to finish current usb transaction if any
    // this return a pointer on the first buffer ready for writing
    g_aud_from_host_buffer = usbfifo_Reset(&usbfifo_OUT);
    // this returns a pointer to Zeroes table (to send 0 to Audio)
    g_aud_from_host_dptr = usbfifo_OUT.rbuff + 1;
    setupDefaultBufferSizeOut();
    unpackState  = 0;
    usbfifo_OUT.ready = 1;
} }


int aud_to_host_reset(){ unsafe {
    usbfifo_IN.ready = 0;
    delay_ticks(5000); // wait 50us to let time to finish current usb transaction if any
    // this return a pointer on the first buffer ready for writing data comming from ADC / SPDIF
    g_aud_to_host_dptr   = usbfifo_Reset(&usbfifo_IN) + 1;
    // this returns a pointer to Zeroes table (to send 0 to usb host until underflow is gone)
    g_aud_to_host_buffer = usbfifo_ReadNext(&usbfifo_IN);
    sampsToWrite = 0;
    speedRemain = 0;
    int length = SetupZerosSendBuffer();
    usbfifo_IN.ready = 1;
    return length;
} }


