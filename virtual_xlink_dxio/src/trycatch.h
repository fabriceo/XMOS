// Copyright (c) 2016, XMOS Ltd, All rights reserved

#ifndef _trycatch_h_
#define _trycatch_h_

/// \file trycatch.h
/// \brief Macros to handle hardware exceptions
///
/// This file contains macros that allow you to handle hardware exceptions
/// raised on the current logical core.

/// Structure describing a hardware exception.
typedef struct exception_t {
  /// Exception type.
  unsigned type;
  /// Exception data.
  unsigned data;
} exception_t;

#include "trycatch_impl.h"

/// Macro to execute a block of code catching any raised hardware exceptions.
/// The TRY macro must be immediately followed by a CATCH macro as
/// follows:
/// \code
/// exception_t exception;
/// TRY { ... } CATCH(exception) { ... }
/// \endcode
/// If a hardware exception is raised, execution jumps to the code inside the
/// catch block. The operand of the CATCH macro populated with information about
/// the raised exception. The catch block is not executed if no exception is
/// raised.
///
/// The TRY and CATCH macros are implemented using setjmp() and longjmp() and
/// have the following limitations.
///
///   * xCORE resources allocated inside the TRY block may not be freed if a
///     hardware exception is raised.
///   * If a hardware exception is raised the values of local variables changed
///     inside the try block are indeterminate.
///   * If the code inside the try block spawns task onto additional logical
///     cores, exceptions on these logical cores will not be caught.
#define TRY \
  { \
    typecatch_try_t trycatch_try; \
    if (setjmp(trycatch_try.buf) == 0) { \
      trycatch_enter(ADDRESS_OF trycatch_try);

/// Macro for catching a hardware exception. This should be used in conjunction
/// with the TRY macros, see documentation of TRY for more details.
#define CATCH(exception) \
    } \
    trycatch_exit(); \
  } \
  if (trycatch_catch(exception))

#endif //_trycatch_h_
