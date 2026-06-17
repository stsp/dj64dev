/* Copyright (C) 2011 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */

/*
 * fse_zero.c - An implementation of /dev/zero and /dev/full for DJGPP
 */

#include <libc/stubs.h>
#include <libc/bss.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <fcntl.h>
#include <io.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/fsext.h>
#include <sys/xdevices.h>
#include <sys/cdefs.h>

static const char DEV_ZERO_PATH[] = "/dev/zero";
static const char DEV_FULL_PATH[] = "/dev/full";

typedef struct {
  int flags;
  unsigned int dev_full:1; /* 1 => /dev/full, 0 => /dev/zero */
  int dup_count;  /* Support for dup() - reference count. */
} DEV_DATA;

static int    dev_fsext_installed = 0;
static int    dev_zero_installed  = 0;
static int    dev_full_installed  = 0;

/* stat(), fstat() support */
static time_t dev_zero_atime;
static time_t dev_zero_ctime;
static ino_t  dev_zero_inode = 0;
static time_t dev_zero_mtime;

static time_t dev_full_atime;
static time_t dev_full_ctime;
static ino_t  dev_full_inode = 0;
static time_t dev_full_mtime;

extern ino_t _invent_inode (const char *name,
			    unsigned time_stamp,
			    unsigned long fsize);

static int dev_fsext (__FSEXT_Fnumber n, int *rv, va_list args);

/* ----------------
 * - internal_dup -
 * ---------------- */

static int
internal_dup (const int fd)
{
  DEV_DATA *data   = NULL;
  int       new_fd = 0;

  /* Allocate a new file descriptor. */
  new_fd = __FSEXT_alloc_fd(dev_fsext);
  if (new_fd < 0)
    return(-1);

  /* Get context */
  data = (DEV_DATA *) __FSEXT_get_data(fd);
  if (data == NULL) {
    errno = EBADF;
    _close(new_fd);
    return(-1);
  }

  /* Associate the context with the new fd too. */
  if (__FSEXT_set_data(new_fd, (void *) data) == NULL) {
    errno = ENOMEM;
    _close(new_fd);
    return(-1);
  }

  data->dup_count++;
  return(new_fd);
}

/* ------------------------
 * - internal_stat_simple -
 * ------------------------ */

/* This sets up the "obvious" fields of 'struct stat'. */

static int inline
internal_stat_simple (struct stat *sbuf)
{
  sbuf->st_dev     = -1;
  sbuf->st_gid     = getgid();
  /* Everyone can read & write the zero device; it's a character device. */
  sbuf->st_mode    = S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH|S_IFCHR;
  sbuf->st_nlink   = 1;
  sbuf->st_size    = 0;
  sbuf->st_blksize = 0;
  sbuf->st_uid     = getuid();

#ifdef HAVE_ST_RDEV
  sbuf->st_rdev    = -1;
#endif

  return(1);
}

/* --------------------------
 * - dev_zero_internal_stat -
 * -------------------------- */

static int
dev_zero_internal_stat (struct stat *sbuf)
{
  internal_stat_simple(sbuf);

  sbuf->st_atime = dev_zero_atime;
  sbuf->st_ctime = dev_zero_ctime;
  sbuf->st_ino   = dev_zero_inode;
  sbuf->st_mtime = dev_zero_mtime;

  return(1);
}

/* --------------------------
 * - dev_full_internal_stat -
 * -------------------------- */

/* This sets up the "obvious" fields of 'struct stat' for full device. */

static int
dev_full_internal_stat (struct stat *sbuf)
{
  internal_stat_simple(sbuf);

  sbuf->st_atime = dev_full_atime;
  sbuf->st_ctime = dev_full_ctime;
  sbuf->st_ino   = dev_full_inode;
  sbuf->st_mtime = dev_full_mtime;

  return(1);
}

/* ------------------
 * - match_dev_path -
 * ------------------ */

static int inline
match_dev_path (const char *filename, const char *dev_path)
{
  int ret = 1; /* Successful match by default */

  if (filename[0] && (filename[1] == ':')) {
    /* Drive-extended */
    if (stricmp(filename + 2, dev_path) != 0) {
      ret = 0;
    }
  } else {
    /* Normal */
    if (stricmp(filename, dev_path) != 0) {
      ret =0;
    }
  }

  return(ret);
}

/* -------------
 * - dev_fsext -
 * ------------- */

