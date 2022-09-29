/* 
stdexcept.h -- Include file for exception handling (see 19.1)
*/
#ifndef _STDEXCEPT_H
#define _STDEXCEPT_H


#include <stdexcept>

#ifdef _NAMESPACES
using std::exception;
using std::bad_exception;
using std::logic_error;
using std::runtime_error;
using std::domain_error;
using std::invalid_argument;
using std::length_error;
using std::out_of_range;
using std::range_error;
using std::overflow_error;
using std::underflow_error;
using std::terminate_handler;
using std::set_terminate;
using std::terminate;
using std::unexpected_handler;
using std::set_unexpected;
using std::unexpected;
#endif /* _NAMESPACES */


#endif /* _STDEXCEPT_H */

