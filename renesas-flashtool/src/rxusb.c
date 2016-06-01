#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <usb.h>

#include "serial.h"

#define MAXBUF		65536
#define NO_TIMEOUT	0x7fffffff
#define WRITE_EP	0x01
#define READ_EP		0x82

#if 0
#define DEBUG() fprintf (stderr, "%s:%d  %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__)
#else
#define DEBUG()
#endif

extern int verbose;
#define dprintf if (verbose>1) printf

typedef struct {
  unsigned char buf[MAXBUF];
  int iput, iget, count;
} Buffer;

Buffer wbuf;
Buffer rbuf;

/* returns one char */
static int
buf_getc (Buffer *b)
{
  int rv;
  if (b->count == 0)
    return -1;
  b->count --;
  rv = b->buf[b->iget++];
  b->iget %= MAXBUF;
  return rv;
}

/* returns number of chars now in buf */
static int
buf_gets (Buffer *b, unsigned char *buf, int len)
{
  int rv = 0;
  while (len > 0 && b->count > 0)
    {
      *buf++ = buf_getc (b);
      len --;
      rv ++;
    }
  return rv;
}

static void
buf_putc (Buffer *b, unsigned char c)
{
  b->count ++;
  b->buf[b->iput ++] = c;
  b->iput %= MAXBUF;
}

static void
buf_puts (Buffer *b, unsigned char *buf, int len)
{
  while (len > 0)
    {
      buf_putc (b, *buf);
      buf ++;
      len --;
    }
}

/* ------------------------------------------------------------ */

static int
Fail1 (int e, const char *filename, int lineno, const char *cmd)
{
  char *es;

#if 0
  fprintf(stderr, "%s => %d\n", cmd, e);
#endif

  if (e == -ETIMEDOUT)
    return e;
  if (e >= 0)
    return e;

  es = strerror (-e);
  es = es ? es : "unknown error";
  fprintf (stderr, "%s:%d: error: %s (%s)\n", filename, lineno, cmd, es);
  exit (1);
}

#define Fail(x) Fail1(x,__FILE__,__LINE__,#x)

/* ------------------------------------------------------------ */

static usb_dev_handle *handle;
static int timeout_msec;
static int doing_sync = 1;
static int do_wait = 0;

void
serial_param (char *parameter)
{
  if (strcmp (parameter, "wait") == 0)
    do_wait = 1;
}

static void
serial_close ()
{
  usb_release_interface (handle, 0);
  usb_close (handle);
}

int
serial_need_autobaud ()
{
  return 0;
}

void
serial_init(char *port, int baud)
{
  int count;
  struct usb_bus *bus;
  struct usb_device *dev;
  time_t start, now;
  int waited_for_it = 0;

  wbuf.iput = wbuf.iget = wbuf.count = 0;
  rbuf.iput = rbuf.iget = rbuf.count = 0;

  time (&start);

  DEBUG();
  usb_init ();
  usb_find_busses ();
 wait_for_it:
  count = usb_find_devices ();

  for (bus = usb_get_busses(); bus; bus=bus->next)
    {

      for (dev = bus->devices; dev; dev=dev->next)
	{
	  if (dev->descriptor.idVendor == 0x045b
	      && dev->descriptor.idProduct == 0x0025)
	    {
	      goto found_it;
	    }
	}
    }

  if (do_wait)
    {
      time (&now);
      waited_for_it = 1;
      if (now - start < 3)
	goto wait_for_it;
    }

  printf("No RX USB devices found.\n");
  exit (1);

 found_it:

  if (waited_for_it)
    usleep (100000);

  handle = usb_open (dev);

  Fail (usb_claim_interface (handle, 0));
  atexit (serial_close);

  DEBUG();
  return;
}

void
serial_reset ()
{
  doing_sync = 1;
}

void
serial_change_baud (int baud)
{
}

