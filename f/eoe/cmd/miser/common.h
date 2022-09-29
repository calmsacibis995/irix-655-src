#include <ctype.h>
#include <stdio.h>
id_type_t
str_to_queue_id(char *str)
{
	id_type_t val = 0;
	int size = 0;
	char *tmp = (char *)&val;
	size = strlen(str);
	strncpy(tmp, str, size);
	for (str = tmp + size; tmp < str; tmp++)
		*tmp = tolower(*tmp);
	return val;
}
