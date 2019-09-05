/* The Linux Standard Base definition is at 
 * https://refspecs.linuxfoundation.org/LSB_1.3.0/gLSB/gLSB/baselib-statfs-2.html
 */

/* Technically, this API/ABI is considered deprecated, but certain Linux binaries 
 * still use it
 */

/* ls uses:
 * statfs("/sys/fs/selinux", {f_type=SELINUX_MAGIC, f_bsize=65536, f_blocks=0, f_bfree=0, f_bavail=0, f_files=0, f_ffree=0, f_fsid={val=[0, 0]}, f_namelen=255, f_frsize=65536, f_flags=ST_VALID|ST_RELATIME}) = 0
 */

//fsid_t

int statfs(const char *path, (struct statfs *buf));

  struct statfs {
             long    f_type;      /* type of filesystem (see below) */
             long    f_bsize;     /* optimal transfer block size */
             long    f_blocks;    /* total data blocks in file system */
             long    f_bfree;     /* free blocks in fs */
             long    f_bavail;    /* free blocks avail to non-superuser */
             long    f_files;     /* total file nodes in file system */
             long    f_ffree;     /* free file nodes in fs */
             long    f_ffree;     /* free file nodes in fs */
             fsid_t  f_fsid;      /* file system id */
             long    f_namelen;   /* maximum length of filenames */
             long    f_spare[6];  /* spare for later */
  };

//On success, 0 is returned. On error, -1 is returned and the global variable errno is set appropriately.

//ENOTDIR
//ENAMETOOLONG
//ENOENT
//EACCES
//ELOOP
//EFAULT
//EIO
//ENOMEM
//ENOSYS
