/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2004 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */

/* Written by Laurynas Biveinis                                      */
/* Internal source file specifying DJGPP symlink prefix and internal */
/* function which fully resolves given symlink. (Function readlink() */
/* resolves only last filename component and one symlink level.)     */

#include <libc/stubs.h>
#include <libc/dosio.h>
#include <libc/symlink.h>
#include <errno.h>
#include <go32.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "xsymlink.h"

/* How many levels of symbolic links process before thinking
 * that we've found a link loop.
 */
#define MAX_SYMLINK 8

static void advance(char ** s, char ** e);

int __solve_symlinks(const char * __symlink_path, char * __real_path)
{
   int    bytes_copied;
   char * start;
   char * end;
   int    old_errno;
   char   fn_buf[FILENAME_MAX + 1];
   char   resolved[FILENAME_MAX + 1];
   int    link_level = 0;
   int    found_absolute_path;
   int    done_something;
   char * ptr;
   int    non_trivial;

   if (!__symlink_path || !__real_path)
   {
      errno = EINVAL;
      return 0;
   }

   if (strlen(__symlink_path) > FILENAME_MAX)
   {
      errno = ENAMETOOLONG;
      return 0;
   }

   strcpy(__real_path, __symlink_path);

   /* Take a shortcut by checking if source path is a simple file or
      directory. ``Simple'' in the sense of DOS, i.e. no /dev/env etc. */
   old_errno = errno;
   bytes_copied = __internal_readlink(__symlink_path, 0, fn_buf, FILENAME_MAX);
   /* If __internal_readlink finds the path specified but it is not a symlink,
      it returns -1 and sets errno to EINVAL. */
   if ((bytes_copied == -1) && (errno == EINVAL))
   {
   	errno = old_errno;
   	return 1;
   }
   errno = old_errno;

   /* Begin by start pointing at the first character and end pointing
      at the first path separator.  In the cases like "/foo" end will
      point to the next path separator.  In all cases, if there are no
      path separators left, end will point to the end of string.
    */
   start = __real_path;
   end = strpbrk(__real_path, "/\\");
   if (start == end)
      end = strpbrk(__real_path + 1, "/\\");
   if (!end)
      end = __real_path + strlen(__real_path);
   while (start && *start)
   {
      /* Extract path component we will be resolving */
      strcpy(resolved, __real_path);
      if (*end)
         resolved[end - __real_path] = '\0';
      old_errno = errno;
      found_absolute_path = 0;
      done_something = 0;
      non_trivial = 0;
      /* Resolve that component. Repeat until we encounter non-symlink,
         or not trivial symlink (in form 'dir/file').  */
      do
      {
         bytes_copied = __internal_readlink(resolved, 0, fn_buf, FILENAME_MAX);
         if (bytes_copied != -1)
         {
            done_something = 1;
            if (link_level == MAX_SYMLINK)
            {
               errno = ELOOP;
               return 0;
            }
            link_level++;
            fn_buf[bytes_copied] = '\0';
            /* We can get /dev/env/SOMEVARIABLE as a symlink target. Do not
               restart processing in this case, but substitute /dev/env/SOMEVARIABLE
               with expanded SOMEVARIABLE.  Note that SOMEVARIABLE may expand to
               either relative or absolute path. */
            if (strncmp(fn_buf, "/dev/env/", strlen("/dev/env/")))
            	strcpy(resolved, fn_buf);
            else
            {
            	/* Fill `resolved' with expanded env variable. Do this by
            	   calling _put_path(). TODO: It could be nice to factor
            	   out some code from _put_path2() to separate function to expand
            	   /dev/env/FOO once, i.e. do not recurse as _put_path2() does.
            	   However, it is not completely trivial, so current implementation
            	   takes a performance hit there. */
            	_put_path(fn_buf);
            	dosmemget(__tb, FILENAME_MAX, resolved);
            }

            /* FIXME: does absolute path check below work with chroot()? */
            if (((bytes_copied > 2) &&  (resolved[1] == ':')) ||
                ((bytes_copied > 0) && ((resolved[0] == '/') ||
                                        (resolved[0] == '\\'))))
            {
               found_absolute_path = 1;
            }
            /* If we found dir/file, do not iterate */
            else
            {
               if ((resolved[0] == '.') &&
                  ((resolved[1] == '/') || (resolved[1] == '\\')))
                  ptr = resolved + 2;
               else
                  ptr = resolved;
               while (ptr < resolved + strlen(resolved)) /* Skip last char */
                  if ((*ptr == '/') || (*ptr == '\\'))
                  {
                     bytes_copied = -1;
                     non_trivial = 1;
                     break;
                  }
                  else
                     ptr++;
            }
         }
      } while (bytes_copied != -1);
      errno = old_errno;
      if (done_something)
      {
         /* If it wasn't the last path component resolved, save the
          * unresolved tail for future
          */
         if (*end)
            strcpy(fn_buf, end);
         else
            fn_buf[0] = '\0';
         if (found_absolute_path)
         {
             /* Discard already resolved part, because symlink's target */
             /* is absolute path */
             strcpy(__real_path, resolved);
         }
         else
         {
             /* Add resolved symlink component to already resolved part */
             memcpy(start, resolved, strlen(resolved) + 1);
         }
         /* Append saved tail for further processing */
         if (fn_buf[0])
             strcat(__real_path, fn_buf);
      }
      if (done_something)
      {
         if (found_absolute_path)
         {
            /* Restart processing. God knows what's in the new absolute path */
            start = __real_path;
            end = strpbrk(__real_path, "/\\");
         }
         else if (non_trivial)
         {
            /* If we got component like 'dir/file', start resolving from */
            /* dir.  */
            end = strpbrk(start + 1, "/\\");
         }
         else
         {
             /* Resolve next component */
             advance(&start, &end);
         }
      }
      else
      {
          /* Resolve next component */
          advance(&start, &end);
      }
      if (!end)
          end = __real_path + strlen(__real_path);
   }
   return 1;
}

/* Advance to the next portion of the path in the case we won't need
   previously resolved part anymore.  Cope with multiple slashes. */
static void advance(char ** s, char ** e)
{
   *s = strpbrk(*s + 1, "/\\");
   if (*s)
   {
     while ((**s == '/') || (**s == '\\'))
       (*s)++;
   }

   while ((**e == '/') || (**e == '\\'))
     (*e)++;
   *e = strpbrk(*e, "/\\");
}
