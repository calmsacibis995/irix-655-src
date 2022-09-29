#ifndef __DNS_LAMED_H_
#define __DNS_LAMED_H_

typedef struct servers {
	u_int32_t	addr;
	struct servers	*next;
} server_t;

typedef struct domains {
	struct domains	*next;
	int		len;
	char		name[1];
} domain_t;

struct addrlist {
	struct in_addr addr;
	struct addrlist *next;
};

typedef struct sortlist {
	struct in_addr	addr;
	struct in_addr	netmask;
	struct sortlist *next;
	struct addrlist *sublist;
} sort_t;

typedef struct {
	unsigned	flags;
	int		max_tries;
	int		timeout;
	uint16_t	xid;
	int		retries;
	char		*map;
	u_int32_t	type;
	u_int32_t	class;
	server_t	*servers;
	domain_t	*domains;
	char		key[MAXDNAME];
} dnsrequest_t;

#define DNS_PARALLEL	1
#define DNS_BUFSIZ	1024

extern int init(char *);
extern domain_t *dns_domain_parse(char *);
extern domain_t *dns_search_parse(char *);
extern server_t *dns_servers_parse(char *);
extern int dns_config(void);
extern struct addrlist *dns_insortlist(struct addrlist *);

#endif
