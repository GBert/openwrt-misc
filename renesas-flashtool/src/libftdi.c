/*
  Copyright (C) 2008, 2009 DJ Delorie <dj@redhat.com>

  This file is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 3, or (at your option) any later
  version.
  
  This file is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.
  
  You should have received a copy of the GNU General Public License
  along with this file; see the file COPYING3.  If not see
  <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include <unistd.h>

#include <ftdi.h>
#include "serial.h"

extern int verbose;
#define dprintf if (verbose>1) printf

static struct ftdi_context handle;
static int timeout_val = 0;
static int use_cnvss = 0;
static int did_console_mode = 0;
static int serial_boot_mode = 2;
static int cbus_baud_scale = 1;
static int rxusb_mode = 0;

void
serial_set_mode (int sbm)
{
  serial_boot_mode = sbm;
}

static int
Fail1(int status, const char *file, int line, const char *sline)
{
  const char *er = "Unknown error";

  if (status >= 0)
    return status;

  er = ftdi_get_error_string (&handle);

  printf("%s:%d: Error %s (%d) doing %s\n", file, line, er, status, sline);
  exit(1);
}

#define Fail(x) Fail1(x,__FILE__,__LINE__,#x)

static int cbus = 0;

static void
set_cbus_io (int mask, int bits)
{
  Fail (ftdi_set_bitmode (&handle, (mask << 4) | bits, 0x20));
}

static void
set_cbus (int c)
{
  cbus |= 1 << c;
  Fail (ftdi_set_bitmode (&handle, 0xf0 | cbus, 0x20));
}

static void
clear_cbus (int c)
{
  cbus &= ~(1 << c);
  Fail (ftdi_set_bitmode (&handle, 0xf0 | cbus, 0x20));
}

static int reset_state = 0;
static int forcetx_state = 0;

/* Bit Bang Pins

    1  TXD >
    5  RXD <
    3  RTS >
   11  CTS <
    2  DTR >
    9  DSR <
   10  DCD <
    6  RI  <
*/

static void
ForceTxLow (int low)
{
  if (low)
    {
      unsigned char ch = 0;
      /* Tx and DTR output */
      if (! forcetx_state)
	Fail (ftdi_set_bitmode (&handle, 0x15, 1));
      ch = 0x00 | (reset_state ? 0x00 : 0x10);
      ftdi_write_data (&handle, &ch, 1);
    }
  else
    Fail (ftdi_set_bitmode (&handle, 0x00, 0));

  forcetx_state = low;
}

static void
Reset(int reset)
{
  reset_state = reset;
  switch (board_type)
    {
    case BT_powermeter:
      if (reset)
	clear_cbus (2);
      else
	set_cbus (2);
      break;
    case BT_powermeterproto:
      if (reset)
	clear_cbus (3);
      else
	set_cbus (3);
      break;
    case BT_m32c10p:
    case BT_rx:
    default:
      if (reset)
	Fail (ftdi_setdtr (&handle, 0));
      else
	Fail (ftdi_setdtr (&handle, 1));
      break;
    }
  if (forcetx_state)
    ForceTxLow (1);
}

static void
ProgramMode(int prog)
{
  /* CNVss is active HIGH, MODE is active LOW.  Active == program mode.  */
  switch (board_type)
    {
    case BT_powermeter:
      if ((!prog) == (!use_cnvss))
	set_cbus (3);
      else
	clear_cbus (3);
      break;
    case BT_rx600:
      /* MD1/CB3  MD0/CB2
	    0        1     Boot Mode
	    1        0     User Bood Mode
	    1        1     User Mode */
      if (prog)
	{
	  clear_cbus (3);
	  set_cbus (2);
	}
      else
	{
	  set_cbus (3);
	  set_cbus (2);
	}
      break;
    case BT_rx:
      /* MD1/CB2  MD0/CB3
	    0        1     Boot Mode
	    1        0     User Boot Mode / USB mode
	    1        1     User Mode */
      if (prog && rxusb_mode)
	{
	  clear_cbus (3);
	  set_cbus (2);
	}
      else if (prog)
	{
	  set_cbus (3);
	  clear_cbus (2);
	}
      else
	{
	  set_cbus (3);
	  set_cbus (2);
	}
      break;

    case BT_m32c10p:
      /*                   PROG   RUN  PULLED
         CBUS0 = CE    P50  H      x	H
	 CBUS1 = EPM   P55  L      x	L
	 CBUS2 = CNVSS      H      L    L
      */
      if (prog)
	set_cbus_io (4, 4);
      else
	set_cbus_io (0, 0);
      break;

    default:
      if ((!prog) == (!use_cnvss))
	set_cbus (2);
      else
	clear_cbus (2);
    }
}

