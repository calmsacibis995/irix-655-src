#include <sys/types.h>
#include <sys/debug.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/iograph.h>
#include <ksys/hwg.h>
#include <sys/invent.h>
#include <sys/mtio.h>
#include <sys/tpsc.h>
#include <sys/scsi.h>
#include <string.h>
#include <bstring.h>

/*	determine which one of the supported drives we are looking at
	from the inquiry data.  This is needed for all the little
	idiosyncrasies...  Called from wd93.c during inventory setup, and
	from ctsetup() in the tpsc driver.
	The return value is the hinv type, for scsi.
*/
int
tpsctapetype(ct_g0inq_data_t *inv, struct tpsc_types *tpp)
{
	int i;
	struct tpsc_types *tpt;
	u_char *id = inv->id_vid;
	u_char *pid = inv->id_pid;
	extern struct tpsc_types tpsc_types[];
	extern struct tpsc_types tpsc_generic;
	extern int tpsc_numtypes;

	tpt = tpsc_types;
	if(inv->id_ailen == 0) {	/* hope only 540S... */
		id = (u_char *)"CIPHER";	/* fake it; must match master.d/tpsc */
		pid = (u_char *)"540S";	/* fake it; must match master.d/tpsc */
	}
	for(i=0; i<tpsc_numtypes; i++, tpt++) {
		if(strncmp((char *)tpt->tp_vendor, (char *)id, tpt->tp_vlen) == 0 &&
		   strncmp((char *)tpt->tp_product, (char *)pid, tpt->tp_plen) == 0)
			break;
	}
	if(i == tpsc_numtypes)
		tpt = &tpsc_generic; 
	if(tpp)
		bcopy(tpt, tpp, sizeof(*tpt));
	return tpt->tp_hinv;
}

/* make an alias name of the form 
 *	tps<ctlr_num>d<targ_num>l<lun_num> given a 
 * path name corresponding to a tape device
 */
int
tp_alias_make(vertex_hdl_t tape_vhdl,char *alias,char *suffix)
{

	scsi_unit_info_t	*unit_info = scsi_unit_info_get(tape_vhdl);
	
	/* for lun 0 there should not be an "l0" in the alias name */
	if (SUI_LUN(unit_info))
		sprintf(alias , "tps%dd%dl%d%s",
			device_controller_num_get(SUI_CTLR_VHDL(unit_info)),
			SUI_TARG(unit_info),
			SUI_LUN(unit_info),
			suffix);
	else
		sprintf(alias , "tps%dd%d%s",
			device_controller_num_get(SUI_CTLR_VHDL(unit_info)),
			SUI_TARG(unit_info),
			suffix);
	
	return 1;

}

/*
 * sub_paths    - new style relative paths from the default vertex for
 *                 the each of the corresponding old style suffix 
 * *_crypt_name - old style suffixes used in aliases.
 */
typedef struct tpsc_alias_name {
	char *alias_name;
	char  hwg_subpath_index;
} tpsc_alias_name_s;

char *sub_paths[] = {
	"",				/* 0 */
	"/swap",			/* 1 */
	"/norewind/swap",		/* 2 */
	"/norewind",			/* 3 */
	"/swap/varblk",			/* 4 */
	"/norewind/swap/varblk",	/* 5 */
	"/varblk",			/* 6 */
	"/norewind/varblk"		/* 7 */
};
#define NOSWAP_REWIND_NOVAR   0
#define SWAP_REWIND_NOVAR     1
#define SWAP_NOREWIND_NOVAR   2
#define NOSWAP_NOREWIND_NOVAR 3
#define SWAP_REWIND_VAR       4
#define SWAP_NOREWIND_VAR     5
#define NOSWAP_REWIND_VAR     6
#define NOSWAP_NOREWIND_VAR   7

