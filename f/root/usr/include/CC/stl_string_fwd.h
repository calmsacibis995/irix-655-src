/*
 * Copyright (c) 1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */ 

#ifndef __SGI_STL_STRING_FWD_H
#define __SGI_STL_STRING_FWD_H

#include <stddef.h>

namespace std {

template <bool threads, int inst> class __default_alloc_template;
typedef __default_alloc_template<true, 0> alloc;

template <class charT> struct char_traits;
template <class charT, 
          class traits = char_traits<charT>, 
          class Alloc = alloc>
class basic_string;

typedef basic_string<char> string;
typedef basic_string<wchar_t> wstring;

template <class charT, class traits, class Alloc>
void __string_copy(const basic_string<charT, traits, Alloc>&, charT*, size_t);

} // Close namespace std.

#endif /* __SGI_STL_STRING_FWD_H */

// Local Variables:
// mode:C++
// End:
