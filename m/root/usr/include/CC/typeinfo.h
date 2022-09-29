/*
typeinfo.h -- Include file for type information (18.5.1)
*/
#ifndef _TYPEINFO_H
#define _TYPEINFO_H

#include <typeinfo>

#ifdef _NAMESPACES
using std::exception;
using std::bad_exception;
using std::terminate_handler;
using std::set_terminate;
using std::terminate;
using std::unexpected_handler;
using std::set_unexpected;
using std::unexpected;
using std::type_info;
using std::bad_cast;
using std::bad_typeid;
#endif /* _NAMESPACES */

#endif /* _TYPEINFO_H */
