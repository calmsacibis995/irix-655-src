#ifndef	_XFS_EXTFREE_ITEM_H
#define	_XFS_EXTFREE_ITEM_H

#ident	"$Revision: 1.9 $"

struct xfs_mount;
struct zone;

typedef struct xfs_extent {
	xfs_dfsbno_t	ext_start;
	xfs_extlen_t	ext_len;
} xfs_extent_t;

/*
 * This is the structure used to lay out an efi log item in the
 * log.  The efi_extents field is a variable size array whose
 * size is given by efi_nextents.
 */
typedef struct xfs_efi_log_format {
	unsigned short		efi_type;	/* efi log item type */
	unsigned short		efi_size;	/* size of this item */
	uint			efi_nextents;	/* # extents to free */
	__uint64_t		efi_id;		/* efi identifier */
	xfs_extent_t		efi_extents[1];	/* array of extents to free */
} xfs_efi_log_format_t;

/*
 * Max number of extents in fast allocation path.
 */
#define	XFS_EFI_MAX_FAST_EXTENTS	16

/*
 * Define EFI flags.
 */
#define	XFS_EFI_RECOVERED	0x1
#define	XFS_EFI_COMMITTED	0x2
#define	XFS_EFI_CANCELED	0x4

/*
 * This is the "extent free intention" log item.  It is used
 * to log the fact that some extents need to be free.  It is
 * used in conjunction with the "extent free done" log item
 * described below.
 */
typedef struct xfs_efi_log_item {
	xfs_log_item_t		efi_item;
	uint			efi_flags;	/* misc flags */
	uint			efi_next_extent;
	xfs_efi_log_format_t	efi_format;
} xfs_efi_log_item_t;

/*
 * This is the structure used to lay out an efd log item in the
 * log.  The efd_extents array is a variable size array whose
 * size is given by efd_nextents;
 */
typedef struct xfs_efd_log_format {
	unsigned short		efd_type;	/* efd log item type */
	unsigned short		efd_size;	/* size of this item */
	uint			efd_nextents;	/* # of extents freed */
	__uint64_t		efd_efi_id;	/* id of corresponding efi */
	xfs_extent_t		efd_extents[1];	/* array of extents freed */
} xfs_efd_log_format_t;

/*
 * This is the "extent free done" log item.  It is used to log
 * the fact that some extents earlier mentioned in an efi item
 * have been freed.
 */
typedef struct xfs_efd_log_item {
	xfs_log_item_t		efd_item;
	xfs_efi_log_item_t	*efd_efip;
	uint			efd_next_extent;
	xfs_efd_log_format_t	efd_format;
} xfs_efd_log_item_t;

/*
 * Max number of extents in fast allocation path.
 */
#define	XFS_EFD_MAX_FAST_EXTENTS	16

extern struct zone	*xfs_efi_zone;
extern struct zone	*xfs_efd_zone;

xfs_efi_log_item_t	*xfs_efi_init(struct xfs_mount *, uint);
xfs_efd_log_item_t	*xfs_efd_init(struct xfs_mount *, xfs_efi_log_item_t *,
				      uint);

#endif	/* _XFS_EXTFREE_ITEM_H */
