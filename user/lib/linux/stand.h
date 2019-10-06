//https://raw.githubusercontent.com/lattera/freebsd/master/lib/libstand/stand.h
#include <errno.h>
#include <sys/types.h>
struct open_file {
    int			f_flags;	/* see F_* below */
    struct devsw	*f_dev;		/* pointer to device operations */
    void		*f_devdata;	/* device specific data */
    struct fs_ops	*f_ops;		/* pointer to file system operations */
    void		*f_fsdata;	/* file system specific data */
    off_t		f_offset;	/* current file offset */
    char		*f_rabuf;	/* readahead buffer pointer */
    size_t		f_ralen;	/* valid data in readahead buffer */
    off_t		f_raoffset;	/* consumer offset in readahead buffer */
#define SOPEN_RASIZE	512
};


extern struct open_file files[];
#define	SOPEN_MAX	64

/* f_flags values */
#define	F_READ		0x0001	/* file opened for reading */
#define	F_WRITE		0x0002	/* file opened for writing */
#define	F_RAW		0x0004	/* raw device open - no file system */
#define F_NODEV		0x0008	/* network open - no device */
