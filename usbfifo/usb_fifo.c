
#include "usb_fifo.h"

// declaration of the buffers

usbfifo_t usbfifo_IN  = { .wrptr = &usbfifo_IN.buffer[0][0],
                          .rdptr = &usbfifo_IN.buffer[0][0],
                          .wbuff = &usbfifo_IN.buffer[0][0],
                          .rbuff = &usbfifo_IN.zeroes[0],
                          .ready = 0,
                          .overflow = 0,
                          .underflow = &usbfifo_IN.zeroes[0],
                          .zeroes = { 0 } };

usbfifo_t usbfifo_OUT = { .wrptr = &usbfifo_OUT.buffer[0][0],
                          .rdptr = &usbfifo_OUT.buffer[0][0],
                          .wbuff = &usbfifo_OUT.buffer[0][0],
                          .rbuff = &usbfifo_OUT.zeroes[0],
                          .ready = 0,
                          .overflow = 0,
                          .underflow = &usbfifo_IN.zeroes[0],
                          .zeroes = { 0 } };

