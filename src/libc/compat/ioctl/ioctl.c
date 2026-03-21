/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2010 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/*
** File    : /djgpp/src/libc/posix/fcntl/ioctl.c
** Author  : Tom Demmer (demmer@lstm.ruhr-uni-bochum.de)
** SYNOPSIS: ioctl() for djgpp.
** Aim:
**   Provide an ioctl function that does what the naive DOS C programmer
**   expects.
**   Provide an ioctl function that does what the naive UNIX C programmer
**   expects.
**   Mangle and shredder them, roll them into one function and expect it
**   to work both ways.
**   If you really do that, you _ARE_ naive.
**
**   Generally, we follow this approach:
**   Commands for ioctl are 8 bit long because they must fit into AL.
**   The other 8 bit are usually unused. We put some flags inside, e.g.
**   if we need a transfer buffer, which register the DOS ioctl call wants
**   to return on saturdays and sun shine and such. The low byte of the
**   high word takes some more flags.
**
**
**
**   The UNIX ioctls have the command in the lower word (16-Bit).
**   In the high word, the upper three bits tell us if the parameters
**   are in or out parameters. Bit 15 means IN, Bit 14 means OUT, Bit
**   13 means VOID. Because there _must_ be one of them set, we can
**   distinguish UNIX from DOS ioctls. The other tell us the size of the
**   Input/Output parameters.
**   The high byte of the lower word will be used to specify the class
**   to which the command belongs, e.g. `T' for tty. Currently not
**   implemented.
**
**
**   It looks like this:
**   DOS:
**   000fffff ffffffff XFFiibRR CCCCCCCC
**   UNIX:
**   IOVxxxxx xSSSSSSS CCCCCCCC CCCCCCCC
**
**   With
**      C: Command part
**      X: Use Xfer buffer
**      F: What to return
**      f: Still free. Will be used for the generioc ioctl request.
**      i: Number of additional input args (dx,si,di)
**         I think CX is always used or can be safely set to 0
**      b: Brain Dead Flag for generic ioctl
**      R: reserved (might become the number of args someday)
**
**      S: sizeof input/output parameter
**      I: Input  flag
**      O: Output flag
**      V: Void Flag (no parameters)
**      x: Currently unused.
**
**   Right now the function supports the DOS ioctl calls up to 0x440b.
**   If you want to add another function, it goes like this:
**   #define DOS_PLAIN_your_func  0xff // e.g
**   #define DOS_your_func (DOS_PLAIN_your_func | \
**                          [DOS_XFER]| \
**                          [DOS_RETAX/DOS_RETDX/DOS_RETDISI] |\
**                          [DOS_XINARGS(xtra-args)])
**
**
** What is completely missing is the UNIX ioctl handler. I don't have the
** slightest idea about what to do here. Right now it just checks for an FSE
** and calls that if it exist. Otherwise we just return -1.
**
**
$Id: ioctl.c,v 1.10 2015/05/08 16:38:44 andris Exp $
$Log: ioctl.c,v $
Revision 1.10  2015/05/08 16:38:44  andris
src/libc/compat/ioctl/ioctl.c: add missing va_end (patch from Ozkan Sezer)

Revision 1.9  2015/05/02 07:32:15  andris
Update file copyright lines aftr running src/copyright.pl

Revision 1.8  2008/04/07 20:35:57  juan.guerrero
Use __FSEXT_func_wrapper instead of calling functions directly
with wrong type of arguments.
Patch by Ozkan Sezer, 2008-03-26.

Revision 1.7  2003/05/10 15:26:11  richdawe
__tb_size is defined in <go32.h> now.

Revision 1.6  2002/10/17 23:00:24  richdawe
Update copyright messages

Revision 1.5  2001/08/09 03:59:13  snowball
Add bare-bones implementation of TIOCSWINSZ.

Revision 1.4  2001/08/08 17:03:19  snowball
Implement TIOCGWINSZ for retrieving the screen height and width.

Revision 1.3  2001/06/20 16:54:33  ams
Do not mix signed and unsigned.

Revision 1.2  1998/06/28 17:25:20  dj
import djgpp 2.02

 * Revision 0.5  1996/07/30  09:42:23  DEMMER
 * Minor code cleanups. Final beta.
 *
 * Revision 0.4  1996/07/29  13:03:29  DEMMER
 * Added va_end(). Probably uneeded for Intel machines. Anyway.
 *
 * Revision 0.3  1996/07/29  12:44:55  DEMMER
 * Split the header stuff from the source
 * Changed encoding bits
 *
 * Revision 0.2  1996/07/04  11:17:37  DEMMER
 * Revised flag scheme.
 * Correct some minor bugs
 *
 * Revision 0.1  1996/07/03  15:42:01  DEMMER
 * Cleaned up flag stuff.
 * Added UNIX ioctl test routines
 *
 * Revision 0.0  1996/07/03  13:53:23  DEMMER
 * Initial version
 *
*/



#include <libc/stubs.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/fsext.h>
#include <libc/fsexthlp.h>
#include <sys/ioctl.h>
#include <libc/farptrgs.h>


