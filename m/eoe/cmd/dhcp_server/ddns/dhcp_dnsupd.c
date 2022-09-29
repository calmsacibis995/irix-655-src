/*
 * dhcp_dnsupd.c - file containing routines to update DNS from DHCP
 */
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>
#include <strings.h>
#include "dhcp_dnsupd.h"

/* taken from nameser_compat.h */
#define DELETE          ns_uop_delete
#define ADD             ns_uop_add

extern int debug;
#if (__RES > 19960801)
static struct __res_state res;	/* used in res_init call */
#endif

/* some external globals */
extern int dhcp_dnsupd_ttl;
extern int dhcp_dnsupd_secure;
extern int dhcp_dnsupd_beforeACK;
extern int dhcp_dnsupd_clientARR;

struct clfqdn {
    u_char flag;
    u_char rcode1;
    u_char rcode2;
    char  domain_name[1];
};

/*
 * convert an address in dotted decimal to inaddr.arpa notation
 */
void
addr_inaddrarpa(char *addr)
{
    int ipa[4];

    if ( (addr == (char*)0) || (*addr == '\0') )
	return;
    if (sscanf(addr, "%d.%d.%d.%d", &ipa[0], &ipa[1], &ipa[2], &ipa[3]) == 4) {
	sprintf(addr, "%d.%d.%d.%d.in-addr.arpa.", 
		ipa[3], ipa[2], ipa[1], ipa[0]);
    }
}
   
/*
 * update (add/delete) the PTR resource record
 */ 
static int dhcp_dnsupd_PTR_RR(int opcode, 
			      u_long ipaddr, char *fqdn, u_long lease)
{
    ns_updrec *rrecp_start = NULL, *rrecp, *tmprrecp;
#if (__RES > 19960801)
    ns_tsig_key *key = NULL;
    char zonename[BUFSIZ];
#endif
    int n = 0;
    int r_section, r_size, r_class, r_type, r_ttl;
    struct in_addr in;
    char inaddr_dname[BUFSIZ], *r_dname;

    r_section = S_UPDATE;
    if (ipaddr == 0) {
	syslog(LOG_ERR, "dhcp_dnsupd_PTR_RR: IP address should be not NULL");
	return DHCP_DNSUPD_ERR;
    }
    in.s_addr = ipaddr;
    r_dname = inet_ntoa(in);
    strcpy(inaddr_dname, r_dname);
    addr_inaddrarpa(inaddr_dname);
    r_dname = inaddr_dname;
    if (r_dname == (char*)0) {
	syslog(LOG_ERR, "dhcp_dnsupd_PTR_RR: IP address should be not NULL");
	return DHCP_DNSUPD_ERR;
    }

    r_class = C_IN;
    r_type = T_PTR;
    
    if ( (dhcp_dnsupd_ttl > 0) && (dhcp_dnsupd_ttl < lease) )
	r_ttl = dhcp_dnsupd_ttl;
    else
	r_ttl = (int)lease;

    r_size = strlen(fqdn);
    
    if (opcode == ADD) {	/* do we need this */
	if ( !(rrecp = res_mkupdrec(r_section, r_dname, r_class,
				    r_type, 0)) ||
	     (r_size > 0 && !(rrecp->r_data = (u_char *)malloc(r_size))) ) {
	    if (rrecp)
		res_freeupdrec(rrecp);
	    syslog(LOG_ERR, "dhcp_dnsupd_PTR_RR: Not enough memory");
	    return DHCP_DNSUPD_ERR;
	}
	rrecp->r_opcode = DELETE;
	rrecp->r_size = r_size;
	(void) strncpy((char *)rrecp->r_data, fqdn, r_size);
	rrecp_start = rrecp;
    }
    if ( !(rrecp = res_mkupdrec(r_section, r_dname, r_class,
				r_type, r_ttl)) ||
	 (r_size > 0 && !(rrecp->r_data = (u_char *)malloc(r_size))) ) {
	if (rrecp)
	    res_freeupdrec(rrecp);
	syslog(LOG_ERR, "dhcp_dnsupd_PTR_RR: Not enough memory");
	return DHCP_DNSUPD_ERR;
    }
    rrecp->r_opcode = opcode;
    rrecp->r_size = r_size;
    (void) strncpy((char *)rrecp->r_data, fqdn, r_size);
    if (rrecp_start == NULL)
	rrecp_start = rrecp;
    else
	rrecp_start->r_next = rrecp;

    if (rrecp_start) {
#if (__RES > 19960801)
	bzero(&res, sizeof(struct __res_state));
	(void) res_ninit(&res);
	if ( (key != NULL) && dhcp_dnsupd_secure) {
	    if ( (n = res_nfindprimary(&res, rrecp_start, key,
				       zonename, sizeof(zonename), &in)) < 0)
		syslog(LOG_ERR, "res_nfindprimary failed");
	    else {
		if ( (n = res_nsendupdate(&res, rrecp_start, key,
					  zonename, in))  < 0)
		    syslog(LOG_ERR, "res_nsendupdate failed");
	    }		
	}
	else {
	    if ((n = res_nupdate(&res, rrecp_start)) < 0)
		syslog(LOG_ERR, "dhcp_dnsupd_PTR_RR: failed update packet");
	}
#else
	res_init();
	_res.options &= (~RES_DEBUG);
	if ((n = res_update(rrecp_start)) < 0)
	    syslog(LOG_ERR, "dhcp_dnsupd_PTR_RR: failed update packet");
#endif

	/* free malloc'ed memory */
	while(rrecp_start) {
	    tmprrecp = rrecp_start;
	    rrecp_start = rrecp_start->r_next;
	    if (tmprrecp->r_size)
		free((char *)tmprrecp->r_data);
	    res_freeupdrec(tmprrecp);
	}
    }
    return (n);
}


