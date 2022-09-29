/*
exception.h -- Include file for exception handling (see 18.6)
*/
#ifndef _CXX_EXCEPTION_H
#define _CXX_EXCEPTION_H

#include <exception>

#ifdef _NAMESPACES
using std::exception;
using std::bad_exception;
using std::terminate_handler;
using std::set_terminate;
using std::terminate;
using std::unexpected_handler;
using std::set_unexpected;
using std::unexpected;
#endif /* _NAMESPACES */

#if defined(_SGI_SOURCE) && !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)
/* Include .../include/exception.h */
#include "../exception.h"
#endif
#endif /* _CXX_EXCEPTION_H */