static int
dev_fsext (__FSEXT_Fnumber n, int *rv, va_list args)
{
  int          emul                            = 0; /* Emulated call? 1 => yes, 0 = no. */
  int          fd                              = 0;
  DEV_DATA    *data                            = NULL;
  char        *filename                        = NULL;
  char        *new_filename                    = NULL;
  int          open_mode                       = 0;
  int          perm     _ATTRIBUTE(__unused__) = 0;
  mode_t       mode     _ATTRIBUTE(__unused__) = 0;
  void        *buf                             = NULL;
  size_t       buflen                          = 0;
  off_t        offset   _ATTRIBUTE(__unused__) = 0;
  offset_t     lloffset _ATTRIBUTE(__unused__) = 0;
  uid_t        owner    _ATTRIBUTE(__unused__) = 0;
  gid_t        group    _ATTRIBUTE(__unused__) = 0;
  int          whence   _ATTRIBUTE(__unused__) = 0;
  struct stat *sbuf                            = NULL;
  int          cmd                             = 0;
  int          iparam                          = 0;
#ifdef DJGPP_SUPPORTS_FIONBIO_NOW
  int         *piparam                         = NULL;
#endif /* DJGPP_SUPPORTS_FIONBIO_NOW */

  switch(n) {
  default:
  case __FSEXT_nop:
    break;

  case __FSEXT_open:
    filename  = va_arg(args, char *);
    open_mode = va_arg(args, int);

    if (open_mode & O_CREAT)
      perm = va_arg(args, int);

    /* Check the filename */
    if (   !match_dev_path(filename, DEV_ZERO_PATH)
	&& !match_dev_path(filename, DEV_FULL_PATH))
      break;

    /* Check whether the zero/full device has been installed. */
    if (match_dev_path(filename, DEV_ZERO_PATH) && !dev_zero_installed)
      break;

    if (match_dev_path(filename, DEV_FULL_PATH) && !dev_full_installed)
      break;

    /* It's for us. */
    emul = 1;

    /* zero/full device _always_ exists, if installed. */
    if (open_mode & O_CREAT) {
      errno = EEXIST;
      *rv   = -1;
      break;
    }

    /* Allocate a file descriptor for the device. */
    fd = __FSEXT_alloc_fd(dev_fsext);
    if (fd < 0) {
      *rv = fd;
      break;
    }

    /* Allocate some fd context. */
    data = (DEV_DATA *) malloc(sizeof(*data));
    if (data == NULL) {
      errno = ENOMEM;
      *rv   = -1;
      break;
    }

    /* Set up the context. */
    memset(data, 0, sizeof(*data));

    data->flags     = open_mode; /* Save open mode for read(), write() */
    data->dup_count = 1;

    /* Is this zero or full device? */
    if (match_dev_path(filename, DEV_FULL_PATH))
      data->dev_full = 1;
    else
      data->dev_full = 0;

    /* zero device always has O_NOINHERIT. */
    data->flags |= O_NOINHERIT;

    /* Ensure that we only have relevant flags. */
    data->flags &= (O_ACCMODE | O_NOINHERIT | O_NONBLOCK);

    /* Associate the context with the fd. */
    if (__FSEXT_set_data(fd, (void *) data) == NULL) {
      errno = ENOMEM;
      *rv   = -1;
      break;
    }

    /* Done */
    *rv = fd;
    break;

  case __FSEXT_creat:
    filename = va_arg(args, char *);
    mode     = va_arg(args, mode_t);

    /* Check the filename */
    if (   !match_dev_path(filename, DEV_ZERO_PATH)
	&& !match_dev_path(filename, DEV_FULL_PATH))
      break;

    /* Check whether the zero/full device has been installed. */
    if (match_dev_path(filename, DEV_ZERO_PATH) && !dev_zero_installed)
      break;

    if (match_dev_path(filename, DEV_FULL_PATH) && !dev_full_installed)
      break;

    /* zero device _always_ exists. */
    emul  = 1;
    errno = EEXIST;
    *rv   = -1;
    break;

  case __FSEXT_read:
    fd     = va_arg(args, int);
    buf    = va_arg(args, void *);
    buflen = va_arg(args, size_t);

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* Get context */
    data = (DEV_DATA *) __FSEXT_get_data(fd);
    if (data == NULL) {
      errno = EBADF;
      *rv   = -1;
      break;
    }

    /* Can we actually read from the zero device? */
    if (   ((data->flags & O_ACCMODE) != O_RDONLY)
	&& ((data->flags & O_ACCMODE) != O_RDWR) ) {
      errno = EACCES;
      *rv   = -1;
      break;
    }

    /* Now read - just zero the buffer. */
    memset(buf, '\0', buflen);

    /* Update access time */
    if (data->dev_full)
      time(&dev_full_atime);
    else
      time(&dev_zero_atime);

    *rv = (int) buflen;
    break;

  case __FSEXT_write:
    fd     = va_arg(args, int);
    buf    = va_arg(args, void *);
    buflen = va_arg(args, size_t);

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* Get context */
    data = (DEV_DATA *) __FSEXT_get_data(fd);
    if (data == NULL) {
      errno = EBADF;
      *rv   = -1;
      break;
    }

    /* Can we actually write to the zero device? */
    if (   ((data->flags & O_ACCMODE) != O_WRONLY)
	&& ((data->flags & O_ACCMODE) != O_RDWR) ) {
      errno = EACCES;
      *rv   = -1;
      break;
    }

    if (data->dev_full) {
      /* Nope, it's full */
      errno = ENOSPC;
      *rv   = -1;
      break;
    }

    /* Now write - just ignore the buffer. */

    /* Update access & modification times - it must be zero device here. */
    time(&dev_zero_atime);
    time(&dev_zero_mtime);

    *rv = (int) buflen;
    break;

  case __FSEXT_ready:
    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    *rv  = __FSEXT_ready_read | __FSEXT_ready_write;
    break;

  case __FSEXT_close:
    fd = va_arg(args, int);

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* Get context */
    data = (DEV_DATA *) __FSEXT_get_data(fd);
    if (data == NULL) {
      errno = EBADF;
      *rv   = -1;
      break;
    }

    __FSEXT_set_data(fd, NULL);
    __FSEXT_set_function(fd, NULL);

    /* Cope with dup()'d zero devices. */
    data->dup_count--;

    if (data->dup_count <= 0) {
      /* No longer referenced */
      free(data);
    }

    _close(fd);
    break;

  case __FSEXT_fcntl:
    fd  = va_arg(args, int);
    cmd = va_arg(args, int);

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* Get context */
    data = (DEV_DATA *) __FSEXT_get_data(fd);
    if (data == NULL) {
      errno = EBADF;
      *rv   = -1;
      break;
    }

    switch(cmd) {
    case F_DUPFD:
      *rv = internal_dup(fd);
      break;

    case F_GETFD:
      if (data->flags & O_NOINHERIT)
	*rv = 1;
      else
	*rv = 0;
      break;

    case F_SETFD:
      iparam = va_arg(args, int);

      /* Unsupported - this can't be changed on DOS/Windows. */
      errno = ENOSYS;
      *rv   = -1;
      break;

    case F_GETFL:
      *rv = data->flags;
      break;

    case F_SETFL:
      iparam = va_arg(args, int);

      /* Trying to change immutable fields? */
      if ((iparam & ~O_NONBLOCK) != (data->flags & ~O_NONBLOCK)) {
	errno = ENOSYS;
	*rv   = -1;
	break;
      }

      /* Handle mutable fields */
      if (iparam & O_NONBLOCK)
	data->flags |= O_NONBLOCK;
      else
	data->flags &= ~O_NONBLOCK;

      *rv = 0;
      break;

    default:
      /* Unknown/unhandled fcntl */
      errno = ENOSYS;
      *rv   = -1;
      break;
    }
    break;

  case __FSEXT_ioctl:
    fd  = va_arg(args, int);
    cmd = va_arg(args, int);

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* Get context */
    data = (DEV_DATA *) __FSEXT_get_data(fd);
    if (data == NULL) {
      errno = EBADF;
      *rv   = -1;
      break;
    }

    switch(cmd) {
      /* */
#ifdef DJGPP_SUPPORTS_FIONBIO_NOW
    case FIONBIO:
      piparam = va_arg(args, int *);
      if (*piparam)
	data->flags |= O_NONBLOCK;
      else
	data->flags &= ~O_NONBLOCK;

      *rv = 0;
      break;
#endif /* DJGPP_SUPPORTS_FIONBIO_NOW */

    default:
      errno = ENOTTY;
      *rv = 1;
      break;
    }
    break;

  case __FSEXT_lseek:
    fd     = va_arg(args, int);
    offset = va_arg(args, off_t);
    whence = va_arg(args, int);

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* Get context */
    data = (DEV_DATA *) __FSEXT_get_data(fd);
    if (data == NULL) {
      errno = EBADF;
      *rv   = -1;
      break;
    }

    /* Seek is meaningless */
    *rv = 0;
    break;

  case __FSEXT_llseek:
    fd       = va_arg(args, int);
    lloffset = va_arg(args, offset_t);
    whence   = va_arg(args, int);

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* Get context */
    data = (DEV_DATA *) __FSEXT_get_data(fd);
    if (data == NULL) {
      errno = EBADF;
      *rv   = -1;
      break;
    }

    /* Seek is meaningless */
    *rv = 0;
    break;

  case __FSEXT_chmod:
    filename = va_arg(args, char *);
    mode     = va_arg(args, mode_t);

    /* Check the filename */
    if (   !match_dev_path(filename, DEV_ZERO_PATH)
	&& !match_dev_path(filename, DEV_FULL_PATH))
      break;

    /* Check whether the zero/full device has been installed. */
    if (match_dev_path(filename, DEV_ZERO_PATH) && !dev_zero_installed)
      break;

    if (match_dev_path(filename, DEV_FULL_PATH) && !dev_full_installed)
      break;

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* The zero device cannot have its mode changed. */
    errno = EPERM;
    *rv   = -1;
    break;

  case __FSEXT_fchmod:
    fd    = va_arg(args, int);
    mode  = va_arg(args, mode_t);

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* Get context */
    data = (DEV_DATA *) __FSEXT_get_data(fd);
    if (data == NULL) {
      errno = EBADF;
      *rv   = -1;
      break;
    }

    /* The zero device cannot have its mode changed. */
    errno = EPERM;
    *rv   = -1;
    break;

  case __FSEXT_chown:
    filename = va_arg(args, char *);
    owner    = va_arg(args, uid_t);
    group    = va_arg(args, gid_t);

    /* Check the filename */
    if (   !match_dev_path(filename, DEV_ZERO_PATH)
	&& !match_dev_path(filename, DEV_FULL_PATH))
      break;

    /* Check whether the zero/full device has been installed. */
    if (match_dev_path(filename, DEV_ZERO_PATH) && !dev_zero_installed)
      break;

    if (match_dev_path(filename, DEV_FULL_PATH) && !dev_full_installed)
      break;

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* Behave like chown() does on a regular file - succeed whatever
     * the uid, gid are. */
    *rv = 0;
    break;

  case __FSEXT_fchown:
    fd    = va_arg(args, int);
    owner = va_arg(args, uid_t);
    group = va_arg(args, gid_t);

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* Get context */
    data = (DEV_DATA *) __FSEXT_get_data(fd);
    if (data == NULL) {
      errno = EBADF;
      *rv   = -1;
      break;
    }

    /* Behave like fchown() does on a regular file - succeed whatever
     * the uid, gid are. */
    *rv = 0;
    break;

  case __FSEXT_link:
    filename     = va_arg(args, char *);
    new_filename = va_arg(args, char *);

    /* Check the filenames */
    if (   !match_dev_path(filename, DEV_ZERO_PATH)
	&& !match_dev_path(filename, DEV_FULL_PATH)
        && !match_dev_path(new_filename, DEV_ZERO_PATH)
	&& !match_dev_path(new_filename, DEV_FULL_PATH))
      break;

    /* Check whether the zero/full device has been installed. */
    if (match_dev_path(filename, DEV_ZERO_PATH) && !dev_zero_installed)
      break;

    if (match_dev_path(filename, DEV_FULL_PATH) && !dev_full_installed)
      break;

    if (match_dev_path(new_filename, DEV_ZERO_PATH) && !dev_zero_installed)
      break;

    if (match_dev_path(new_filename, DEV_FULL_PATH) && !dev_full_installed)
      break;

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* Fail request */
    /* It doesn't make sense to link to or from /dev/zero or /dev/full. */
    errno = EPERM;
    *rv   = -1;
    break;

  case __FSEXT_unlink:
    filename = va_arg(args, char *);

    /* Check the filename */
    if (   !match_dev_path(filename, DEV_ZERO_PATH)
	&& !match_dev_path(filename, DEV_FULL_PATH))
      break;

    /* Check whether the zero/full device has been installed. */
    if (match_dev_path(filename, DEV_ZERO_PATH) && !dev_zero_installed)
      break;

    if (match_dev_path(filename, DEV_FULL_PATH) && !dev_full_installed)
      break;

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* The zero device cannot be removed. */
    errno = EPERM;
    *rv   = -1;
    break;

  case __FSEXT_dup:
#ifdef DJGPP_SUPPORTS_FSEXT_DUP_NOW
    fd = va_arg(args, int);

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* Get context */
    data = (DEV_DATA *) __FSEXT_get_data(fd);
    if (data == NULL) {
      errno = EBADF;
      *rv   = -1;
      break;
    }

    /* Done */
    *rv = internal_dup(fd);
#endif /* DJGPP_SUPPORTS_FSEXT_DUP_NOW */
    break;

  case __FSEXT_dup2:
#ifdef DJGPP_SUPPORTS_FSEXT_DUP2_NOW
    /* TODO: When __FSEXT_dup2 is supported, add support. */
#endif /* DJGPP_SUPPORTS_FSEXT_DUP2_NOW */
    break;

  case __FSEXT_stat:
    filename = va_arg(args, char *);
    sbuf     = va_arg(args, struct stat *);

    /* Check the filename */
    if (   !match_dev_path(filename, DEV_ZERO_PATH)
	&& !match_dev_path(filename, DEV_FULL_PATH))
      break;

    /* Check whether the zero/full device has been installed. */
    if (match_dev_path(filename, DEV_ZERO_PATH) && !dev_zero_installed)
      break;

    if (match_dev_path(filename, DEV_FULL_PATH) && !dev_full_installed)
      break;

    /* It's for us. */
    emul = 1;

    /* Set up the stat buf */
    memset(sbuf, 0, sizeof(*sbuf));

    if (match_dev_path(filename, DEV_FULL_PATH))
      dev_full_internal_stat(sbuf);
    else
      dev_zero_internal_stat(sbuf);

    *rv  = 0;
    break;

  case __FSEXT_fstat:
    fd   = va_arg(args, int);
    sbuf = va_arg(args, struct stat *);

    /* This must be emulated, since the FSEXT has been called. */
    emul = 1;

    /* Get context */
    data = (DEV_DATA *) __FSEXT_get_data(fd);
    if (data == NULL) {
      errno = EBADF;
      *rv   = -1;
      break;
    }

    /* Set up the stat buf */
    memset(sbuf, 0, sizeof(*sbuf));

    if (data->dev_full)
      dev_full_internal_stat(sbuf);
    else
      dev_zero_internal_stat(sbuf);

    *rv = 0;
    break;
  }

  return(emul);
}

