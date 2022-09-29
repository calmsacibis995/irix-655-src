#include <sys/types.h>

/*
 * xvm stubs
 */
void *xvm_client_msg_list[] = {
	(void *)0
};

void *xvm_server_msg_list[] = {
	(void *)0
};

/* ARGSUSED */
int xvm_label_boot(char *rootname, dev_t *rootdev) { return 0; }

/* ARGSUSED */
int xvm_dev(dev_t device) { return(0); }

/* ARGSUSED */
int xvm_get_subvolumes( dev_t xvm_dev, dev_t *data, dev_t *log, dev_t *realtime, dev_t *secondary_partition) { return(0); }

void xvm_devinit(void) {}

/* ARGSUSED */
dev_t xvm_devnm_to_dev(char *name) { return NODEV; }

void xvm_icrash(void) {}
void xvm_cell_icrash(void) {}
void xvm_multicast_cell_enable(void) {}

