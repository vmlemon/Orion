#include <sys/types.h>
#include <sys/_types.h>

#define	MNT_RDONLY	0x00000001	/* read only filesystem */
#define MNT_EXRDONLY 0x00000080

/*
 * Export arguments for local filesystem mount calls.
 */
#define	MAXSECFLAVORS	5
struct export_args {
	int	ex_flags;		/* export related flags */
	uid_t	ex_root;		/* mapping for root uid */
	struct	xucred ex_anon;		/* mapping for anonymous user */
	struct	sockaddr *ex_addr;	/* net address to which exported */
	u_char	ex_addrlen;		/* and the net address length */
	struct	sockaddr *ex_mask;	/* mask of valid bits in saddr */
	u_char	ex_masklen;		/* and the smask length */
	char	*ex_indexfile;		/* index file for WebNFS URLs */
	int	ex_numsecflavors;	/* security flavor count */
	int	ex_secflavors[MAXSECFLAVORS]; /* list of security flavors */
};
