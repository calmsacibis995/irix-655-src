
/* __file__ =============================================== *
 *
 *  Output converter for wchar_t to eucJP with JIS 212 support
 */

define(CZZ_TNAME(),`
define(`OUTPUT_SCALAR_TYPE','CZZ_TYPE()`)
define(`OUTPUT_TYPE','CZZ_NAME()`_outval_type)
')
CZZ_TNAME()

define(CZZ_CNAME(),`
define(`DECLARATIONS',`
    'defn(`DECLARATIONS')`
    /* === Following found in ''__file__`` line ''__line__`` ============= */
    int			codeset$1;
    int			out1$1;
    int			out2$1;
    /* === Previous found in ''__file__`` line ''__line__`` ============= */
')
define(`OUTPUT_REFERENCE',`( '$`1 )->val')
define(`OUTPUT_SETVAL',`
    'defn(`GET_VALUE')`
    /* === Following found in ''__file__`` line ''__line__`` ============= */
    if ( ( codeset$1 = (('RETURN_VAR`) >> EUCWC_SHFT)) == ((P11)>>EUCWC_SHFT)) {
	/* eucw1 2 bytes */
	'OUTPUT_WCTOB_FAIL`
	'OUTPUT_COUNT_ADJUST(2)`
	'OUTPUT_CHECK_COUNT(< 0,OUTPUT_COUNT_ADJUST(-2))`
	if ( (((('RETURN_VAR`) >> 7) | 0200) & 0377) < 0240 ) {
	    'OUTPUT_COUNT_ADJUST(-2)`
	    goto illegal_recovery;
	}
	'OUTPUT_ASSIGN( OUTPUT_PTR_VAR + 0, (((('RETURN_VAR`) >> 7) | 0200) & 0377) )`;
	'OUTPUT_ASSIGN( OUTPUT_PTR_VAR + 1, (('RETURN_VAR` | 0200) & 0377 ) )`;
	'OUTPUT_PTR_ADJUST(2)`
    } else if ( codeset$1 == ((P10)>>EUCWC_SHFT)) {
	/* eucw2 3 bytes */
	'OUTPUT_WCTOB_FAIL`
	'OUTPUT_COUNT_ADJUST(3)`
	'OUTPUT_CHECK_COUNT(< 0,OUTPUT_COUNT_ADJUST(-3))`
	'OUTPUT_PTR_ADJUST(3)`

	out2$1 =   ('RETURN_VAR`)        & 0xFF ;
	out1$1 = ( ('RETURN_VAR`) >> 8 ) & 0xFF ;

	if ( out1$1 >= 0xA1 && out2$1 == 0xA0 )
	{
	    out2$1 = out1$1;
	    out1$1 = 0xE1;
            'OUTPUT_ASSIGN( OUTPUT_PTR_VAR - 3, (SS3)  )`;
	}
	else
        if ( out1$1 >= 0xA0 && out2$1 < 0xA0 )
	{
	 	'OUTPUT_ASSIGN( OUTPUT_PTR_VAR - 3, (SS2)  )`;
		out2$1 += 0x60;
	}
	else
	{
                'OUTPUT_ASSIGN( OUTPUT_PTR_VAR - 3, (SS3)  )`;
		if ( out2$1 < 0xA0 )
		{
			out1$1 += 0x20;
			out2$1 += 0x60;
		}
		else
			out1$1 += 0x40;
	}

	'OUTPUT_ASSIGN( OUTPUT_PTR_VAR - 1, out2$1 )`;
	'OUTPUT_ASSIGN( OUTPUT_PTR_VAR - 2, out1$1 )`;

    } else {
	if ('RETURN_VAR` >= 0240) {
	    goto illegal_recovery;
	}
        'OUTPUT_CHECK_COUNT(<= 0,)`
        'OUTPUT_COUNT_ADJUST(1)`
        'OUTPUT_ASSIGN( OUTPUT_PTR_VAR, RETURN_VAR )`;
        'OUTPUT_PTR_ADJUST(1)`
    }
    /* === Previous code found in ''__file__`` line ''__line__`` ========= */
')
define(`OUTPUT_WCTOB_RETVAL',`
    'defn(`OUTPUT_SETVAL')`
    return 'RETURN_VAR`;
')
')

/* === Following found in __file__ line __line__ ============= */
ifelse(CZZ_PACK(),p,`#pragma pack(1)')
typedef struct OUTPUT_TYPE {
    OUTPUT_SCALAR_TYPE	    val;
} OUTPUT_TYPE;
ifelse(CZZ_PACK(),p,`#pragma pack(0)')
/* === Previous code found in __file__ line __line__ ========= */

define(CZZ_UNAME(),`
    undefine(`OUTPUT_SCALAR_TYPE')
    undefine(`OUTPUT_TYPE')
    undefine(`OUTPUT_SETVAL')
    undefine(`OUTPUT_WCTOB_RETVAL')
')

/* __file__ ============================================== */
