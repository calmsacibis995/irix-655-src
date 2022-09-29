#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char	elsieid[] = "@(#)zdump.c	7.24";
#else
static char rcsid[] = "$OpenBSD: zdump.c,v 1.5 1997/01/21 04:52:45 millert Exp $";
#endif
#endif /* LIBC_SCCS and not lint */

#ident "$Id: zdump.c,v 1.1 1998/12/15 03:41:01 rwu Exp $"

/*
** This code has been made independent of the rest of the time
** conversion package to increase confidence in the verification it provides.
** You can use this code to help in verifying other implementations.
*/

#include "private.h"
#include <tzfile.h>
#include <locale.h>
#include <pfmt.h>
#include <msgs/uxsgicore.h>


#ifndef MAX_STRING_LENGTH
#define MAX_STRING_LENGTH	1024
#endif /* !defined MAX_STRING_LENGTH */

extern char **	environ;
extern char *	optarg;
extern int	optind;
extern char *	new_tzname[2];

static char *	abbr(struct tm *tmp);
static long	delta(struct tm *newp, struct tm *oldp);
static time_t	hunt(char *name, time_t  lot, time_t  hit);
static int	longest;
static void	show(char *zone, time_t t, int v);


int
main(argc, argv)
int	argc;
char *	argv[];
{
	register int		i;
	register int		c;
	register int		vflag;
	register char *		cutoff;
	register int		cutyear;
	register long		cuttime;
	time_t			now;
	time_t			t;
	time_t			newt;
	time_t			hibit;
	struct tm		tm;
	struct tm		newtm;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxsgicore");
	(void)setlabel("UX:zdump");

	INITIALIZE(cuttime);
	vflag = 0;
	cutoff = NULL;
	while ((c = getopt(argc, argv, "c:v")) == 'c' || c == 'v')
		if (c == 'v')
			vflag = 1;
		else	cutoff = optarg;
	if (c != -1 ||
		(optind == argc - 1 && strcmp(argv[optind], "=") == 0)) {
			pfmt(stderr, MM_ERROR, 
				_SGI_MMMX_zdump_usage);
			(void) exit(EXIT_FAILURE);
	}
	if (cutoff != NULL) {
		int	y;

		cutyear = atoi(cutoff);
		cuttime = 0;
		for (y = EPOCH_YEAR; y < cutyear; ++y)
			cuttime += DAYS_PER_NYEAR + isleap(y);
		cuttime *= SECS_PER_HOUR * HOURS_PER_DAY;
	}
	(void) time(&now);

	/* get length of the longest timezone string */
	longest = 0;
	for (i = optind; i < argc; ++i)
		if (strlen(argv[i]) > longest)
			longest = strlen(argv[i]);

	for (hibit = 1; (hibit << 1) != 0; hibit <<= 1)
		continue;


	/* get to end of environ[] */
	for (i = 0;  environ[i] != NULL;  ++i)
		continue;

	for (i = optind; i < argc; ++i) {
		static char     buf[MAX_STRING_LENGTH];
                register char **        saveenv;
                char *                  tzequals;
                char *                  fakeenv[2];

                tzequals = malloc(strlen(argv[i]) + 4);
                if (tzequals == NULL) {
			pfmt(stderr, MM_ERROR, 
				_SGI_MMMX_zdump_mem_alloc_err);
                        (void) exit(EXIT_FAILURE);
                }
                (void) sprintf(tzequals, "TZ=%s%s", ":", argv[i]);
                fakeenv[0] = tzequals;
                fakeenv[1] = NULL;
                saveenv = environ;
                environ = fakeenv;
                (void) tzset();

		show(argv[i], now, FALSE);

		if (!vflag) {
                	environ = saveenv;
			continue;
		}
		/*
		** Get lowest value of t.
		*/
		t = hibit;
		if (t > 0)		/* time_t is unsigned */
			t = 0;
		show(argv[i], t, TRUE);
		t += SECS_PER_HOUR * HOURS_PER_DAY;
		show(argv[i], t, TRUE);
		tm = *localtime(&t);
		(void) strncpy(buf, abbr(&tm), (sizeof buf) - 1);
		for ( ; ; ) {
			if (cutoff != NULL && t >= cuttime)
				break;
			newt = t + SECS_PER_HOUR * 12;
			if (cutoff != NULL && newt >= cuttime)
				break;
			if (newt <= t)
				break;
			newtm = *localtime(&newt);
			if (delta(&newtm, &tm) != (newt - t) ||
				newtm.tm_isdst != tm.tm_isdst ||
				strcmp(abbr(&newtm), buf) != 0) {
					newt = hunt(argv[i], t, newt);
					newtm = *localtime(&newt);
					(void) strncpy(buf, abbr(&newtm),
						(sizeof buf) - 1);
			}
			t = newt;
			tm = newtm;
		}
		/*
		** Get highest value of t.
		*/
		t = ~((time_t) 0);
		if (t < 0)		/* time_t is signed */
			t &= ~hibit;
		t -= SECS_PER_HOUR * HOURS_PER_DAY;
		show(argv[i], t, TRUE);
		t += SECS_PER_HOUR * HOURS_PER_DAY;
		show(argv[i], t, TRUE);
                environ = saveenv;
	}
	if (fflush(stdout) || ferror(stdout)) {
		pfmt(stderr, MM_ERROR, 
			_SGI_MMMX_zdump_write_out_err,
			strerror(errno));
		(void) exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);

	/* gcc -Wall pacifier */
	for ( ; ; )
		continue;
}