static void
serial_close ()
{
  if (!did_console_mode)
    {
      ProgramMode (0);
      Reset (1);
      /* >3 mSec reset pulse */
      usleep(3*1000);
      Reset (0);
    }

  ftdi_usb_reset (&handle);
  ftdi_usb_close (&handle);
  ftdi_deinit (&handle);
}

void
serial_param (char *parameter)
{
  if (strcmp (parameter, "rxusb") == 0)
    rxusb_mode = 1;
}

int
serial_need_autobaud ()
{
  return 1;
}

void
serial_init (char *port, int baud)
{
  char *chipname;
  int status;

  ftdi_init (&handle);

  /* "port" here is either "0" through "99" for the Nth instance of a
     matching device, or 000 through 999 for the Linux USB device
     number (ala lsusb).  */
  if (port && memcmp (port, "/dev/", 5) == 0)
    port = NULL;

  if (port)
    {
      int i;
      int chosen = -1;
      int num_devices;
      struct ftdi_device_list *device_info;
      struct usb_device *u_chose = NULL;
      int again = 0;

      if (strlen (port) < 3)
	chosen = atoi (port);

    try_again:
      num_devices = Fail (ftdi_usb_find_all (&handle, &device_info, 0x0403, 0x6001));

      if (num_devices == 0)
	{
	  printf("no FTDI devices present.\n");
	  exit(1);
	}

      for (i=0; i<num_devices; i++)
	{
	  char manufacturer[200], description[200], serial[200];
	  struct usb_device *u = device_info->dev;

	  ftdi_usb_get_strings (&handle, u, manufacturer, 200, description, 200, serial, 200);

	  if (chosen == -1)
	    {
	      if (strcmp (device_info->dev->filename, port) == 0)
		u_chose = u;
	    }
	  else
	    if (i == chosen)
	      u_chose = u;

	  if (again)
	    {
	      printf("Dev %2d: ", i);
	      printf("Filename \"%s\", " , device_info->dev->filename);
	      printf("Manf \"%s\", ", manufacturer);
	      printf("Desc \"%s\", ", description);
	      printf("Ser# \"%s\"\n", serial);
	    }
	  device_info = device_info->next;
	}
      if (again)
	exit (1);

      if (u_chose)
	Fail (ftdi_usb_open_dev (&handle, u_chose));
      else
	{
	  if (chosen == -1)
	    printf("USB device \"%s\" not found\n", port);
	  else
	    printf("USB device #%d doesn't exist\n", chosen);
	  again = 1;
	  goto try_again;
	}
    }
  else
    {
    try_again2:
      switch (board_type)
	{
	case BT_powermeter:
	  chipname = "FT232R PowerMeter";
	  break;
	case BT_powermeterproto:
	  chipname = "FT232R PowerMeter";
	  break;
	case BT_rx600:
	  chipname = "FT232R - RX600";
	  break;
	case BT_rx:
	  chipname = "FT232R - gRX";
	  break;
	case BT_m32c10p:
	  chipname = "FT232R USB M32C 10pin";
	  break;
	default:
	  chipname = "FT232R - R8C";
	  break;
	}

      status = ftdi_usb_open_desc (&handle, 0x0403, 0x6001, chipname, NULL);
      if (status < 0 && board_type == BT_rx)
	{
	  board_type = BT_rx600;
	  goto try_again2;
	}
      Fail(status);
    }

  atexit (serial_close);

  ProgramMode (1);

  if (serial_boot_mode == 3)
    ForceTxLow(1);

  Reset (1);
  /* >3 mSec reset pulse */
  usleep(3*1000);

  Reset (0);

  if (serial_boot_mode == 3)
    {
      usleep (50*1000);
      ForceTxLow(0);
      usleep (200*1000);
    }

  Fail (ftdi_set_baudrate (&handle, baud));
  if (handle.baudrate > baud*2)
    {
      /* bug in 0.17 and 0.18 at least - enabling CBUS mode scales
	 baud rates by 4.  */
      cbus_baud_scale = 4;
      Fail (ftdi_set_baudrate (&handle, baud / cbus_baud_scale));
    }
  else
    cbus_baud_scale = 1;

  Fail (ftdi_set_line_property (&handle, BITS_8, STOP_BIT_1, NONE));
  Fail (ftdi_setflowctrl (&handle, SIO_DISABLE_FLOW_CTRL));

  /* 3 mSec delay to let board "wake up" after reset */
  usleep(3*1000);
}

void
serial_change_baud (int baud)
{
  Fail (ftdi_set_baudrate (&handle, baud / cbus_baud_scale));
}

