/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)main.c	5.5 (Berkeley) 2/7/86";
#endif not lint

/* Many bug fixes are from Jim Guyton <guyton@rand-unix> */

/*
 * TFTP User Program -- Command Interface.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>

#include <netinet/in.h>

#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <setjmp.h>
#include <ctype.h>
#include <netdb.h>

#define	TIMEOUT		5		/* secs between rexmt's */

struct	sockaddr_in sin;
int	f;
short   port;
int	trace;
int	verbose;
int	connected;
char	mode[32];
char	line[200];
int	margc;
char	*margv[20];
char	*prompt = "tftp";
jmp_buf	toplevel;
void	intr();
struct	servent *sp;

int	quit(), help(), setverbose(), settrace(), status();
int     get(), put(), setpeer(), modecmd(), setrexmt(), settimeout();
int     setbinary(), setascii();

#define HELPINDENT (sizeof("connect"))

struct cmd {
	char	*name;
	char	*help;
	int	(*handler)();
};

char	vhelp[] = "toggle verbose mode";
char	thelp[] = "toggle packet tracing";
char	chelp[] = "connect to remote tftp";
char	qhelp[] = "exit tftp";
char	hhelp[] = "print help information";
char	shelp[] = "send file";
char	rhelp[] = "receive file";
char	mhelp[] = "set file transfer mode";
char	sthelp[] = "show current status";
char	xhelp[] = "set per-packet retransmission timeout";
char	ihelp[] = "set total retransmission timeout";
char    ashelp[] = "set mode to netascii";
char    bnhelp[] = "set mode to octet";

struct cmd cmdtab[] = {
	{ "connect",	chelp,		setpeer },
	{ "mode",       mhelp,          modecmd },
	{ "put",	shelp,		put },
	{ "get",	rhelp,		get },
	{ "quit",	qhelp,		quit },
	{ "verbose",	vhelp,		setverbose },
	{ "trace",	thelp,		settrace },
	{ "status",	sthelp,		status },
	{ "binary",     bnhelp,         setbinary },
	{ "ascii",      ashelp,         setascii },
	{ "rexmt",	xhelp,		setrexmt },
	{ "timeout",	ihelp,		settimeout },
	{ "?",		hhelp,		help },
	0
};

struct	cmd *getcmd();
char	*tail();
char	*index();
char	*rindex();

void makeargv();

main(argc, argv)
	char *argv[];
{
	struct sockaddr_in sin;
	int top;

	sp = getservbyname("tftp", "udp");
	if (sp == 0) {
		fprintf(stderr, "tftp: udp/tftp: unknown service\n");
		exit(1);
	}
	f = socket(AF_INET, SOCK_DGRAM, 0);
	if (f < 0) {
		perror("tftp: socket");
		exit(3);
	}
	bzero((char *)&sin, sizeof (sin));
	sin.sin_family = AF_INET;
	if (bind(f, &sin, sizeof (sin)) < 0) {
		perror("tftp: bind");
		exit(1);
	}
	strcpy(mode, "netascii");
	signal(SIGINT, intr);
	if (argc > 1) {
		if (setjmp(toplevel) != 0)
			exit(0);
		setpeer(argc, argv);
	}
	top = setjmp(toplevel) == 0;
	for (;;)
		command(top);
}

char    hostname[100];

