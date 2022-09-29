/* Prototypes for HIPPI-LE layer hooks */
extern void ifhip_attach( hippi_vars_t * );
extern void ifhip_le_odone( hippi_vars_t *, 
			    volatile struct hip_d2b_hd *, int );
extern int  ifhip_fillin( hippi_vars_t * );
extern void ifhip_le_input( hippi_vars_t *, volatile struct hip_b2h * );
extern void ifhip_shutdown( hippi_vars_t * );
