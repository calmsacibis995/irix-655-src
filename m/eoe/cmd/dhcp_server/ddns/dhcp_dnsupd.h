/*
 * dhcp_dnsupd.h - dhcp dynamic updates
 */
#ifndef _dhcp_dnsupd_h
#define _dhcp_dnsupd_h

#define DHCP_DNSUPD_OK 0
#define DHCP_DNSUPD_ERR 1


extern void addr_inaddrarpa(char *addr);
extern int dhcp_dnsadd(int, u_long ipaddr, char *hname,
		       char* domain, u_long lease, char *fqdn);
extern int dhcp_dnsdel(u_long addr);
#endif /* _dhcp_dnsupd_h */
