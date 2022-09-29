#ifndef __I18N_CAPABLE_H__ 
#define __I18N_CAPABLE_H__ 

#include <stdlib.h> 
#include <locale_attr.h>
#include <locale.h>

#ifndef MULTIBYTE_SUPPORT 
#define MULTIBYTE_SUPPORT 1
#endif 

#ifndef EUC_SUPPORT 
#define EUC_SUPPORT 1 
#endif 

#ifndef SB_SUPPORT 
#define SB_SUPPORT 1 
#endif 

#define eucw3 (int)(__libc_attr._csinfo._eucwidth[2])

#if MULTIBYTE_SUPPORT 
# if EUC_SUPPORT 
# define I18N_EUC_CODE (_IS_EUC_LOCALE && eucw3 ) 
# if SB_SUPPORT 
# define I18N_SBCS_CODE (MB_CUR_MAX == 1) 
# else 
# define I18N_SBCS_CODE 0 
# endif 
# else 
# define I18N_EUC_CODE 0 
# if SB_SUPPORT 
# define I18N_SBCS_CODE (MB_CUR_MAX == 1) 
# else 
# define I18N_SBCS_CODE 0 
# endif 
# endif 
#else 
# if EUC_SUPPORT 
# define I18N_EUC_CODE 1 
# if SB_SUPPORT 
# define I18N_SBCS_CODE (MB_CUR_MAX == 1) 
# else 
# define I18N_SBCS_CODE 0 
# endif 
# else 
# define I18N_EUC_CODE 0 
# if SB_SUPPORT 
# define I18N_SBCS_CODE 1 
# else 
# define I18N_EUC_CODE 1 
# define I18N_SBCS_CODE (MB_CUR_MAX == 1) 
# endif 
# endif 
#endif 

#define SETLOCALE(CATEGORY, LOCALE) \
	if(!setlocale(CATEGORY, LOCALE)) \
		perror("Setlocale "); \
	else if(!MULTIBYTE_SUPPORT && MB_CUR_MAX >1 && \
		( !_IS_EUC_LOCALE || !EUC_SUPPORT)) \
		{ \
		fprintf(stderr,"This binary is not compatible with the \
current locale settings. continuing in 'C' locale\n"); \
		setlocale(CATEGORY,"C"); \
		putenv("LANG=C");\
		}

#endif /* __I18N_CAPABLE_H__ */ 

