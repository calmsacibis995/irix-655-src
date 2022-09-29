/* Copyright (C) 1989 Silicon Graphics, Inc. All rights reserved.  */
#ifndef __DISASSEMBLER_H__
#define __DISASSEMBLER_H__
/*
 * |-----------------------------------------------------------|
 * | Copyright (c) 1991, 1990 MIPS Computer Systems, Inc.      |
 * | All Rights Reserved                                       |
 * |-----------------------------------------------------------|
 * |          Restricted Rights Legend                         |
 * | Use, duplication, or disclosure by the Government is      |
 * | subject to restrictions as set forth in                   |
 * | subparagraph (c)(1)(ii) of the Rights in Technical        |
 * | Data and Computer Software Clause of DFARS 252.227-7013.  |
 * |         MIPS Computer Systems, Inc.                       |
 * |         950 DeGuigne Avenue                               |
 * |         Sunnyvale, California 94088-3650, USA             |
 * |-----------------------------------------------------------|
 */
/* $Header: /hosts/bonnie.mti/depot/cmplrs.src/v7.2+/include/RCS/disassembler.h,v 7.7 1996/04/25 23:00:55 zaineb Exp $ */

/* Disassembler package */

#ifdef __cplusplus
extern "C" {
#endif
#include <libelf.h>
#include <sgidefs.h>
#include <sys/elftypes.h>
/* Three sets of register names which people might want to use:
  compiler: zero, at, v0, ...
  hardware: r0, r1, r2, ...
  assembler: $0, $at, $2,...
*/
#define COMPILER_NAMES dis_reg_names[0]
#define HARDWARE_NAMES dis_reg_names[1]
#define ASSEMBLER_NAMES dis_reg_names[2]

#if (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))

extern char *dis_reg_names[3][32];

/* Initialize disassembler and set options for disassembly.

  "addr_format" and "value_format" specify in the style of "printf" the
  null-terminated formats for printing the address and value of the
  instruction. If nil, they default to "%#010x\t"; if they are the empty
  string, we omit to print these items.

  "reg_names" is a pointer to an array of 32 strings which we will use to
  represent the general-purpose registers. The *_NAMES macros above give
  three common choices; if nil, we use compiler names.

  print_jal_targets tells us whether to print the targets of jal
  instructions numerically.
*/
void
dis_init(
  char *addr_format,
  char *value_format,
  char *reg_names[],
  int print_jal_targets
  );

void
dis_init32(
  char *addr_format,
  char *value_format,
  char *reg_names[],
  int print_jal_targets
  );

void
dis_init64(
  char *addr_format,
  char *value_format,
  char *reg_names[],
  int print_jal_targets
  );

/* Given a null-terminated buffer (presumably from "disasm"), we append an
  ascii representation of register values in the form "\t<$5=0x50,$7=0x44,...>".
  "regmask" is a bitmask of registers as in "disasm", "reg_values" an array
  (with empty slots for the registers not indicated in the mask) of their
  values */
void
dis_regs(
  char *buffer,
  unsigned regmask,
  unsigned reg_values[]
  );

void
dis_regs32(
  char *buffer,
  unsigned regmask,
  unsigned reg_values[]
  );

void
dis_regs64(
  char *buffer,
  unsigned regmask,
  __uint64_t reg_values[]
  );

/* Disassemble instruction, putting null-terminated result into "buffer"
  (no trailing newline). Details governed by "dis_init".
  buffer: array of characters allocated by the caller; 64 bytes should be
    more than enough.
  address: byte address
  iword: the instruction
  regmask: returns a bitmask of the gp registers the instruction uses,
    with the LSB indicating register 0 regardless of endianness.
  symbol_value: for a jal instruction, the target value; for a load or
    store, the immediate value
  ls_register: for a load or store, the number of the base register
  return value: -1 for a load or store, 1 for a jal, 2 for a j, jr, jalr,
    or branch, 0 for others
*/
int
disasm(
  char *buffer,
  Elf32_Addr address, 
  Elf32_Addr iword, 
  Elf32_Addr *regmask, 
  Elf32_Addr *symbol_value, 
  Elf32_Addr *ls_register
  );

int
disasm32(
  char *buffer,
  Elf32_Addr address, 
  Elf32_Addr iword, 
  Elf32_Addr *regmask, 
  Elf32_Addr *symbol_value, 
  Elf32_Addr *ls_register
  );

int
disasm64(
  char *buffer,
  Elf64_Addr address, 
  Elf32_Addr iword, 
  Elf32_Addr *regmask, 
  Elf64_Addr *symbol_value, 
  Elf32_Addr *ls_register
  );

/* Older interface, which always prints on stdout */

int
disassembler(Elf32_Addr iadr, int regstyle, char *(*get_symname)(),
  int (*get_regvalue)(), long (*get_bytes)(), void (*print_header)()
  );

int
disassembler32(Elf32_Addr iadr, int regstyle, char *(*get_symname)(),
  int (*get_regvalue)(), long (*get_bytes)(), void (*print_header)()
  );


int
disassembler64(__uint64_t iadr, int regstyle, char *(*get_symname)(),
  int (*get_regvalue)(), long (*get_bytes)(), void (*print_header)()
  );

#endif /* (defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS))*/
#ifdef _LANGUAGE_PASCAL

type
  charptr = ^char;
  char32ptr = packed array [0 .. 31] of charptr;
  card32array = array [0 .. 31] of cardinal;

var
  dis_reg_names: packed array [0 .. 2] of char32ptr;

{ In the presence of optimization, use of "charptr" instead of a pointer to
  an entire array may be dangerous, but we dont want to impose a particular
  fixed array size.  To be safe, pass your entire array by reference
  to a dummy routine so the optimizer thinks its being used. }
procedure dis_init(addr_format: charptr;
  value_format: charptr;
  var reg_names: char32ptr;
  print_jal_targets: integer { nonzero => true }
  ); external;

procedure dis_regs(buffer: charptr; regmask: cardinal; reg_values: card32array);
  external;

function disasm(buffer: charptr;
  address, iword: cardinal;
  var regmask, symbol_value, ls_register: cardinal
  ): integer; { nonzero => true }
  external;

#endif /* _LANGUAGE_PASCAL */

#ifdef __cplusplus
}
#endif
#endif /* !__DISASSEMBLER_H__ */