tpsc_alias_name_s var_crypt_name[] = {
	{ "s",		SWAP_REWIND_NOVAR },
	{ "nrs",	SWAP_NOREWIND_NOVAR },
	{ "ns",		NOSWAP_REWIND_NOVAR },
	{ "",		NOSWAP_REWIND_NOVAR },
	{ "nrns",	NOSWAP_NOREWIND_NOVAR },
	{ "nr",		NOSWAP_NOREWIND_NOVAR },
	{ "sv",		SWAP_REWIND_VAR },
	{ "nrsv",	SWAP_NOREWIND_VAR },
	{ "nsv",	NOSWAP_REWIND_VAR },
	{ "v",		NOSWAP_REWIND_VAR },
	{ "nrnsv",      NOSWAP_NOREWIND_VAR },
	{ "nrv",	NOSWAP_NOREWIND_VAR }
};
tpsc_alias_name_s nonvar_crypt_name[] = {
	{ "s",		SWAP_REWIND_NOVAR },
	{ "nrs",	SWAP_NOREWIND_NOVAR },
	{ "ns",		NOSWAP_REWIND_NOVAR },
	{ "",		NOSWAP_REWIND_NOVAR },
	{ "nrns",	NOSWAP_NOREWIND_NOVAR },
	{ "nr",		NOSWAP_NOREWIND_NOVAR },
};
tpsc_alias_name_s swap_crypt_name[] = { 
	{ "s",		SWAP_REWIND_NOVAR },
	{ "nrs",	SWAP_NOREWIND_NOVAR },
	{ "ns",		NOSWAP_REWIND_NOVAR },
	{ "",		SWAP_REWIND_NOVAR },
	{ "nrns",	NOSWAP_NOREWIND_NOVAR },
	{ "nr",		SWAP_NOREWIND_NOVAR },
};

#define VAR_CRYPT_NAME_ARR_LEN		(sizeof(var_crypt_name) / sizeof(tpsc_alias_name_s))
#define NONVAR_CRYPT_NAME_ARR_LEN	(sizeof(nonvar_crypt_name) / sizeof(tpsc_alias_name_s))
#define SWAP_CRYPT_NAME_ARR_LEN         (sizeof(swap_crypt_name) / sizeof(tpsc_alias_name_s))

#define IS_SWAP_DEV(ttp)		(((ttp)->tp_hinv == TPQIC150) || \
					 ((ttp)->tp_hinv == TPQIC24)  || \
					 ((ttp)->tp_hinv == TPQIC1000))
#define CRYPT_NAME(ttp)			(((IS_SWAP_DEV(ttp)) ? swap_crypt_name : \
					  (ttp->tp_capablity & MTCAN_VAR) ? var_crypt_name : \
					  nonvar_crypt_name))
#define MAX_OLD_TPS_SUFFIXES(ttp) 	(((IS_SWAP_DEV(ttp)) ? SWAP_CRYPT_NAME_ARR_LEN : \
					  (ttp->tp_capablity & MTCAN_VAR) ? VAR_CRYPT_NAME_ARR_LEN : \
					  NONVAR_CRYPT_NAME_ARR_LEN))

/* make all aliases of /hw/tape/tps#d#l#.... to the /hw/ctlr/#/target/#/lun/#/tape/default/...
 */
/* ARGSUSED */
void
ct_alias_make(vertex_hdl_t	tape_vhdl,		
              vertex_hdl_t 	default_mode_vhdl,
	      struct tpsc_types *ttp)
{
	char			path_name[200];
	char			alias[20];
	dev_t			hwtape;
	/* REFERENCED */
	graph_error_t		rv;
	vertex_hdl_t		to;
	int			i,j;
	struct tpsc_alias_name	*crypt_name;
	char			**dens;
	char			**crypt_dens;
	vertex_hdl_t		mode_vhdl;
	dev_t			hwscsi;
	vertex_hdl_t		scsi_vhdl;
	scsi_unit_info_t	*unit_info;	
	int			controller_number;