/*
 * update (add/delete) the A resource record
 */ 

static int dhcp_dnsupd_A_RR(int opcode, 
			    u_long ipaddr, char *fqdn, u_long lease)
{
    ns_updrec *rrecp_start = NULL, *rrecp, *tmprrecp;
#if (__RES > 19960801)
    ns_tsig_key *key = NULL;
    char zonename[BUFSIZ];
#endif
    int n = 0;
    int r_section, r_size, r_class, r_type, r_ttl;
    struct in_addr in;
    char *r_dname, *r_data;
    char dname[BUFSIZ];

    r_section = S_UPDATE;
    if (ipaddr == 0) {
	syslog(LOG_ERR, "dhcp_dnsupd_A_RR: IP address should be not NULL");
	return DHCP_DNSUPD_ERR;
    }
    in.s_addr = ipaddr;
    r_dname = fqdn;
    if (r_dname == (char*)0) {
	syslog(LOG_ERR, "dhcp_dnsupd_A_RR: IP address should be not NULL");
	return DHCP_DNSUPD_ERR;
    }

    r_class = C_IN;
    r_type = T_A;
    r_ttl = 0;
    r_size = 0;
    
    if ( !(rrecp = res_mkupdrec(r_section, r_dname, r_class,
				r_type, r_ttl)) ||
	 (r_size > 0 && !(rrecp->r_data = (u_char *)malloc(r_size))) ) {
	if (rrecp)
	    res_freeupdrec(rrecp);
	syslog(LOG_ERR, "dhcp_dnsupd_A_RR: Not enough memory");
	return DHCP_DNSUPD_ERR;
    }

    rrecp->r_opcode = DELETE;
    rrecp->r_data = (u_char*)0;
    rrecp->r_size = r_size;
    rrecp_start = rrecp;

    if (opcode == ADD) {
	/* add the new fqdn */
	r_data = inet_ntoa(in);
	r_size = strlen(r_data);
	strncpy(dname, r_data, r_size);
	if ( (dhcp_dnsupd_ttl > 0) && (dhcp_dnsupd_ttl < lease) )
	    r_ttl = dhcp_dnsupd_ttl;
	else
	    r_ttl = (int) lease;
	if ( !(rrecp = res_mkupdrec(r_section, r_dname, r_class,
				    r_type, r_ttl)) ||
	     (r_size > 0 && !(rrecp->r_data = (u_char *)malloc(r_size))) ) {
	    if (rrecp)
		res_freeupdrec(rrecp);
	    syslog(LOG_ERR, "dhcp_dnsupd_A_RR: Not enough memory");
	    return DHCP_DNSUPD_ERR;
	}
	rrecp->r_opcode = ADD;
	rrecp->r_size = r_size;
	(void) strncpy((char *)rrecp->r_data, dname, r_size);
	rrecp_start->r_next = rrecp;
    }
    if (rrecp_start) {
#if (__RES > 19960801)
	bzero(&res, sizeof(struct __res_state));
	(void) res_ninit(&res);
	if ( (key != NULL) && dhcp_dnsupd_secure) {
	    if ( (n = res_nfindprimary(&res, rrecp_start, key,
				       zonename, sizeof(zonename), &in)) < 0)
		syslog(LOG_ERR, "res_nfindprimary failed");
	    else {
		if ( (n = res_nsendupdate(&res, rrecp_start, key,
					  zonename, in))  < 0)
		    syslog(LOG_ERR, "res_nsendupdate failed");
	    }		
	}
	else {
	    if ((n = res_nupdate(&res, rrecp_start)) < 0)
		syslog(LOG_ERR, "dhcp_dnsupd_A_RR: failed update packet");
	}
#else
	res_init();
	_res.options &= (~RES_DEBUG);
	if ((n = res_update(rrecp_start)) < 0)
	    syslog(LOG_ERR, "dhcp_dnsupd_A_RR: failed update packet");
#endif
	/* free malloc'ed memory */
	while(rrecp_start) {
	    tmprrecp = rrecp_start;
	    rrecp_start = rrecp_start->r_next;
	    if (tmprrecp->r_size)
		free((char *)tmprrecp->r_data);
	    res_freeupdrec(tmprrecp);
	}
    }
    return n;
}


