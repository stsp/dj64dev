/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/uio.h>

ssize_t
writev (int fd, const struct iovec *iov, int iovcnt)
{
  int     old_errno = errno;
  size_t  maxbytes;
  ssize_t nbytes = 0;
  ssize_t ret;
  int     i;

  /* Check args */
  if ((iovcnt <= 0) || (iovcnt > IOV_MAX)) {
    errno = EINVAL;
    return(-1);
  }

  /* Calculate the total number of bytes needed to write data. */
  for (maxbytes = 0, i = 0; i < iovcnt; i++) {
    maxbytes += iov[i].iov_len;
    if (maxbytes > SSIZE_MAX)
      break;
  }

  /* If we write the maximum number of bytes, we may overflow the return
   * value. Return an error if this is the case. */
  if (maxbytes > SSIZE_MAX) {
    errno = EINVAL;
    return(-1);
  }

  /* Write out the data vector-by-vector. */
  for (i = 0; i < iovcnt; i++) {
    ret = write(fd, iov[i].iov_base, iov[i].iov_len);

    if (ret < 0) {
      if (!i) {
	/* Fail of the first one. Fail and pass through errno. */
	return(-1);
      } else {
	/* Hide the error and return what we've written so far. */
	errno = old_errno;
	return(nbytes);
      }
    }

    nbytes += ret;

    /* If we didn't write out a whole vector, stop. */
    if ((size_t) ret < iov[i].iov_len)
      break;
  }

  return(nbytes);
}