void
serial_set_timeout (int mseconds)
{
  if (verbose > 2)
  dprintf("timeout = %d\n", mseconds);
  if (mseconds == -1)
    mseconds = 0x7fffffff;
  timeout_val = mseconds;
  //handle.usb_read_timeout = timeout_val;
}

void
serial_use_cnvss (int use_it)
{
  use_cnvss = use_it;
}

void
serial_sync ()
{
#if 0
  int rx, tx, ev;
  while (1)
    {
      FT_GetStatus (handle, &rx, &tx, &ev);
      if (tx == 0)
	break;
      usleep (1*1000);
    }
  if (verbose > 2)
    printf("\033[31m(S)\033[0m");
#endif
}

void
serial_drain ()
{
  ftdi_usb_purge_rx_buffer (&handle);
  if (verbose > 2)
    printf("\033[31m(D)\033[0m");
}

void
serial_pause (int msec)
{
  serial_sync ();
  if (msec)
    usleep (msec*1000);
}

static void dw (unsigned char ch)
{
  if (isgraph(ch))
    printf("\033[32m%c\033[0m", ch);
  //else
    printf("\033[36m%02x\033[0m ", ch);
}

void
serial_write (unsigned char ch)
{
  int num;

  if (verbose > 1)
    dw (ch);
  num = ftdi_write_data (&handle, &ch, 1);
  serial_sync ();
  usleep(100);
}

void
serial_write_block(unsigned char *ch, int len)
{
#if 0
  while (len--)
    serial_write(*ch++);
  serial_sync ();
#else
  int num;
  if (verbose > 1)
    {
      int i;
      printf("\033[36m[%d]\033[0m", len);
      for (i=0; i<len; i++)
	dw (ch[i]);
    }
#if 0
  while (len > 32)
    {
      num = ftdi_write_data (&handle, ch, 32);
      ch += num;
      len -= num;
      serial_sync ();
    }
#endif
  if (len)
    num = ftdi_write_data (&handle, ch, len);
  serial_sync ();
#endif
}

void
serial_write_string(char *ch)
{
  while (*ch)
    serial_write(*ch++);
  serial_sync ();
#if 0
  int num;
  if (verbose > 1)
    {
      int i;
      for (i=0; ch[i]; i++)
	dw (ch[i]);
    }
  num = ftdi_write_data (handle, &ch, strlen(ch));
#endif
}

/* -1 means "empty", other values mean "full" */
static int read_cache = -1;

/* returns -1 for timeout, else 0..255 */
int
serial_read(void)
{
  struct timeval tv1, tv2;
  long et, oet;
  int rv;
  unsigned char buf;

  if (read_cache != -1)
    {
      rv = read_cache;
      read_cache = -1;
      return rv;
    }

  gettimeofday(&tv1, 0);
  do {
    rv = ftdi_read_data (&handle, &buf, 1);
    if (rv == 0)
      usleep (10);
    gettimeofday(&tv2, 0);
    et = tv2.tv_usec - tv1.tv_usec;
    et += (tv2.tv_sec - tv1.tv_sec) * 1000000;
    et /= 1000;
    oet = et;
  } while (rv >= 0
	   && rv == 0
	   && et < timeout_val);

  if (rv < 0)
    {
      Fail(rv);
      printf("[read failed? %d]", rv);
      return -1;
    }
  if (rv == 0)
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

/* Returns number of bytes ready, or zero.  */
int
serial_ready (void)
{
  unsigned char ch;
  int rv;

  if (read_cache != -1)
    return 1;

  rv = ftdi_read_data (&handle, &ch, 1);
  if (rv == 1)
    {
      if (verbose > 1)
	{
	  if (isgraph(ch))
	    printf("\033[31m%c\033[0m", ch);
	  //else
	  printf("\033[35m%02x\033[0m ", ch);
	  fflush(stdout);
	}
      read_cache = ch;
      return 1;
    }
  return 0;
}

int
serial_setup_console ()
{
  did_console_mode = 1;

  ProgramMode (0);
  Reset (1);
  /* >3 mSec reset pulse */
  usleep(3*1000);
  serial_drain ();
  Reset (0);
  ftdi_write_data_set_chunksize (&handle, 64);
  ftdi_read_data_set_chunksize (&handle, 64);
  return 1;
}

void
serial_reset ()
{
  usleep (10000);
  Reset (1);
  usleep (10000);
  Reset (0);
  usleep (10000);
  while (serial_ready ())
    serial_read ();
}

void
serial_trigger_scope ()
{
  set_cbus(2);
  clear_cbus(2);
  set_cbus(2);
  clear_cbus(2);
}
