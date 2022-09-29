#ifndef lint
static char sccsid[] = "@(#)spellin.c	4.1 12/18/82";
#endif

#include "spell.h"
#include <nl_types.h>
#include <i18n_capable.h>
#include <msgs/uxeoe.h>

/* add entries to hash table for use by spell
   preexisting hash table is first argument
   words to be added are standard input
   if no hash table is given, create one from scratch
*/

main(argc,argv)
char **argv;
{
	register i, j;
	long h;
	register long *lp;
	char word[NW];
	register char *wp;
	nl_catd  catd=0;

	setlocale(LC_ALL, "");
        catd=catopen("uxeoe",0);

	if(!prime(argc,argv)) {
		fprintf(stderr,
			CATGETS(catd, _MSG_CANNOT_INIT_HASH));
		exit(1);
	}
	while (fgets(word, sizeof(word), stdin)) {
		for (i=0; i<NP; i++) {
			for (wp = word, h = 0, lp = pow2[i];
				 (j = *wp) != '\0'; ++wp, ++lp)
				h += j * *lp;
			h %= p[i];
			set(h);
		}
	}
#ifdef gcos
	freopen((char *)NULL, "wi", stdout);
#endif
	if (fwrite((char *)tab, sizeof(*tab), TABSIZE, stdout) != TABSIZE) {
		fprintf(stderr,
			CATGETS(catd, _MSG_TROUBLE_WRITE_HASH));
		exit(1);
	}
	return(0);
}
