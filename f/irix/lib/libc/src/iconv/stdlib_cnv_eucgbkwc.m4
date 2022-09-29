/* __file__ =============================================== *
 *
 *  eucJP Wchar converter with JIS 212 support
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
    'INTERMEDIATE_TYPE$1`	    correction$1;
    /* === Previous found in ''__file__`` line ''__line__`` ============= */
')
define(`INITIALIZE',`
    /* === Entering in ''__file__`` line ''__line__`` ============= */
    'defn(`INITIALIZE')`
    correction$1 = 0;
    /* === Previous found in ''__file__`` line ''__line__`` ============= */
')
define(`GET_VALUE',`
	/* === Following found in ''__file__`` line ''__line__`` ============= */
	'defn(`GET_VALUE')`
	/* === Back in ''__file__`` line ''__line__`` ============= */
	
	if ('RETURN_VAR` == SS2) {
		/* 3 bytes eucw2 */
		/* ============ Get next input character. =============	*/

		/* === leaving in ''__file__`` line ''__line__`` ======	*/
		 'defn(`GET_VALUE')`
		/* === Back in ''__file__`` line ''__line__`` =========	*/

		if ('RETURN_VAR` < 0200) goto illegal_recovery;
		state$1 = ('RETURN_VAR` & 0xFF) << 8;

		/* === leaving in ''__file__`` line ''__line__`` ======	*/
		 'defn(`GET_VALUE')`
		/* === Back in ''__file__`` line ''__line__`` =========	*/

		if ('RETURN_VAR` < 0200) goto illegal_recovery;
		state$1 |= ( ('RETURN_VAR` & 0xFF) - 0x60 );
		state$1 |= P10;

		goto cont$1;
	} else if ('RETURN_VAR` == SS3 ) {
		/* 3 byte euc */

		/* === leaving in ''__file__`` line ''__line__`` ======	*/
                'defn(`GET_VALUE')`
		/* === Back in ''__file__`` line ''__line__`` =========	*/

                if ('RETURN_VAR` < 0200)
			goto illegal_recovery;

		if ('RETURN_VAR` == 0xE1 )
		{
			'defn(`GET_VALUE')`
			state$1 = ( ( 'RETURN_VAR` & 0xFF ) << 8 ) | 0xA0;
		}
		else
		{
	                if ('RETURN_VAR` > 0xC0)
				correction$1 = 0x4000;
			else
				correction$1 = 0x2060;

			state$1 = ('RETURN_VAR` & 0xFF) << 8;

			/* ============ Get next input character. =============	*/
	                'defn(`GET_VALUE')`
			/* === Back in ''__file__`` line ''__line__`` =========	*/

			if ('RETURN_VAR` < 0200) goto illegal_recovery;

		        state$1 += ( 'RETURN_VAR` & 0xFF ) - correction$1;
		}
		state$1 |= P10;

		goto cont$1;

	} else if ('RETURN_VAR` >= 0240) {
		/* 2 bytes eucw1 */
		state$1 = ('RETURN_VAR` & 0177) << 7;
		/* ============ Get next input character. =============	*/

		/* === leaving in ''__file__`` line ''__line__`` ======	*/
                'defn(`GET_VALUE')`
		/* === Back in ''__file__`` line ''__line__`` =========	*/
                if ('RETURN_VAR` < 0200) goto illegal_recovery;

                state$1 |= ( 'RETURN_VAR` & 0177 ) | P11;
		goto cont$1;
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