/****************************************************************************/
/****************************************************************************/
/* S T A R T   O F   I M P L E M E N T A T I O N  ***************************/
/****************************************************************************/
/****************************************************************************/


#include <errno.h>
#include <go32.h>
#include <dpmi.h>
#include <sys/fsext.h>
#include <dos.h>
#include <libc/dosio.h>

static int _dos_ioctl(int fd, int cmd, int argcx,int argdx,int argsi,int argdi,
                      int xarg)
{
/*
** Do an int0x21,0x44xx and such
*/
    __dpmi_regs r = {0};
    int dos_selector = 0;
    int dos_segment = 0;

    /*
    ** Lower word of cmd -> al
    */
    r.x.ax = 0x4400 |(cmd &0xff);
    r.x.bx = fd;
    r.x.cx = argcx;
    r.x.dx = argdx;
    r.x.si = argsi;
    r.x.di = argdi;
    /*
    ** This one must have dl=0
    */
    if(cmd == DOS_SETDEVDATA) r.x.dx = argdx &0xf;
    /*
    ** Normally, there is a difference between reading and writing.
    ** But some functions require codes in the buffer on input already.
    ** So we transfer before int and after the int.
    ** We always use DX as a pointer to the buffer.
    ** I _do_ like clear APIs.
    */
    if(cmd & DOS_XFER){
        if(argcx <= (int)__tb_size){ /* Can we use transfer buffer ? */
            dosmemput((void *)(uintptr_t)argdx,argcx, __tb);
            r.x.ds = (__tb>>4) &0xffff;
            r.x.dx = __tb &0xf;
        }
        else{
            /* No, we have to get some DOS mem ourselves */
            dos_segment = __dpmi_allocate_dos_memory((argcx+15)>>4,
                                                     &dos_selector);
            r.x.ds = dos_segment;
            if(-1 == dos_segment){
                errno = ENOMEM;
                return -1;
            }
        }
    }
    /*
    ** At DOS 3.2+ MS decided to change the API to an a bit more
    ** abstruse way. CX now does not hold the number of bytes
    ** to r/w, but is now a function_number:categegory_code.
    ** The size of the XBuffer is determined by the device driver
    ** which can set it to whatever it likes.
    ** We do not have second sight, so you'll have to add this size as a
    ** parameter after all the registers and the buffer pointer.
    */
    if( cmd & DOS_BRAINDEAD  ){
        if(xarg <= (int)__tb_size){ /* Can we use transfer buffer ? */
            dosmemput((void *)(uintptr_t)argdx,xarg, __tb);
            r.x.ds = (__tb>>4) &0xffff;
            r.x.dx = __tb &0xf;
        }
        else{
            /* No, we have to get some DOS mem ourselves */
            dos_segment = __dpmi_allocate_dos_memory((xarg+15)>>4,
                                                      &dos_selector);
            r.x.ds = dos_segment;
            if(-1 == dos_segment){
                errno = ENOMEM;
                return -1;
            }
        }
    }
    /*
    ** Call DOS
    */
    if(-1 == __dpmi_int(0x21,&r)){
        if(dos_selector) __dpmi_free_dos_memory(dos_selector);
        errno = EINVAL;
        return -1;
    }
    errno = 0 ;
    /*
    ** DOS error?
    */
    if(r.x.flags &1){
        if(dos_selector) __dpmi_free_dos_memory(dos_selector);
        errno = __doserr_to_errno(r.x.ax);
        return -1;
    }
    /*
    ** move back to our buffers and find the return value
    */
    if(cmd & DOS_XFER){
        if(dos_selector){
            dosmemget(dos_segment<<4,argcx,(void*)(uintptr_t) argdx);
            __dpmi_free_dos_memory(dos_selector);
        }
        else
           dosmemget(__tb,argcx,(void *)(uintptr_t) argdx);
    }
    if(cmd & DOS_BRAINDEAD){
        if(dos_selector){
            dosmemget(dos_segment<<4,xarg,(void*)(uintptr_t) argdx);
            __dpmi_free_dos_memory(dos_selector);
        }
        else
           dosmemget(__tb,xarg,(void *)(uintptr_t) argdx);
    }
    /*
    ** Return the requested value or 0.
    */
    switch(cmd & DOS_RETMASK){
        case DOS_RETAX   :   return r.x.ax;
        case DOS_RETDX   :   return r.x.dx;
        case DOS_RETDISI :   return (r.x.di << 16) | r.x.si;
        default          :   return 0;
    }
    /* NOTREACHED */
}


static int _unix_ioctl(int fd, int cmd, va_list args)
{
  __FSEXT_Function *func = __FSEXT_get_function(fd);
  if(func)
  {
    int rv;
    if (__FSEXT_func_wrapper(func, __FSEXT_ioctl, &rv, fd))
       return rv;
  }

  switch (cmd)
  {
    case TIOCGWINSZ:
    {
      struct winsize *win;

      win = va_arg(args, struct winsize *);

      _farsetsel(_dos_ds);
      win->ws_row = _farnspeekb(0x0484) + 1;
      win->ws_col = _farnspeekw(0x044a);
      win->ws_xpixel = 1;
      win->ws_ypixel = 1;
      return 0;
    }

    case TIOCSWINSZ:  /* Do nothing implementation.  */
    {
      struct winsize *win;

      win = va_arg(args, struct winsize *);

      _farsetsel(_dos_ds);
      if (win->ws_row == _farnspeekb(0x484) + 1
          && win->ws_col == _farnspeekw(0x44a))
        return 0;
      break;
    }
  }

  /* All else fails.  */
  errno =  ENOSYS;
  return -1;
}