/* 1 = synchronous (not supported*
   2 = full duplex with separate MODE
   3 = half duplex on MODE */
void
serial_set_mode (int serial_boot_mode)
{
}

void
serial_use_cnvss (int use_cnvss)
{
}

int
serial_setup_console ()
{
  return 0;
}

void
serial_set_timeout (int mseconds)
{
  timeout_msec = mseconds;
}

static void
maybe_write (int all)
{
  DEBUG();
  while (wbuf.count > 0)
    {
      unsigned char buf[64], *bp;
      int r = buf_gets (&wbuf, buf, 64);
      bp = buf;
      while (r > 0)
	{
	  int w;

	  DEBUG();
	  w = Fail (usb_bulk_write (handle, WRITE_EP, (char *)bp, r, 100));
	  DEBUG();

#if 0
          printf("rxusb: %d out of %d bytes written\n", w, r);
#endif
	  if (w <= 0)
	    return;
	  bp += w;
	  r -= w;
	}
      if (!all && wbuf.count < 64)
	{
	  DEBUG();
	  return;
	}
    }
  DEBUG();
}

/* write out all output bytes and wait for them to finish transmitting. */
void
serial_sync ()
{
  DEBUG();
  maybe_write (1);
  DEBUG();
}

static void
usb_read_maybe (int block)
{
  unsigned char buf[64];
  int r;
  DEBUG();
  if (wbuf.count)
    serial_sync ();
  while (1)
    {
      r = Fail (usb_bulk_read (handle, READ_EP, (char *)buf, 64, block ? timeout_msec : 1));
      if (r == -ETIMEDOUT)
	break;
      if (r > 0)
	{
	  buf_puts (&rbuf, buf, r);
	  DEBUG();
	  return;
	}
      else
	break;
    }
  DEBUG();
}

/* read in any pending input bytes.  */
void
serial_drain ()
{
  DEBUG();
  do
    {
      rbuf.iput = rbuf.iget = rbuf.count = 0;
      usb_read_maybe (0);
    } while (rbuf.count > 0);
  DEBUG();
}

void
serial_pause (int msec)
{
  DEBUG();
  serial_sync ();
  usleep (msec * 1000);
  DEBUG();
}

static void dw (unsigned char ch)
{
  if (isgraph(ch))
    printf("\033[32m%c\033[0m", ch);
  //else
    printf("\033[36m%02x\033[0m ", ch);
}

void
serial_write(unsigned char ch)
{
  DEBUG();
  if (doing_sync && ch == 0)
    return;
  doing_sync = 0;
  if (verbose > 1)
    dw (ch);
  buf_putc (&wbuf, ch);
  if (wbuf.count > 64)
    maybe_write (0);
  DEBUG();
}

void
serial_write_block(unsigned char *ch, int len)
{
  DEBUG();
  buf_puts (&wbuf, ch, len);
  if (wbuf.count > 64)
    maybe_write (0);
  DEBUG();
}

void
serial_write_string(char *ch)
{
  DEBUG();
  buf_puts (&wbuf, (unsigned char *)ch, strlen (ch));
}

/* returns -1 for timeout, else 0..255 */
int
serial_read(void)
{
  int buf;

  DEBUG();
  if (wbuf.count)
    serial_sync ();
  if (rbuf.count == 0)
    usb_read_maybe (1);
  DEBUG();
  if (rbuf.count)
    buf = buf_getc (&rbuf);
  else
    {
      dprintf("[T]");
      return -1;
    }
  if (verbose > 1)
    {
      if (isgraph(buf))
	printf("\033[31m%c\033[0m", buf);
      //else
	printf("\033[35m%02x\033[0m ", buf);
      fflush(stdout);
    }
  return buf;
}

int
serial_ready ()
{
  DEBUG();
  if (wbuf.count)
    serial_sync ();
  if (rbuf.count == 0)
    usb_read_maybe (0);
  DEBUG();
  return rbuf.count;
}