static time_t
hunt(char *name, time_t lot, time_t hit)
{
	time_t		t;
	struct tm	lotm;
	struct tm	tm;
	static char	loab[MAX_STRING_LENGTH];

	lotm = *localtime(&lot);
	(void) strncpy(loab, abbr(&lotm), (sizeof loab) - 1);
	loab[(sizeof loab) - 1] = '\0';
	while ((hit - lot) >= 2) {
		t = lot / 2 + hit / 2;
		if (t <= lot)
			++t;
		else if (t >= hit)
			--t;
		tm = *localtime(&t);
		if (delta(&tm, &lotm) == (t - lot) &&
			tm.tm_isdst == lotm.tm_isdst &&
			strcmp(abbr(&tm), loab) == 0) {
				lot = t;
				lotm = tm;
		} else	hit = t;
	}
	show(name, lot, TRUE);
	show(name, hit, TRUE);
	return hit;
}

/*
** Thanks to Paul Eggert (eggert@twinsun.com) for logic used in delta.
*/

static long
delta(struct tm * newp, struct tm * oldp)
{
	long	result;
	int	tmy;

	if (newp->tm_year < oldp->tm_year)
		return -delta(oldp, newp);
	result = 0;
	for (tmy = oldp->tm_year; tmy < newp->tm_year; ++tmy)
		result += DAYS_PER_NYEAR + isleap(tmy + TM_YEAR_BASE);
	result += newp->tm_yday - oldp->tm_yday;
	result *= HOURS_PER_DAY;
	result += newp->tm_hour - oldp->tm_hour;
	result *= MINS_PER_HOUR;
	result += newp->tm_min - oldp->tm_min;
	result *= SECS_PER_MIN;
	result += newp->tm_sec - oldp->tm_sec;
	return result;
}

static void
show(char * zone, time_t t, int v)
{
	struct tm *	tmp;

	(void) printf("%-*s  ", longest, zone);
	if (v)
		(void) printf("%.24s GMT = ", asctime(gmtime(&t)));
	tmp = localtime(&t);
	(void) printf("%.24s", asctime(tmp));
	if (*abbr(tmp) != '\0')
		(void) printf(" %s", abbr(tmp));
	if (v) {
		(void) printf(" isdst=%d", tmp->tm_isdst);
#ifdef TM_GMTOFF
		(void) printf(" gmtoff=%ld", tmp->TM_GMTOFF);
#endif /* defined TM_GMTOFF */
	}
	(void) printf("\n");
}

static char *
abbr(struct tm *tmp)
{
	register char *	result;
	static char	nada;

	if (tmp->tm_isdst != 0 && tmp->tm_isdst != 1)
		return &nada;
	result = tzname[tmp->tm_isdst];
	return (result == NULL) ? &nada : result;
}