setpeer(argc, argv)
	int argc;
	char *argv[];
{
	struct hostent *host;

	if (argc < 2) {
		strcpy(line, "Connect ");
		printf("(to) ");
		gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
#ifdef sgi
	if (argc < 2 || argc > 3) {
#else
	if (argc > 3) {
#endif
		printf("usage: %s host-name [port]\n", argv[0]);
		return 0;
	}
	host = gethostbyname(argv[1]);
	if (host) {
		sin.sin_family = host->h_addrtype;
		bcopy(host->h_addr, &sin.sin_addr, host->h_length);
		strcpy(hostname, host->h_name);
	} else {
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = inet_addr(argv[1]);
		if (sin.sin_addr.s_addr == -1) {
			connected = 0;
			printf("%s: unknown host\n", argv[1]);
			return 0;
		}
		strcpy(hostname, argv[1]);
	}
	port = sp->s_port;
	if (argc == 3) {
		port = atoi(argv[2]);
		if (port < 0) {
			printf("%s: bad port number\n", argv[2]);
			connected = 0;
			return 0;
		}
		port = htons(port);
	}
	connected = 1;
	return 0;
}

struct	modes {
	char *m_name;
	char *m_mode;
} modes[] = {
	{ "ascii",	"netascii" },
	{ "netascii",   "netascii" },
	{ "binary",     "octet" },
	{ "image",      "octet" },
	{ "octet",     "octet" },
/*      { "mail",       "mail" },       */
	{ 0,		0 }
};

modecmd(argc, argv)
	char *argv[];
{
	register struct modes *p;
	char *sep;

	if (argc < 2) {
		printf("Using %s mode to transfer files.\n", mode);
		return 0;
	}
	if (argc == 2) {
		for (p = modes; p->m_name; p++)
			if (strcmp(argv[1], p->m_name) == 0)
				break;
		if (p->m_name) {
			setmode(p->m_mode);
			return 0;
		}
		printf("%s: unknown mode\n", argv[1]);
		/* drop through and print usage message */
	}

	printf("usage: %s [", argv[0]);
	sep = " ";
	for (p = modes; p->m_name; p++) {
		printf("%s%s", sep, p->m_name);
		if (*sep == ' ')
			sep = " | ";
	}
	printf(" ]\n");
	return 0;
}

setbinary(argc, argv)
char *argv[];
{
	setmode("octet");
	return 0;
}

setascii(argc, argv)
char *argv[];
{
	setmode("netascii");
	return 0;
}

setmode(newmode)
char *newmode;
{
	strcpy(mode, newmode);
	if (verbose)
		printf("mode set to %s\n", mode);
	return 0;
}


void putusage(char *s);


/*
 * Send file(s).
 */
put(argc, argv)
	char *argv[];
{
	int fd;
	register int n;
	register char *cp, *targ;

	if (argc < 2) {
		strcpy(line, "send ");
		printf("(file) ");
		gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		putusage(argv[0]);
		return 0;
	}
	targ = argv[argc - 1];
	if (index(argv[argc - 1], ':')) {
		char *cp;
		struct hostent *hp;

		for (n = 1; n < argc - 1; n++)
			if (index(argv[n], ':')) {
				putusage(argv[0]);
				return 0;
			}
		cp = argv[argc - 1];
		targ = index(cp, ':');
		*targ++ = 0;
		hp = gethostbyname(cp);
		if (hp == 0) {
			printf("%s: Unknown host.\n", cp);
			return 0;
		}
		bcopy(hp->h_addr, (caddr_t)&sin.sin_addr, hp->h_length);
		sin.sin_family = hp->h_addrtype;
		port = sp->s_port;
		connected = 1;
		strcpy(hostname, hp->h_name);
	}
	if (!connected) {
		printf("No target machine specified.\n");
		return 0;
	}
	if (argc < 4) {
		cp = argc == 2 ? tail(targ) : argv[1];
		fd = open(cp, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "tftp: "); perror(cp);
			return 0;
		}
		if (verbose)
			printf("putting %s to %s:%s [%s]\n",
				cp, hostname, targ, mode);
		sin.sin_port = port;
		sendfile(fd, targ, mode);
		return 0;
	}

	/* this assumes the target is a directory */
	/* on a remote unix system.  hmmmm.  */
	cp = index(targ, '\0'); 
	*cp++ = '/';
	for (n = 1; n < argc - 1; n++) {
		strcpy(cp, tail(argv[n]));
		fd = open(argv[n], O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "tftp: "); perror(argv[n]);
			continue;
		}
		if (verbose)
			printf("putting %s to %s:%s [%s]\n",
				argv[n], hostname, targ, mode);
		sin.sin_port = port;
		sendfile(fd, targ, mode);
	}
	return 0;
}

void
putusage(s)
	char *s;
{
	printf("usage: %s file ... host:target, or\n", s);
	printf("       %s file ... target (when already connected)\n", s);
}

/*
 * Receive file(s).
 *	SGI BUG FIX:  This code was so bad that I had to rewrite 
 *		fairly large chunks of it in order to get it to
 *		match the man page.  If you end up merging in a
 *		new version, make sure that it actually parses
 *		lines of the form "get localhost:file" and
 *		"get localhost:file file2" when !connected.
 */

get(argc, argv)
	char *argv[];
{
	int fd;
	register int n;
	register char *cp;
	char *src;
	struct hostent *hp = NULL;

	if (argc < 2) {
		strcpy(line, "get ");
		printf("(files) ");
		gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		getusage(argv[0]);
		return 0;
	}

	/* If we're not connected, the first filename (at least)
	 * must provide a hostname.
	 */
	if (!connected && (index(argv[1], ':') == 0)) {
		getusage(argv[0]);
		return 0;
	}
	connected = 1;

	for (n = 1; n < argc ; n++) {
		src = index(argv[n], ':');
		if (src == NULL)
			src = argv[n];
		else {
			*src++ = 0;
			hp = gethostbyname(argv[n]);
			if (hp == 0) {
				printf("%s: Unknown host.\n", argv[n]);
				continue;
			}
			bcopy(hp->h_addr, (caddr_t)&sin.sin_addr, hp->h_length);
			sin.sin_family = hp->h_addrtype;
			port = sp->s_port;
			strcpy(hostname, hp->h_name);
		}


		cp = argc == 3 ? argv[2] : tail(src);
		fd = creat(cp, 0644);
		if (fd < 0) {
			fprintf(stderr, "tftp: "); perror(cp);
			continue;
		}
		if (verbose)
			printf("getting from %s:%s to %s [%s]\n",
				hostname, src, cp, mode);
		
		sin.sin_port = port;
		recvfile(fd, src, mode);

		if (argc == 3)
		    break;
	}
	return 0;
}

