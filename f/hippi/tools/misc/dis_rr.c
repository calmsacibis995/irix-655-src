/*
 * Disassembler for dumping RoadRunner object code.
 * Filename to be supplied as argument should contain ascii hex
 * strings, one instruction per line.
 *
 * Each word on the line may be preceded by a memory location, if
 * so, use the "-l" switch to indicate that memory locations are
 * supplied. Otherwise, we assume that the first instructions proceed
 * sequentially at address 0. You can also use the "-s <startaddress>"
 * argument to supply some other starting address.
 * 
 */ 
/* $Date: 1996/06/03 19:00:03 $	$Revision: 1.1 $
 *
 * $Log: dis_rr.c,v $
 * Revision 1.1  1996/06/03 19:00:03  irene
 * clean up compiler warnings
 *
 * Revision 1.0  1996/05/14  19:02:37  irene
 * No Message Supplied
 *
 *
 */
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

int use_startaddr;
int startaddr;
int locations;
char *cmdname;
char *fname;

void disassemble_line (uint_t address, uint_t instruction);
void disassemble_file();

void
usage()
{
    fprintf (stderr, 
	     "USAGE: %s [-h] [-l | -s <startaddress>] -f <filename>\n",
	     cmdname);
}

void
help()
{
    printf ("USAGE: %s [-h] [-l | -s <startaddress>] -f <filename>\n", 
	    cmdname);
    printf ("\t-h\n");
    printf ("\t\tPrint this help message.\n");
    printf ("\t-l\n");
    printf ("\t\tFile contains memory locations along with instructions\n");
    printf ("\t-s <startaddress>\n");
    printf ("\t\tFile does not contain memory locations. Start at this address\n");
    printf ("\t-f <filename>\n");
    printf ("\t\tName of file containing instructions to be disassembled.\n");
}

#define ITYP 1
#define JTYP 2
#define RTYP 3

typedef struct {
    char    *opstring;
    int	    optype;
    int	    opcode;
} instr_t;

instr_t instr_tab[] = {
	{"lbu",		ITYP,	1},
	{"lhu",		ITYP,	2},
	{"lw",		ITYP,	5},
	{"ldw",		ITYP,	6},
	{"ldwp",	ITYP,	7},
	{"sw",		ITYP,	8},
	{"sdw",		ITYP,	9},

	{"addi",	ITYP,	0x10},
	{"addiu",	ITYP,	0x11},
	{"andi",	ITYP,	0x20},

	{"ori",		ITYP,	0x28},
	{"xori",	ITYP,	0x24},
	{"slti",	ITYP,	0x1c},
	{"sltiu",	ITYP,	0x1d},
	{"lui",		ITYP,	0x17},

	{"addu",	RTYP,	0x12},
	{"subu",	RTYP,	0x13},
	{"and",		RTYP,	0x21},
	{"nand",	RTYP,	0x22},
	{"or",		RTYP,	0x29},
	{"nor",		RTYP,	0x2a},
	{"xor",		RTYP,	0x25},
	{"xnor",	RTYP,	0x26},
	{"slt",		RTYP,	0x1e},
	{"sltu",	RTYP,	0x1f},
	{"pri",		RTYP,	0x23},

	{"sll",		RTYP,	0x18},
	{"srl",		RTYP,	0x1a},
	{"sllv",	RTYP,	0x19},
	{"srlv",	RTYP,	0x1b},

	{"j",		JTYP,	0x30},
	{"jal",		JTYP,	0x31},
	{"jr",		RTYP,	0x32},
	{"jalr",	RTYP,	0x33},
	{"jalr",	RTYP,	0x33},
	{"joff",	RTYP,	0x3c},

	{"beq",		ITYP,	0x34},
	{"bne",		ITYP,	0x35},
	{"bltz",	ITYP,	0x36},
	{"bltzal",	ITYP,	0x37},
	{"blez",	ITYP,	0x38},
	{"bgtz",	ITYP,	0x39},
	{"bgez",	ITYP,	0x3a},
	{"bgezal",	ITYP,	0x3b},

	{"halt",	ITYP,	0x3f},
	{"",	0,	0}   
};

instr_t * lookup (int opcode);


main(int argc, char ** argv)
{
    int c;

    setbuf(stdout, 0);
    setbuf(stderr, 0);
    cmdname = argv[0];

    while ((c = getopt(argc, argv, "hls:f:")) != EOF) {
	switch (c) {
	    case 's':
		startaddr = strtoul(optarg, 0, 0);
		use_startaddr = 1;
		break;
	    case 'l':
		locations = 1;
		break;
	    case 'f':
		fname = optarg;
		break;
	    case 'h':
		help();
		exit(0);
	    default:
		usage();
		exit(1);
	}
    }
    if ((fname == NULL) || (locations && use_startaddr)) {
	usage();
	exit(1);
    }
    disassemble_file();
}

void
disassemble_file()
{
    FILE * fp;
    int line;
    int i;
    uint_t word1;
    uint_t word2;
    char aline[256];
    char junk[256];

    fp = fopen (fname, "r");
    if (fp == 0) {
	fprintf (stderr, "Couldn't open file %s: %s\n", 
		 fname, strerror(errno));
	exit(1);
    }

    line = 0;
    if (fgets (aline, sizeof(aline), fp) == 0) {
	fprintf (stderr, "File %s is empty.\n", fname);
	exit(1);
    }

    while (1) {
	line++;
	i = sscanf (aline, "%x %x %s\n", &word1, &word2, junk);
	if (locations) {
	    if (i != 2) {
		fprintf (stderr, "Line %d: bad format\n", line);
		exit(1);
	    }
	    disassemble_line (word1, word2);
	} else {
	    if (i != 1) {
		fprintf (stderr, "Line %d: bad format\n", line);
		exit(1);
	    }
	    disassemble_line(startaddr, word1);
	    startaddr += 4;
	}
	if (fgets (aline, sizeof(aline), fp) == 0)
	    exit(0);
    }
}

void
disassemble_line (uint_t address, uint_t instruction)
{
    int opcode;
    int	rs, rt, rd;
    int	immed;
    int	target;
    int	shamt;
    instr_t * ip;

    printf ("%08x\t%08x:\t", address, instruction);
    opcode = (instruction >> 26);
    if ((ip = lookup (opcode)) == NULL) {
	printf ("????\n");
	return;
    }

    switch (ip->optype) {
    case ITYP:
	rs = (instruction >> 21) & 0x1f;
	rt = (instruction >> 16) & 0x1f;
	immed = instruction & 0xffff;
	switch (opcode) {
	case 0x3f:
	    printf ("%s\n", ip->opstring);
	    break;
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3a:
	case 0x3b:
	    printf ("%s\tr%d,0x%x\n", ip->opstring, rs, immed);
	    break;
	case 0x17:
	    printf ("%s\tr%d,0x%x\n", ip->opstring, rt, immed);
	    break;
	default:
	    printf ("%s\tr%d,r%d,0x%x\n", ip->opstring, rt, rs, immed);
	    break;
	}    
	break;
    case JTYP:
	target = instruction & 0x03ffffff;
	printf ("%s\t0x%x\n", ip->opstring, target);
	break;
    case RTYP:
	rs = (instruction >> 21) & 0x1f;
	rt = (instruction >> 16) & 0x1f;
	rd = (instruction >> 11) & 0x1f;
	shamt = (instruction >> 6) & 0x1f;
	switch (opcode) {
	case 0x18:
	case 0x1a:
	    printf ("%s\tr%d,r%d,%d\n", ip->opstring,rd,rt,shamt);
	    break;
	case 0x19:
	case 0x1b:
	    printf ("%s\tr%d,r%d,r%d\n", ip->opstring,rd,rt,rs);	    
	    break;
	case 0x32:
	    printf ("%s\tr%d\n", ip->opstring,rs);
	    break;
	case 0x33:
	    printf ("%s\tr%d,r%d\n", ip->opstring,rd,rs);
	    break;
	case 0x3c:
	    printf ("%s\tr%d,r%d\n", ip->opstring,rs,rt);
	    break;
	default:
	    printf ("%s\tr%d,r%d,r%d\n", ip->opstring,rd,rs,rt);
	    break;
	}
    }
}

/* Returns a pointer to the instr_t for this opcode.
 * Returns NULL if no such opcode.
 */
instr_t *
lookup (int opcode)
{
    instr_t * ip;

    for (ip = instr_tab; ; ip++) {
	if (ip->opcode == 0)
	    return NULL;
	if (ip->opcode == opcode)
	    return ip;
    }
}
