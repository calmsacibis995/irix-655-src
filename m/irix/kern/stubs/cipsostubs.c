
#include <sys/types.h>
#include <sys/systm.h>
/*
 * CIPSO (labelled sockets with DAC) stubs.
 */

#ifdef DEBUG
#define DOPANIC(s) panic(s)
#else /* DEBUG */
#define DOPANIC(s)
#endif /* DEBUG */

void cipso_init() {}		/* Do not put a DOPANIC in this stub! */
void cipso_confignote()   	{ DOPANIC("cipso_confignote stub"); }
				
int	recvlumsg()		{ DOPANIC("recvlumsg stub"); /* NOTREACHED */ }
int 	getpsoacl()		{ DOPANIC("getpsoacl stub"); /* NOTREACHED */ }
int 	setpsoacl()		{ DOPANIC("setpsoacl stub"); /* NOTREACHED */ }
int	siocgetlabel()		{ DOPANIC("siocgetlabel stub"); /* NOTREACHED */ }
int	siocsetlabel()		{ DOPANIC("siocsetlabel stub"); /* NOTREACHED */ }
int	siocgetacl()		{ DOPANIC("siocgetacl stub"); /* NOTREACHED */ }
int	siocsetacl()		{ DOPANIC("siocsetacl stub"); /* NOTREACHED */}
int	siocgetuid()		{ DOPANIC("siocgetuid stub"); /* NOTREACHED */}
int	siocsetuid()		{ DOPANIC("siocsetuid stub"); /* NOTREACHED */}
int	siocgetrcvuid()		{ DOPANIC("siocgetrcvuid stub"); /* NOTREACHED */}
int	siocgiflabel()		{ DOPANIC("siocgiflabel stub");/* NOTREACHED */}
int	siocsiflabel()		{ DOPANIC("siocsiflabel stub");/* NOTREACHED */}
int	siocgifuid()		{ DOPANIC("siocgifuid stub"); /* NOTREACHED */}
int	siocsifuid()		{ DOPANIC("siocsifuid stub"); /* NOTREACHED */}
int 	dac_allowed()		{ DOPANIC("dac_allowed stub"); /* NOTREACHED */}
void	*ip_recvlabel()		{ DOPANIC("ip_recvlabel stub");/* NOTREACHED */}
void	*ip_xmitlabel()		{ DOPANIC("ip_xmitlabel stub");/* NOTREACHED */}
int 	soacl_ok()		{ DOPANIC("soacl_ok stub"); /* NOTREACHED */}
void 	in_pcbchameleon()	{ DOPANIC("in_pcbchameleon stub"); }
void 	set_lo_secattr()	{ DOPANIC("set_lo_secattr stub"); }

void	cipso_proc_init()	{ DOPANIC("cipso_proc_init stub"); }
void	cipso_proc_exit()	{ DOPANIC("cipso_proc_exit stub"); }
void	cipso_socket_init()	{ DOPANIC("cipso_socket_init stub"); }
void	svr4_tcl_endpt_init()	{ DOPANIC("svr4_tcl_endpt_init stub"); }
void	svr4_tco_endpt_init()	{ DOPANIC("svr4_tco_endpt_init stub"); }
void	svr4_tcoo_endpt_init()	{ DOPANIC("svr4_tcoo_endpt_init stub"); }
int	if_security_invalid()	{ DOPANIC("f_security_invalid stub");/* NOTREACHED */}


