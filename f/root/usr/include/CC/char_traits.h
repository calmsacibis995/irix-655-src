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

#ifndef __SGI_STL_CHAR_TRAITS_H
#define __SGI_STL_CHAR_TRAITS_H

#include <string.h>
#include <wchar.h>

namespace std {

// Class __char_traits_base.

template <class charT, class intT> struct __char_traits_base {
  typedef charT char_type;
  typedef intT int_type;
  // typedef streamoff off_type;
  // typedef streampos pos_type;
  // typedef mbstate_t state_type;

  static void assign(char_type& c1, const char_type& c2) { c1 = c2; }
  static bool eq(const charT& c1, const charT& c2) { return c1 == c2; }
  static bool lt(const charT& c1, const charT& c2) { return c1 < c2; }

  static int compare(const charT* s1, const charT* s2, size_t n) {
    for (size_t i = 0; i < n; ++i)
      if (!eq(s1[i], s2[i]))
        return s1[i] < s2[i] ? -1 : 1;
    return 0;
  }

  static size_t length(const charT* s) {
    const charT null = charT();
    size_t i;
    for (i = 0; !eq(s[i], null); ++i)
      {}
    return i;
  }

  static const charT* find(const charT* s, size_t n, const charT& c) {
    for ( ; n > 0 ; ++s, --n)
      if (eq(*s, c))
        return s;
    return 0;
  }

  static charT* move(charT* s1, const charT* s2, size_t n) {
    memmove(s1, s2, n * sizeof(charT));
    return s1;
  }
    
  static charT* copy(charT* s1, const charT* s2, size_t n) {
    memcpy(s1, s2, n * sizeof(charT));
    return s1;
  } 

  static charT* assign(charT* s, size_t n, charT c) {
    for (size_t i = 0; i < n; ++i)
      s[i] = c;
    return s;
  }

  static int_type not_eof(const int_type& c) {
    return !eq(c, eof()) ? c : 0;
  }

  static char_type to_char_type(const int_type& c) {
    return static_cast<char_type>(c);
  }

  static int_type to_int_type(const char_type& c) {
    return static_cast<int_type>(c);
  }

  static bool eq_int_type(const int_type& c1, const int_type& c2) {
    return c1 == c2;
  }

  static int_type eof() {
    return static_cast<int_type>(-1);
  }
};

// Generic char_traits class.  Note that this class is provided only
//  as a base for explicit specialization; it is unlikely to be useful
//  as is for any particular user-defined type.  In particular, it 
//  *will not work* for a non-POD type.

template <class charT> struct char_traits
  : public __char_traits_base<charT, charT>
{};

// Specialization for char.

template<> struct char_traits<char> 
  : public __char_traits_base<char, int>
{
  static int compare(const char* s1, const char* s2, size_t n) 
    { return strncmp(s1, s2, n); }
  
  static size_t length(const char* s) { return strlen(s); }

  static void assign(char& c1, const char& c2) { c1 = c2; }

  static char* assign(char* s, size_t n, char c)
    { memset(s, c, n); return s; }
};

// Specialization for wchar_t.

template<> struct char_traits<wchar_t>
  : public __char_traits_base<wchar_t, wint_t>
{};

} // Close namespace std.

#endif /* __SGI_STL_CHAR_TRAITS_H */

// Local Variables:
// mode:C++
// End:

