#ifndef USB_FIFO_H
#define USB_FIFO_H

//fifoUSBout
/*
 * this is the number of USB buffers managed by the fifo, for usb input and usb outputs
 * each buffer is 1024 bytes + 4 for the lenght of the data.
 * it is preferable to have an even number here, as some routines considers half of the buffer.
 */
#define USB_BUFFERS (16)
#define USB_BUFFERS_HALF (USB_BUFFERS>>1)
#define USB_PACKET_BYTES (4+1024)
#define USB_PACKET_WORDS (USB_PACKET_BYTES/4)
#define USB_BUFFERS_SIZE (USB_BUFFERS*USB_PACKET_WORDS)
#define USB_BUFFERS_SIZE_BYTES (USB_BUFFERS*USB_PACKET_BYTES)
#define USB_BUFFERS_HALF_WORDS ((USB_BUFFERS>>1)*USB_PACKET_WORDS)
#define USB_BUFFERS_HALF_BYTES ((USB_BUFFERS>>1)*USB_PACKET_BYTES)

#if defined (__XC__)
#ifndef XCunsafe
#define XCunsafe unsafe
#endif
#else
#ifndef XCunsafe
#define XCunsafe
#endif
#endif


typedef struct usbfifo_s {
    int *  XCunsafe wrptr;   // 0  pointer for writing data
    int *  XCunsafe rdptr;   // 4  pointer for reading data
    int *  XCunsafe wbuff;   // 8  last writing data pointer given to caller
    int *  XCunsafe rbuff;   // 12 last reading data pointer given to caller
    int ready;               // 0  by default, should be set to 1 by application
    int *  XCunsafe overflow;  // represent an overflow status : write has colided read
    int *  XCunsafe underflow; // represent underflow status : read has colided write
    int buffer[USB_BUFFERS][USB_PACKET_WORDS];
    int zeroes[USB_PACKET_WORDS];
} usbfifo_t;

extern usbfifo_t usbfifo_IN;     // instance of the above structure, used for sending data from ADC/SPDIF to usb host
extern usbfifo_t usbfifo_OUT;    // same for receiving data from host

#if defined (__XC__)

#define check(pp) ((((int)pp & 3)==0) && (pp >= &f->buffer[0][0]) && (pp < &f->zeroes[USB_PACKET_WORDS]))

#pragma unsafe arrays
static inline int * XCunsafe usbfifo_Init(usbfifo_t * f)  { XCunsafe {
    f->ready = 0;
    f->wrptr = &f->buffer[0][0];
    f->rdptr = &f->buffer[0][0];
    f->wbuff = &f->buffer[0][0];
    f->rbuff = &f->zeroes[0];
    f->overflow  = null;
    f->underflow = f->rbuff;
    f->zeroes[0] = 0;
    return f->wrptr;
} }

/*
 * initialize zeroes buffer and read/write ptr.
 * returns a pointer on the first buffer available for writing.
 */
#pragma unsafe arrays
static inline int * XCunsafe usbfifo_Reset(usbfifo_t * f) { XCunsafe {
    int * XCunsafe ptr = usbfifo_Init(f);
    for (int i=0; i<USB_PACKET_WORDS;i++) f->zeroes[i] = 0;
    return ptr;
} }

/*
 * After writing data to the given buffer,
 * this function is called so that the write pointer is moved forward to the next buffer.
 * if there is an overflow situation, the write buffer given is the last one.
 * The new pointer is kept in memory and returned in all case.
 *
 */
#pragma unsafe arrays
static inline int * XCunsafe usbfifo_NextWriteBuff(usbfifo_t * f) { XCunsafe {
    // always move forward the write pointer as we have writen in the buffer.
    int * XCunsafe wrptr = f->wrptr + USB_PACKET_WORDS;
    // check fifo roll over
    if (wrptr >= &f->buffer[USB_BUFFERS][0]) wrptr = &f->buffer[0][0];
    f->wrptr = wrptr;
    // detect colision
    if (wrptr == f->rdptr)
        f->overflow = wrptr;
    else
        f->wbuff = wrptr;
    return f->wbuff;
} }

/*
 * this routine should be called to check the end of an overflow condition.
 * then a new write pointer is returned if half of the whole buffer is available.
 * otherwise null is returned and overflow condition remains
 *
 */
#pragma unsafe arrays
static inline int * unsafe usbfifo_CheckEndOverflow(usbfifo_t * f) { XCunsafe {
    int count = (int)f->wrptr - (int)f->rdptr;
    if (count <= 0) count += USB_BUFFERS_SIZE_BYTES; // "<=0" assumes we are on overflow
    if (count <= USB_BUFFERS_HALF_BYTES) {
        f->overflow = null;
        // provide the write pointer (already calculated earlier) that can be used as of now
        f->wbuff = f->wrptr;
        return f->wbuff; }
    return null;
} }


/*
 * calling this function will return a pointer containing either Zeroes or a valid buffer to read data from.
 * if there is no more (or not yet) buffer available, a pointer on the zeroes buffer is returned.
 * the pointer is saved in local memory (rbuff) so that a size update can be made at the right place (for zeroes and buffer)
 *
 */
#pragma unsafe arrays
static inline int * XCunsafe usbfifo_ReadNext(usbfifo_t *  f) { XCunsafe {
    // check underflow situation
    if (f->underflow) {
        int count;
        if (f->overflow) count = USB_BUFFERS_SIZE_BYTES;
        else {
            count = (int)f->wrptr - (int)f->rdptr;
            if (count < 0) count += USB_BUFFERS_SIZE_BYTES; }
        if ( count >= USB_BUFFERS_HALF_BYTES) {
            f->underflow = 0;
            // now using the normal read buffer
            f->rbuff = f->rdptr; }
    } else {
        // try to move forward the reading pointer
        int * XCunsafe rdptr = f->rdptr + USB_PACKET_WORDS;
        // check fifo rollover
        if (rdptr >= &f->buffer[USB_BUFFERS][0]) rdptr = &f->buffer[0][0];
        f->rdptr = rdptr; // commit
        // detect underflow situation/colide
        if (f->rdptr == f->wrptr) {
            // using the zero buffer as the new rdptr points at the end of the fifo
            f->rbuff = &f->zeroes[0];
            f->underflow = f->rbuff;
        } else
            // here we can use the normal buffer pointed by rdptr
            f->rbuff = f->rdptr;
    }
    return f->rbuff;
} }

/*
 * Helpers function so that the fifo information can be accessed from
 * the usb_buffer.XC without violation of the paralell usage rules...
 */
#pragma unsafe arrays
static inline int * XCunsafe usbfifo_LastReadBuff(usbfifo_t *  f) { XCunsafe {
    return f->rbuff;
}}

#pragma unsafe arrays
static inline int * XCunsafe usbfifo_LastWriteBuff(usbfifo_t *  f) { XCunsafe {
    return f->wbuff;
}}

#pragma unsafe arrays
static inline int * XCunsafe usbfifo_Overflow(usbfifo_t *  f) { XCunsafe {
    return f->overflow;
}}

#pragma unsafe arrays
static inline int * XCunsafe usbfifo_Underflow(usbfifo_t *  f) { XCunsafe {
    return f->underflow;
}}

#pragma unsafe arrays
static inline int usbfifo_Ready(usbfifo_t *  f) { XCunsafe {
    return f->ready;
}}

#pragma unsafe arrays
static inline int * XCunsafe usbfifo_Zeroes(usbfifo_t *  f) { XCunsafe {
    return &f->zeroes[0];
}}

/*
 * this is used to force the first word of the packet to a given length provided in bytes
 */
#pragma unsafe arrays
static inline int * unsafe usbfifo_SetWriteSize(usbfifo_t * f, int bytes) { XCunsafe {
    f->zeroes[0] = f->wbuff[0] = bytes;
    return f->wbuff;
} }
#pragma unsafe arrays
static inline void usbfifo_SetReadSize(usbfifo_t * f, int bytes) { XCunsafe {
    f->zeroes[0] = f->rbuff[0] = bytes;
} }

/*
 * this is used to check if a pointer as reached the maximum number/size provided in the function above
 *
 */

#pragma unsafe arrays
static inline int usbfifo_CheckReadMax(usbfifo_t * f, int * XCunsafe rptr) { XCunsafe {
    // get size of current buffer
    int bytes = f->rbuff[0];
    int ptr = (int)rptr;
    // compute position of data pointer relative to begining of current buffer
    ptr -= (int)&f->rbuff[1];
    return (ptr >= bytes);
} }


#pragma unsafe arrays
static inline int * XCunsafe usbfifo_CheckReadNext(usbfifo_t * f, int * XCunsafe rptr) { XCunsafe {
    if (usbfifo_CheckReadMax(f,rptr)) return usbfifo_ReadNext(f)+1;
    else return rptr;
} }

/*
 * return the number of buffers filled in the fifo. a started buffer counts for a full one
 */
#pragma unsafe arrays
static inline int usbfifo_Count(usbfifo_t * f) { XCunsafe {
    if (f->overflow) return USB_BUFFERS;
    int count = (int)f->wrptr - (int)f->rdptr;
    if (count < 0) count += USB_BUFFERS_SIZE_BYTES; // "< 0" because here we know we are not in overflow
    return count >> 10;
} }

/*
 * return true if the fifo is really empty
 */
#pragma unsafe arrays
static inline int usbfifo_IsEmpty(usbfifo_t * f) { XCunsafe {
    return ((! f->overflow) && (f->rdptr == f->wrptr));
}}

/*
 * used during debug
 */
#pragma unsafe arrays
static inline int usbfifo_checkBoundaries(usbfifo_t * f, int * unsafe ptr) {
    int result;
    if (ptr == null) result = check(f->rdptr) && check(f->wrptr);
    else result = check(ptr);
    return result;
}

#endif
// macro for handling these routines from a different XC task
// to bypass violation rules detection

#define usbfifo_getAddress_IN(x)  asm volatile("ldaw %0, dp[usbfifo_IN]" :"=r"(x));
#define usbfifo_getAddress_OUT(x) asm volatile("ldaw %0, dp[usbfifo_OUT]":"=r"(x));

#endif
