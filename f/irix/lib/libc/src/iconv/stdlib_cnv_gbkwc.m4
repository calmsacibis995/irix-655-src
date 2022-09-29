/* __file__ =============================================== *
 *
 *  DBCS gbk Wchar converter.
 */

define(CZZ_TNAME(),`
define(`INTERMEDIATE_TYPE$1',int)
')

define(CZZ_CNAME(),`
define(`DECLARATIONS',`
    /* === Entering in ''__file__`` line ''__line__`` ============= */
    'defn(`DECLARATIONS')`
    /* === Following found in ''__file__`` line ''__line__`` ============= */
    'INTERMEDIATE_TYPE$1`	    state$1;
    /* === Previous found in ''__file__`` line ''__line__`` ============= */
')
define(`INITIALIZE',`
    /* === Entering in ''__file__`` line ''__line__`` ============= */
    'defn(`INITIALIZE')`
    /* === Previous found in ''__file__`` line ''__line__`` ============= */
')
define(`GET_VALUE',`
	/* === Following found in ''__file__`` line ''__line__`` ============= */
	'defn(`GET_VALUE')`
	/* === Back in ''__file__`` line ''__line__`` ============= */
	
	if ('RETURN_VAR` > 0xA0 ) {
	    /* 2 bytes eucw1 */
	    state$1 = 'RETURN_VAR`;

	    /* ============ Get next input character. =================	*/
	    /* === leaving in ''__file__`` line ''__line__`` ==========	*/
	    'defn(`GET_VALUE')`
	    /* === Back in ''__file__`` line ''__line__`` ============= */

	    /* REV could add more checks */
	    if ( 'RETURN_VAR` > 0xA0 )
		state$1 = ( ( state$1 & 0177 ) << 7 ) | ( 'RETURN_VAR` & 0177 ) | P11;
	    else if ('RETURN_VAR` < 0x40) {
		goto illegal_recovery;
	    }
	    else
		state$1 = ( ( state$1 & 0xFF ) << 8 ) | 'RETURN_VAR` | P10 ;
	    goto cont$1;

	} else if ( 0x81 <= 'RETURN_VAR` && 'RETURN_VAR` <= 0xA0 ) {
	    state$1 = 'RETURN_VAR` << 8;

	    /* === leaving in ''__file__`` line ''__line__`` ==========	*/
	    'defn(`GET_VALUE')`
	    /* === Back in ''__file__`` line ''__line__`` ============= */
	    if ( 'RETURN_VAR` < 0x40  || 'RETURN_VAR` == 0x7F || 'RETURN_VAR` > 0xFE ) {
		goto illegal_recovery;
	    }

	    state$1 |= 'RETURN_VAR` | P10;

 	} else {
	    /* single byte */
	    state$1 = 'RETURN_VAR`;
	    goto cont$1;
	}

cont$1:
    /* === Previous code found in ''__file__`` line ''__line__`` ========= */
    'define(`RETURN_VAR',state$1)`
')
')

define(CZZ_UNAME(),`
    undefine(`INTERMEDIATE_TYPE$1')
    undefine(`GET_VALUE')
')

/* End __file__ ============================================== */