/* -------------
 * - check_bss -
 * ------------- */

/* Force initialisation in restarted programs, e.g. Emacs. */

static int dev_zero_bss_count = -1;

static void
check_bss_count (void)
{
  if (dev_zero_bss_count == __bss_count)
    return;

  /* Save __bss_count. */
  dev_zero_bss_count = __bss_count;

  /* Force (re-)initialisation. */
  dev_fsext_installed = dev_zero_installed = dev_full_installed = 0;
}

/* ----------------------
 * - __install_dev_zero -
 * ---------------------- */

int
__install_dev_zero (void)
{
  check_bss_count();

  if (dev_zero_installed)
    return(dev_zero_installed);

  if (!dev_fsext_installed) {
    __FSEXT_add_open_handler(dev_fsext);
    dev_fsext_installed = 1;
  }

  time(&dev_zero_ctime);
  dev_zero_atime     = dev_zero_mtime = dev_zero_ctime;
  dev_zero_inode     = _invent_inode(DEV_ZERO_PATH, 0, 0);
  dev_zero_installed = 1;

  return(1);
}

/* ----------------------
 * - __install_dev_full -
 * ---------------------- */

int
__install_dev_full (void)
{
  check_bss_count();

  if (dev_full_installed)
    return(dev_full_installed);

  if (!dev_fsext_installed) {
    __FSEXT_add_open_handler(dev_fsext);
    dev_fsext_installed = 1;
  }

  time(&dev_full_ctime);
  dev_full_atime     = dev_full_mtime = dev_full_ctime;
  dev_full_inode     = _invent_inode(DEV_FULL_PATH, 0, 0);
  dev_full_installed = 1;

  return(1);
}
