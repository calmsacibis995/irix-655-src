/* oplocks are not included in Mini Root */

#include <sys/errno.h>

int
oplock_fcntl()
{
	return (EINVAL);
}

void
oplock_fs_create()
{
	return;
}

#ifdef CELL_CAPABLE
void
oplock_cxfs_check() { }

int
oplock_cxfs_req()
{
	return(0);
}
#endif	/* CELL_CAPABLE */