/* 
 * delete DNS records on a DHCPRELEASE
 */
int dhcp_dnsdel(u_long ipa)
{
    struct hostent *he;
    char hsname[MAXDNAME];
    int n;

    he = gethostbyaddr(&ipa, sizeof(ipa), AF_INET);
    if (he) {
        strcpy(hsname, he->h_name);
	n = dhcp_dnsupd_A_RR(DELETE, ipa, hsname, 0);
	if (n < 0) 
	  return -1;
    }
    return (dhcp_dnsupd_PTR_RR(DELETE, ipa, hsname, 0));
}

/*
 * update DNS records as needed and set the client_fqdn option
 */
int dhcp_dnsadd(int when, u_long ipaddr, char *hostname, char *domain,
		u_long lease, char *client_fqdn)
{
    char cf[MAXDNAME+3];
    struct clfqdn *clfqdn_p;
    char *fqhnm;

    clfqdn_p  = (struct clfqdn*)client_fqdn;
    sprintf(cf, "%s.%s", hostname, domain);
    if (clfqdn_p->domain_name[0] != '\0') {
	if (strcmp(cf, clfqdn_p->domain_name) != 0) {
	    if (debug >= 2)
		syslog(LOG_DEBUG, "Name specified in CLIENT_FQDN (%s) option"
		       "mismatch; using %s", clfqdn_p->domain_name, cf);
	}
	strcpy(clfqdn_p->domain_name, cf);
    }
    fqhnm = cf;
    clfqdn_p->rcode1 = (u_char)0;
    clfqdn_p->rcode2 = (u_char)0;
    if ( when != dhcp_dnsupd_beforeACK) { /* do updates to DNS */
	/* called before (0) sending DHCPACK and update beforeACK = 1
	 * or when called after(1) sending DHCPACK and beforeACK = 0
	 */
	clfqdn_p->rcode1 = 
	    (u_char)dhcp_dnsupd_PTR_RR(ADD, ipaddr, fqhnm, lease);
	if ( (clfqdn_p->flag == 1) || (dhcp_dnsupd_clientARR == 1) ) {
	    clfqdn_p->rcode2 =
		(u_char)dhcp_dnsupd_A_RR(ADD, ipaddr, fqhnm, lease);
	    strcpy(clfqdn_p->domain_name, fqhnm);
	}
    }
    else if (when == 0) {	/* update AfterACK but called before the ACK */
	clfqdn_p->rcode1 = (u_char)255;
	if ( (clfqdn_p->flag == 1) || (dhcp_dnsupd_clientARR == 1) ) {
	    clfqdn_p->rcode2 = (u_char) 255;
	    strcpy(clfqdn_p->domain_name, fqhnm);
	}
    }
    if ( (when == 0) &&
	 (clfqdn_p->flag == 0) && (dhcp_dnsupd_clientARR == 1) ) 
	clfqdn_p->flag = (u_char) 3; /* forced update at server */
    return 0;
}
	
