#ifndef __sgi_template_complex_h
#define __sgi_template_complex_h

/*
 * Copyright 1997, Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
 * the contents of this file may not be disclosed to third parties, copied or
 * duplicated in any form, in whole or in part, without the prior written
 * permission of Silicon Graphics, Inc.
 *
 * RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software clause at DFARS 252.227-7013, and/or in similar or
 * successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
 * rights reserved under the Copyright Laws of the United States.
 */


// This header declares the template class complex, as described in 
//   in the draft C++ standard.  Single-precision complex numbers
//   are complex<float>, double-precision are complex<double>, and
//   quad precision are complex<long double>.

// The template complex is declared within namespace std.  This 
//   header imports it into the global namespace with a "using"
//   declaration.  If you include the header <complex>, then the
//   template complex will be declared in namespace std and not 
//   imported into the global namespace.

// If you are migrating code from an earlier compiler release, then
//   you can either modify your code (by changing all instances of
//   complex to complex<double>) or else you can continue to use the
//   non-template class.  You can select the non-template complex
//   class by defining the macro _NON_TEMPLATE_COMPLEX before including
//   this header.

// The template-based complex class is only supported on the MIPSpro 
//   (-n32 and -64) compilers.  If you are using the ucode (-o32 or 
//   -32) compiler, then this header will automatically define 
//   _NON_TEMPLATE_COMPLEX.

// Regardless of whether you are using the newer template-based
//   complex class or the older non-template class, the complex
//   library is not automatically loaded by the C++ compiler.  You 
//   must explicitly provide a command-line option telling the 
//   compiler to link the complex library.  (This is expected to 
//   change in a future release.)  You must also include the math 
//   library, -lm.
// To use the newer template-based complex library, include the options
//   -lComplex -lm
// To use the older non-template complex library, include the options
//   -lcomplex -lm

#if _MIPS_SIM == _ABIO32
#define _NON_TEMPLATE_COMPLEX
#endif

#ifdef _NON_TEMPLATE_COMPLEX
#include <deprecated/complex.h>
#else

#include <complex>
using std::complex;

using std::real;
using std::imag;
using std::abs;
using std::arg;
using std::norm;
using std::conj;
using std::polar;

using std::sqrt;
using std::exp;
using std::log;
using std::log10;
using std::pow;
using std::sin;
using std::cos;
using std::tan;
using std::sinh;
using std::cosh;
using std::tanh;

#endif /* _NON_TEMPLATE_COMPLEX */

#endif /* __sgi_template_complex_h */
