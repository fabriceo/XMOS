// Copyright (c) 2016, XMOS Ltd, All rights reserved

#ifndef _trycatch_impl_h_
#define _trycatch_impl_h_

#include <xccompat.h>
#include <stdint.h>

#ifdef __XC__
// In the 12.0 tools setjmp.h doesn't export setjmp when included from XC.
// Supply the necessary prototypes here.
typedef int jmp_buf[9];

int setjmp(jmp_buf buf);
void longjmp(jmp_buf buf, int);
#else
#include <setjmp.h>
#endif

typedef struct typecatch_try_t {
  jmp_buf buf;
  intptr_t prev;
  intptr_t old_handler;
} typecatch_try_t;

#ifdef __XC__
#define ADDRESS_OF
#else
#define ADDRESS_OF &
#endif

void trycatch_enter(REFERENCE_PARAM(typecatch_try_t, buf));

void trycatch_exit(void);

int trycatch_catch(REFERENCE_PARAM(exception_t, exception));

#endif //_trycatch_impl_h_
