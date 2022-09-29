
#ident  "$Revision: 1.1 $"
/*******************************************************************
 The following functions are specific to diff only
*******************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include "regex.h"  
#include "diff.h"




/*********************************************************************

     Purpose : MB implementation of re_compile_pattern() function      

      Parameters : char *pattern, int size, struct re_pattern_buffer *

      Return Values : char *,Null if success else pointer to errmsg
		      functionally same as original function.

*********************************************************************/
char *
mbRe_compile_pattern (pattern, size, bufp)
     char *pattern;
     int size;
     struct re_pattern_buffer *bufp;
{
char ErrMsg[BUFSIZ];
int i;
int map=0;

	if (obscure_syntax|RE_CHAR_CLASSES)
		map|=REG_EXTENDED;
	map|=REG_NOSUB; /*  report only success or fail  */
	if (obscure_syntax|RE_NEWLINE_OR)
	for(i=0;i<size;i++)
		if(pattern[i]=='\n')
			pattern[i]='|';
	if((bufp->err=regcomp(&bufp->regstru, pattern, map))!=0) {
		regerror(bufp->err, &bufp->regstru, ErrMsg, sizeof(ErrMsg));
		return ErrMsg;
	}

    return ((char *)NULL);
}


/*********************************************************************

     Purpose : MB implementation of re_search() function       

      Parameters :

     	struct re_pattern_buffer *pbufp;
    	char *string;
     	int size, startpos, range;
     	struct re_registers *regs;

      Return Values : 0 if error, -1 if no match found, 1 if match found

*********************************************************************/

int
mbRe_search (pbufp, string, size, startpos, range, regs)  /**  size = no. of wc's less null 
							   startpos, range, regs are useless here; they're here just because
							   re_search() uses them 
							   -1 if no match found
							   +1 if match found 
							   0  if error  **/
     struct re_pattern_buffer *pbufp;
     char *string;
     int size, startpos, range;
     struct re_registers *regs;
{
char *String;
int ret=0;

	if (pbufp->err)
		return 0;
	String = (char *)xmalloc(size+1);
	memcpy(String, string, size);
	String[size]='\0';
	if((ret=regexec(&pbufp->regstru, String, (size_t)0, NULL, 0))!=0) {
		free(String);
		if (ret == REG_NOMATCH)
			return -1;
		else
			return 0;
	}
   free(String);
   return 1; /* found */
}
