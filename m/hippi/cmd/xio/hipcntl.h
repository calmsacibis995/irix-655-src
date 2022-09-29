/*
 * hipbpcntl.h
 * 
 *
 */

/* This is the maximum number of HIPPI devices allowed on a machine. */
#define MAX_HIP_DEVS		32

/* Index of "Nb of bytes sent" location in status msg*/
#define HIPSRC_NB_BYTE_IDX      14      /* Index of the SRC hi byte count */
#define HIPDST_NB_BYTE_IDX      30      /* Index of the DST hi byte count */


/* Text strings used by the elf2hipfw to generate the intermediate
 * firmware include files included in hipcntl.
 */
#define DST_FWVERS_STR "hippi_dstvers"
#define SRC_FWVERS_STR "hippi_srcvers"

#define DST_FW_STR    "hippi_dst"
#define SRC_FW_STR    "hippi_src"
#define DST_PROM_STR  "lincprom_dst"
#define SRC_PROM_STR  "lincprom_src"

#define DST_FW_FILE   "hippi_dst.firm"
#define SRC_FW_FILE   "hippi_src.firm"
#define DST_PROM_FILE "lincprom_dst.firm"
#define SRC_PROM_FILE "lincprom_src.firm"

#define HIP_PROM_BASE         0xbfc00000
#define HIP_PROM_SECTOR_SIZE  0x4000          /* 16K */
#define HIP_PROM_NSECTORS             8
#define HIP_PROM_SIZE         0x20000         /* 128K */

#ifdef HIP_DEBUG
extern int hip_debug;
#define dprintf(lvl, x)	if (hip_debug>=lvl) { printf x; }
#else
#define dprintf(lvl, x)
#endif	/* HIP_DEBUG */

void	hipstartup( int, int, char ** );
void	hipshutdown( int, int, char ** );
void	hipdownload( int, int, char ** );
void	hipaccept( int, int, char ** );
void	hipreject( int, int, char ** );
void	hipstatus( int, int, char ** );
void	hipstimeo( int, int, char ** );
void	hipgetmac( int, int, char ** );
void	hipsetmac( int, int, char ** );
void	hipgetversions( int, int, char ** );
void	hiploopback( int, int, char ** );

void	hippbp_setconfig(int, struct hip_bp_config *);
void    hipbpulp( int, int, char ** );
void    hipbpjobs( int, int, char ** );
void    hipbpports( int, int, char ** );
void    hipbpspages( int, int, char ** );
void    hipbpdpages( int, int, char ** );
void    hipbpstatus( int, int, char ** );

int	is_bp_cmd(char *);
int	is_hip_cmd(char *);

struct cmd {
	char	*name;
	void	(*func)( int, int, char ** );
};