	if ((hwscsi = hwgraph_path_to_dev("/hw/"EDGE_LBL_SCSI)) == NODEV) {
		rv = hwgraph_path_add(hwgraph_root, EDGE_LBL_SCSI , &hwscsi);
		ASSERT(rv == GRAPH_SUCCESS);
	} 
	unit_info = scsi_unit_info_get(tape_vhdl);
	/*
	 * Get the ioconfig assigned controller number for this tape drive
	 */
	controller_number = device_controller_num_get(SUI_CTLR_VHDL(unit_info) );
	/* Form the alias to the devscsi device for this tape drive
	 * using the persistent ioconfig assigned controller number.
	 */
	sprintf(alias,"sc%dd%dl%d",
		controller_number,
		SUI_TARG(unit_info),
		SUI_LUN(unit_info));
	/* Store the persistent scsi controller information on the
	 * tape device vertex.
	 */
	device_controller_num_set(tape_vhdl,controller_number);

	rv = hwgraph_traverse(tape_vhdl,"../"EDGE_LBL_SCSI,&scsi_vhdl);
	ASSERT(rv == GRAPH_SUCCESS);
	hwgraph_vertex_unref(scsi_vhdl);
	if (hwgraph_traverse(hwscsi,alias,&scsi_vhdl) != GRAPH_SUCCESS) {
		rv = hwgraph_edge_add(hwscsi,scsi_vhdl,alias);
		ASSERT(rv == GRAPH_SUCCESS);
	} else
		hwgraph_vertex_unref(scsi_vhdl);

	if ((hwtape = hwgraph_path_to_dev("/hw/"EDGE_LBL_TAPE)) == NODEV) {
		rv = hwgraph_path_add(hwgraph_root, EDGE_LBL_TAPE , &hwtape);
		ASSERT(rv == GRAPH_SUCCESS);
	}

	vertex_to_name(tape_vhdl,path_name,200);
	strcat(path_name,"/"TPSC_STAT"/"EDGE_LBL_CHAR);
	tp_alias_make(tape_vhdl,alias,TPSC_STAT);
	if (hwgraph_traverse(hwtape,alias,&to) != GRAPH_SUCCESS) {
		rv = hwgraph_traverse(tape_vhdl,TPSC_STAT,&mode_vhdl);
		ASSERT(rv == GRAPH_SUCCESS);
		rv = hwgraph_edge_add(hwtape,mode_vhdl,alias);
		ASSERT(rv == GRAPH_SUCCESS); 
		hwgraph_vertex_unref(mode_vhdl);
	} else
		hwgraph_vertex_unref(to);

	
	crypt_name = CRYPT_NAME(ttp);
	dens = ttp->tp_hwg_dens_names;
	crypt_dens = ttp->tp_alias_dens_names;
	for ( i = 0 ; i < MAX_OLD_TPS_SUFFIXES(ttp) ; i++) {
	
		char		sub_path[100];
		char		suffix[20];
		
		if (ttp->tp_capablity & (MTCAN_COMPRESS | MTCAN_SETDEN)) {

			for (j = 0; j < ttp->tp_dens_count; j++) {

				sprintf(sub_path,"/../%s/%s/"EDGE_LBL_CHAR,
					sub_paths[crypt_name[i].hwg_subpath_index], (j ? dens[j] : ""));
				sprintf(suffix, "%s%s",
					crypt_name[i].alias_name, crypt_dens[j]);
				tp_alias_make(tape_vhdl,alias,suffix);
				rv = hwgraph_traverse(default_mode_vhdl,sub_path,
						      &mode_vhdl);
				if (rv != GRAPH_SUCCESS)
					continue;

				if (hwgraph_traverse(hwtape,alias,&to) != GRAPH_SUCCESS) {
					rv = hwgraph_edge_add(hwtape,mode_vhdl,alias);
					ASSERT(rv == GRAPH_SUCCESS);
				} else
					hwgraph_vertex_unref(to);
				hwgraph_vertex_unref(mode_vhdl);
			}
		} 
		sprintf(sub_path,"/../%s/"EDGE_LBL_CHAR, sub_paths[crypt_name[i].hwg_subpath_index]);
		sprintf(suffix,"%s",crypt_name[i].alias_name);
		tp_alias_make(tape_vhdl,alias,suffix);
		rv = hwgraph_traverse(default_mode_vhdl,sub_path,
				      &mode_vhdl);
		if (rv != GRAPH_SUCCESS)
			continue;
		if (hwgraph_traverse(hwtape,alias,&to) != GRAPH_SUCCESS) {
			rv = hwgraph_edge_add(hwtape,mode_vhdl,alias);
			ASSERT(rv == GRAPH_SUCCESS);
		} else
			hwgraph_vertex_unref(to);
		hwgraph_vertex_unref(mode_vhdl);

	}
	hwgraph_vertex_unref(hwscsi);
	hwgraph_vertex_unref(hwtape);
}
/* 
 * remove all the the tps#d#.. aliases from the /hw/tape directory
 */
