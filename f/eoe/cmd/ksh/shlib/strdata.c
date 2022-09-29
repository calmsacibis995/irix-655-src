/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ksh:shlib/strdata.c	1.1.4.1"

/*
 * data for string evaluator library
 */

#include	"streval.h"

const struct Optable optable[] =
	/* opcode	precedence,assignment  opname */
{
	{DEFAULT,	MAXPREC|NOASSIGN,	0177, 0L },
	{DONE,		0|NOASSIGN,		0L , 0L },
	{NEQ,		9|NOASSIGN,		L'!', L'=' },
	{NOT,		MAXPREC|NOASSIGN,	L'!', 0L },
	{MOD,		13|NOFLOAT,		L'%', 0L },
	{ANDAND,	5|NOASSIGN|SEQPOINT,	L'&', L'&' },
	{AND,		8|NOFLOAT,		L'&', 0L },
	{LPAREN,	MAXPREC|NOASSIGN|SEQPOINT,L'(', 0L },
	{RPAREN,	1|NOASSIGN,		L')', 0L },
	{TIMES,		13,			L'*', 0L },
#ifdef future
	{PLUSPLUS,	14|NOASSIGN|NOFLOAT|SEQPOINT, L'+', L'+'},
#endif
	{PLUS,		12,			L'+', 0L },
#ifdef future
	{MINUSMINUS,	14|NOASSIGN|NOFLOAT|SEQPOINT, L'-', L'-'},
#endif
	{MINUS,		12,			L'-', 0L },
	{DIVIDE,	13,			L'/', 0L },
#ifdef future
	{COLON,		2|NOASSIGN,		L':', 0L },
#endif
	{LSHIFT,	11|NOFLOAT,		L'<', L'<' },
	{LE,		10|NOASSIGN,		L'<', L'=' },
	{LT,		10|NOASSIGN,		L'<', 0L },
	{EQ,		9|NOASSIGN,		L'=', L'=' },
	{ASSIGNMENT,	2|RASSOC,		L'=', 0L },
	{RSHIFT,	11|NOFLOAT,		L'>', L'>' },
	{GE,		10|NOASSIGN,		L'>', L'=' },
	{GT,		10|NOASSIGN,		L'>', 0L },
#ifdef future
	{QCOLON,	3|NOASSIGN|SEQPOINT,	L'?', L':' },
	{QUEST,		3|NOASSIGN|SEQPOINT|RASSOC,	L'?', 0L },
#endif
	{XOR,		7|NOFLOAT,		L'^', 0L },
	{OROR,		5|NOASSIGN|SEQPOINT,	L'|', L'|' },
	{OR,		6|NOFLOAT,		L'|', 0L }
};


#ifndef KSHELL
    const wchar_t e_number[]	= "bad number";
#endif /* KSHELL */
const wchar_t e_moretokens[]	= L"more tokens expected";
const wchar_t wc_e_paren[]	= L"unbalanced parenthesis";
const char e_paren[]		= "unbalanced parenthesis";
const wchar_t e_badcolon[]	= L"invalid use of :";
const wchar_t e_divzero[]	= L"divide by zero";
const wchar_t wc_e_synbad[]	= L"syntax error";
const char e_synbad[]		= "syntax error";
const wchar_t e_notlvalue[]	= L"assignment requires lvalue";
const char e_recursive[]	= "recursion too deep";
const wchar_t wc_e_recursive[]	= L"recursion too deep";
#ifdef future
    const wchar_t e_questcolon[]	= L": expected for ? operator";
#endif
#ifdef FLOAT
    const wchar_t e_incompatible[]= L"operands have incompatible types";
#endif /* FLOAT */

const wchar_t e_hdigits[] = L"00112233445566778899aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ";
