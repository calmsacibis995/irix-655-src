#ifndef lint
static char sccsid[] = "@(#)spellout.c	4.1 12/18/82";
#endif

#include "spell.h"
#include <nl_types.h>
#include <i18n_capable.h>
#include <msgs/uxeoe.h>


main(argc, argv)
char **argv;
{
	register i, j;
	long h;
	register long *lp;
	char word[NW];
	int dflag = 0;
	int indict;
	register char *wp;
	nl_catd  catd=0;

	setlocale(LC_ALL, "");
        catd=catopen("uxeoe",0);
	
	if (argc>1 && argv[1][0]=='-' && argv[1][1]=='d') {
		dflag = 1;
		argc--;
		argv++;
	}
	if(argc<=1) {
		fprintf(stderr, CATGETS(catd, _MSG_INSUFFICIENT_ARG));
		exit(1);
	}
	if(!prime(argc,argv)) {
		fprintf(stderr, CATGETS(catd, _MSG_CANNOT_INIT_HASH));
		exit(1);
	}
	while (fgets(word, sizeof(word), stdin)) {
		indict = 1;
		for (i=0; i<NP; i++) {
			for (wp = word, h = 0, lp = pow2[i];
				(j = *wp) != '\0'; ++wp, ++lp)
				h += j * *lp;
			h %= p[i];
			if (get(h)==0) {
				indict = 0;
				break;
			}
		}
		if (dflag == indict)
			fputs(word, stdout);
	}
}