void
ct_alias_remove(vertex_hdl_t	tape_vhdl) 
{
	char			alias[20];
	dev_t			hwtape;
	/* REFERENCED */
	graph_error_t		rv;
	vertex_hdl_t		to;
	int			i,j;
	struct tpsc_alias_name	*crypt_name;
	char			**crypt_dens;
	vertex_hdl_t		default_mode_vhdl;
	struct tpsc_types	*ttp;
	tpsc_local_info_t	*tli;

	if ((hwtape = hwgraph_path_to_dev("/hw/"EDGE_LBL_TAPE)) == NODEV) {
		return;
	}

	/* remove the stat alias */
	tp_alias_make(tape_vhdl,alias,TPSC_STAT);
	if (hwgraph_traverse(hwtape,alias,&to) == GRAPH_SUCCESS) {
		rv = hwgraph_edge_remove(hwtape,alias,NULL);
		ASSERT(rv == GRAPH_SUCCESS);
		hwgraph_vertex_unref(to);
	}
	rv = hwgraph_traverse(tape_vhdl,TPSC_DEFAULT"/"EDGE_LBL_CHAR,
			      &default_mode_vhdl);
	ASSERT(rv == GRAPH_SUCCESS);
	tli  = (tpsc_local_info_t *)hwgraph_fastinfo_get(default_mode_vhdl);
	ASSERT(tli);
	hwgraph_vertex_unref(default_mode_vhdl);
	ttp = tli->tli_ttp;
	crypt_name = CRYPT_NAME(ttp);
	crypt_dens = ttp->tp_alias_dens_names;
	for ( i = 0 ; i < MAX_OLD_TPS_SUFFIXES(ttp) ; i++) {
	
		char		suffix[20];
		
		if (ttp->tp_capablity & (MTCAN_COMPRESS | MTCAN_SETDEN)) {

			for (j = 0; j < ttp->tp_dens_count; j++) {

				sprintf(suffix, "%s%s",
					crypt_name[i].alias_name, crypt_dens[j]);
				tp_alias_make(tape_vhdl,alias,suffix);
				if (hwgraph_traverse(hwtape,alias,&to) == GRAPH_SUCCESS) {
					rv = hwgraph_edge_remove(hwtape,
								 alias,NULL);
					ASSERT(rv == GRAPH_SUCCESS);
					hwgraph_vertex_unref(to);
				}
			}
		} 
		sprintf(suffix,"%s",crypt_name[i].alias_name);
		tp_alias_make(tape_vhdl,alias,suffix);
		if (hwgraph_traverse(hwtape,alias,&to) == GRAPH_SUCCESS) {
			rv = hwgraph_edge_remove(hwtape,alias,NULL);
			ASSERT(rv == GRAPH_SUCCESS);
			hwgraph_vertex_unref(to);
		}
	}
	hwgraph_vertex_unref(hwtape);
}