getusage(s)
char * s;
{
	printf("usage: %s host:file host:file ... file, or\n", s);
	printf("       %s file file ... file if connected\n", s);
	return 0;
}

int	rexmtval = TIMEOUT;

setrexmt(argc, argv)
	char *argv[];
{
	int t;

	if (argc < 2) {
		strcpy(line, "Rexmt-timeout ");
		printf("(value) ");
		gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc != 2) {
		printf("usage: %s value\n", argv[0]);
	} else {
		t = atoi(argv[1]);
		if (t < 0)
			printf("%s: bad value\n", argv[1]);
		else
			rexmtval = t;
	}
	return 0;
}

int	maxtimeout = 5 * TIMEOUT;

settimeout(argc, argv)
	char *argv[];
{
	int t;

	if (argc < 2) {
		strcpy(line, "Maximum-timeout ");
		printf("(value) ");
		gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc != 2) {
		printf("usage: %s value\n", argv[0]);
	} else {
		t = atoi(argv[1]);
		if (t < 0)
			printf("%s: bad value\n", argv[1]);
		else
			maxtimeout = t;
	}
	return 0;
}

status(argc, argv)
	char *argv[];
{
	if (connected)
		printf("Connected to %s.\n", hostname);
	else
		printf("Not connected.\n");
	printf("Mode: %s Verbose: %s Tracing: %s\n", mode,
		verbose ? "on" : "off", trace ? "on" : "off");
	printf("Rexmt-interval: %d seconds, Max-timeout: %d seconds\n",
		rexmtval, maxtimeout);
	return 0;
}

void
intr()
{
	signal(SIGALRM, SIG_IGN);
	alarm(0);
	longjmp(toplevel, -1);
}

char *
tail(filename)
	char *filename;
{
	register char *s;
	
	while (*filename) {
		s = rindex(filename, '/');
		if (s == NULL)
			break;
		if (s[1])
			return (s + 1);
		*s = '\0';
	}
	return (filename);
}

/*
 * Command parser.
 */
command(top)
	int top;
{
	register struct cmd *c;

	if (!top)
		putchar('\n');
	for (;;) {
		printf("%s> ", prompt);
		if (gets(line) == 0) {
			if (feof(stdin)) {
				quit();
			} else {
				continue;
			}
		}
		if (line[0] == 0)
			continue;
		makeargv();
#ifdef sgi
		if (margv[0] == NULL)
			continue;
#endif
		c = getcmd(margv[0]);
		if (c == (struct cmd *)-1) {
			printf("?Ambiguous command\n");
			continue;
		}
		if (c == 0) {
			printf("?Invalid command\n");
			continue;
		}
		(*c->handler)(margc, margv);
	}
}

struct cmd *
getcmd(name)
	register char *name;
{
	register char *p, *q;
	register struct cmd *c, *found;
	register int nmatches, longest;

	longest = 0;
	nmatches = 0;
	found = 0;
	for (c = cmdtab; p = c->name; c++) {
		for (q = name; *q == *p++; q++)
			if (*q == 0)		/* exact match? */
				return (c);
		if (!*q) {			/* the name was a prefix */
			if (q - name > longest) {
				longest = q - name;
				nmatches = 1;
				found = c;
			} else if (q - name == longest)
				nmatches++;
		}
	}
	if (nmatches > 1)
		return ((struct cmd *)-1);
	return (found);
}

/*
 * Slice a string up into argc/argv.
 */
void
makeargv()
{
	register char *cp;
	register char **argp = margv;

	margc = 0;
	for (cp = line; *cp;) {
		while (isspace(*cp))
			cp++;
		if (*cp == '\0')
			break;
		*argp++ = cp;
		margc += 1;
		while (*cp != '\0' && !isspace(*cp))
			cp++;
		if (*cp == '\0')
			break;
		*cp++ = '\0';
	}
	*argp++ = 0;
}

/*VARARGS*/
quit()
{
	exit(0);
	return 0; /* stop compiler warning even tho we can't get here */
}

/*
 * Help command.
 */
help(argc, argv)
	int argc;
	char *argv[];
{
	register struct cmd *c;

	if (argc == 1) {
		printf("Commands may be abbreviated.  Commands are:\n\n");
		for (c = cmdtab; c->name; c++)
			printf("%-*s\t%s\n", HELPINDENT, c->name, c->help);
		return 0;
	}
	while (--argc > 0) {
		register char *arg;
		arg = *++argv;
		c = getcmd(arg);
		if (c == (struct cmd *)-1)
			printf("?Ambiguous help command %s\n", arg);
		else if (c == (struct cmd *)0)
			printf("?Invalid help command %s\n", arg);
		else
			printf("%s\n", c->help);
	}
	return 0;
}

/*VARARGS*/
settrace()
{
	trace = !trace;
	printf("Packet tracing %s.\n", trace ? "on" : "off");
	return 0;
}

/*VARARGS*/
setverbose()
{
	verbose = !verbose;
	printf("Verbose mode %s.\n", verbose ? "on" : "off");
	return 0;
}