/*
**
** The user callable entry point.
**
*/
int ioctl(int fd, int cmd, ...)
{
  int argcx,argdx,argsi,argdi;
  int narg,xarg;
  __FSEXT_Function *func = __FSEXT_get_function(fd);
  int rv;
  va_list args;

  /**
   ** see if this is a file system extension file
   **
   */
  if (func)
  {
    char *arg;

    va_start(args, cmd);
    arg = va_arg(args, char *);
    va_end(args);

    if (__FSEXT_func_wrapper(func, __FSEXT_ioctl, &rv, fd, cmd, arg))
      return rv;
  }

  va_start(args, cmd);

  if(__IS_UNIX_IOCTL(cmd))
  {
#ifdef TEST
    {
    int inflg   = (cmd & IOC_IN)   == IOC_IN;
    int outflg  = (cmd & IOC_OUT)  == IOC_OUT;
    int voidflg = (cmd & IOC_VOID) == IOC_VOID;

    int size = (cmd >> 16) & IOCPARM_MASK;
    char i_class = (cmd &0xff00) >> 8;
    printf("Calling UNIX ioctl %x:\n"
           "Class:\t\t'%c'\n"
           "Inflag:\t\t%d\tOutflag:\t%d\tvoidflg:\t%d\n"
           "Size:\t\t%d\n",
           cmd & 0xffff,  i_class,
           inflg,outflg,voidflg,size);
    }
#endif
     rv = _unix_ioctl(fd, cmd, args);
     va_end(args);
     return rv;
  }
  /* Handle a DOS request */
  /* extract arguments */
  narg = (cmd >> 12) & 3;
  argdx=argsi=argdi=xarg=0;
  argcx = va_arg(args,int);

  if (narg > 0)             argdx = va_arg(args,int);
  if (narg > 1)             argdi = va_arg(args,int);
  if (narg > 2)             argsi = va_arg(args,int);
  if (cmd & DOS_BRAINDEAD)  xarg  = va_arg(args,int);
  va_end(args);

  return _dos_ioctl(fd,cmd,argcx,argdx,argsi,argdi,xarg);
}




#ifdef TEST

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main (int argc, char **argv)
{
    int fd;
    int res;
    short *s;
    char mybuf[512];

    if(-1== (fd=open("NUL",O_RDWR))){
        return 2;
    }
    printf("fd: %d\n",fd);

    res = ioctl(fd,DOS_GETDEVDATA,0,0);
    printf("nul:\tDEV_DATA: %x\n",res ) ;
    res = ioctl(fileno(stdout),DOS_GETDEVDATA,0,0);
    printf("stdout:\tDEV_DATA: %x\n",res ) ;
    res = ioctl(fileno(stdin ),DOS_GETDEVDATA,0,0);
    printf("stdin:\tDEV_DATA: %x\n",res ) ;
    close(fd);
    fd = open("ioctl.c",O_RDONLY);
    res = ioctl(fd,DOS_GETDEVDATA,0,0);
    printf("ioctl.c:\tDEV_DATA: %x\n",res ) ;
    close(fd);
    fd=open("EMMQXXX0",O_RDONLY);

    mybuf[0] = '\0';
    mybuf[1] = '\0';
    res = ioctl(fd,DOS_RCVDATA,6,&mybuf);
    s = (short *) &mybuf;
    if (*s == 0x25) printf("EMM386 > 4.45\n");
    mybuf[0] = '\x02';
    mybuf[1] = '\0';
    res = ioctl(fd,DOS_RCVDATA,2,&mybuf);
    printf("res: %d\tEMM Version %d.%d\n",res,(int )mybuf[0],(int) mybuf[1]);
    close(fd);

/*
**
** The generic ioctls need an extra parameter, because CX no longer
** holds the size for the buffer.
**
*/
/*  It would work like this:

    res=ioctl(fd,DOS_GENCHARREQ,category_code,&mybuf,driver_cmd,driver_cmd,
              sizeof(buffer table));
    R.Brown's interrupt list is not too specific about the role of SI and DI.
    It is important for European DOS 4.0 and OS/2 comp box. He just says
    they are the "parameter to pass to driver", whatever that might mean.
    I guess you can safely set the to 0.

*/

    printf("\n\nTIOCGETD = %x\n",TIOCGETD); ioctl(0,TIOCGETD,NULL);
    printf("TIOCSETD = %x\n",TIOCSETD); ioctl(0,TIOCSETD,NULL);
    printf("TIOCHPCL = %x\n",TIOCHPCL); ioctl(0,TIOCHPCL,NULL);
    return 0;
}
#endif

