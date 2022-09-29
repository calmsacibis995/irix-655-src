/*
 * $Id: spindle.h,v 1.16 1999/05/11 19:29:02 kenmcd Exp $
 *
 * struct exported by spindle_stats.
 * There is one struct per physical disk spindle.
 */
#include <sys/types.h>

typedef struct {
    unsigned int	s_id;		/* unique identifier for disk drive */
    int			s_inv_type;	/* hw invent type (see sys/invent.h> */
    unsigned int	s_ctlr;		/* major dev number (i.e. controller) */
    unsigned int	s_unit;		/* minor dev number (i.e. drive unit) */
    unsigned int	s_lun;		/* lun number (i.e. for raid luns) */
    char		s_dname[16];	/* name of device, i.e. dksCdU or dksCdUlL */
					/* s_dname[] max widths ... */
					/* 00..02  dks */
					/* 03..06  4-digit CCCC */
					/* 07      d */
					/* 08..10  3-digit UUU */
					/* 11      l */
					/* 12..13  2-digit LL */
					/* 14      NULL */
					/* 15      spare */
    int			s_wname;	/* width of name of device */
    char		*s_hwgname;	/* path to node in hwgfs */
    char		*s_sn;		/* SCSI serial num, NULL if not known */
    int			s_drivenumber; 	/* index in array of spindles */
    __psunsigned_t	s_iotime_addr; 	/* kmem offset of struct iotime */
    unsigned int	s_ops;		/* I/O count (reads + writes) */
    unsigned int	s_wops;		/* number of writes */
    unsigned int	s_bops;		/* block I/O count (reads + writes) */
    unsigned int	s_wbops;	/* number of blocks written */
    unsigned int	s_act;		/* active time (ticks) */
    unsigned int	s_resp;		/* response time (ticks) */
#if !BEFORE_IRIX6_5
    unsigned int	s_qlen;		/* instantaneous queue length */
    unsigned int	s_sum_qlen;	/* summed queue length on each request completion */
#endif
} spindle;


typedef struct {
    unsigned int	c_id;		/* unique identifier for controller */
    int			c_inv_type;	/* hw invent type (see sys/invent.h> */
    unsigned int	c_ctlr;		/* major dev number */
    char		c_dname[16];	/* name of controller, i.e. dksC */
    int			c_wname;	/* width of name of device */
    int			c_ndrives;	/* no. of attached active disk drives */
    int			*c_drives;	/* indicies of attached drives */
    unsigned long long	c_ops;		/* I/O count (reads + writes) */
    unsigned long long	c_wops;		/* number of writes */
    unsigned long long	c_bops;		/* block I/O count (reads + writes) */
    unsigned long long	c_wbops;	/* number of blocks written */
    unsigned long long	c_act;		/* active time (ticks) */
    unsigned long long	c_resp;		/* response time (ticks) */
    unsigned long long	c_qlen;		/* instantaneous queue length */
    unsigned long long	c_sum_qlen;	/* summed queue length on each request completion */
} controller;


/*
* Returns count of spindles or controllers
* spindle_stats_init() must be called _before_ controller_stats_init()
*/
extern int spindle_stats_init(int);
extern int controller_stats_init(int);

/*
* Returns ptr to array of spindles or controllers
* alternates buffers on successive calls
*/
extern spindle *spindle_stats(int *); 
extern controller *controller_stats(int *); 
