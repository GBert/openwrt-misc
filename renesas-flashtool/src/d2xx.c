/*
  Copyright (C) 2008, 2009, 2010 DJ Delorie <dj@redhat.com>

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
#if defined(WIN32) || defined(__CYGWIN__)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "ftd2xx.h"
#include "serial.h"

extern int verbose;
#define dprintf if (verbose>1) printf

static FT_HANDLE handle;
static int timeout_val = 0;
static int use_cnvss = 0;
static int serial_boot_mode = 2;

void
serial_set_mode (int sbm)
{
  serial_boot_mode = sbm;
}

static struct {
  int code;
  const char *msg;
} ftdi_error_strings[] = {
  { FT_OK, "FT_OK" },
  { FT_INVALID_HANDLE, "FT_INVALID_HANDLE" },
  { FT_DEVICE_NOT_FOUND, "FT_DEVICE_NOT_FOUND" },
  { FT_DEVICE_NOT_OPENED, "FT_DEVICE_NOT_OPENED" },
  { FT_IO_ERROR, "FT_IO_ERROR" },
  { FT_INSUFFICIENT_RESOURCES, "FT_INSUFFICIENT_RESOURCES" },
  { FT_INVALID_PARAMETER, "FT_INVALID_PARAMETER" },
  { FT_INVALID_BAUD_RATE, "FT_INVALID_BAUD_RATE" },
  { FT_DEVICE_NOT_OPENED_FOR_ERASE, "FT_DEVICE_NOT_OPENED_FOR_ERASE" },
  { FT_DEVICE_NOT_OPENED_FOR_WRITE, "FT_DEVICE_NOT_OPENED_FOR_WRITE" },
  { FT_FAILED_TO_WRITE_DEVICE, "FT_FAILED_TO_WRITE_DEVICE" },
  { FT_EEPROM_READ_FAILED, "FT_EEPROM_READ_FAILED" },
  { FT_EEPROM_WRITE_FAILED, "FT_EEPROM_WRITE_FAILED" },
  { FT_EEPROM_ERASE_FAILED, "FT_EEPROM_ERASE_FAILED" },
  { FT_EEPROM_NOT_PRESENT, "FT_EEPROM_NOT_PRESENT" },
  { FT_EEPROM_NOT_PROGRAMMED, "FT_EEPROM_NOT_PROGRAMMED" },
  { FT_INVALID_ARGS, "FT_INVALID_ARGS" },
  { FT_NOT_SUPPORTED, "FT_NOT_SUPPORTED" },
  { FT_OTHER_ERROR, "FT_OTHER_ERROR" }
};
#define NUM_ERROR_STRINGS (sizeof(ftdi_error_strings) / sizeof(ftdi_error_strings[0]))

static void
Fail1(int status, const char *file, int line, const char *sline)
{
  const char *er = "Unknown error";
  int i;
  if (status == FT_OK)
    return;

  for (i=0; i<NUM_ERROR_STRINGS; i++)
    if (ftdi_error_strings[i].code == status)
      er = ftdi_error_strings[i].msg;

  printf("%s:%d: Error %s (%d) doing %s\n", file, line, er, status, sline);
  exit(1);
}

#define Fail(x) Fail1(x,__FILE__,__LINE__,#x)

static int cbus = 0;

static void
set_cbus (int c)
{
  cbus |= 1 << c;
  Fail (FT_SetBitMode (handle, 0xf0 | cbus, 0x20));
}

static void
clear_cbus (int c)
{
  cbus &= ~(1 << c);
  Fail (FT_SetBitMode (handle, 0xf0 | cbus, 0x20));
}

static void
Reset(int reset)
{
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
    default:
      if (reset)
	Fail (FT_ClrDtr (handle));
      else
	Fail (FT_SetDtr (handle));
      break;
    }
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
  ProgramMode (0);
  Reset (1);
  /* >3 mSec reset pulse */
  usleep(3*1000);
  Reset (0);
  FT_Close (handle);
}

void
serial_param (char *parameter)
{
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

  switch (board_type)
    {
    case BT_powermeter:
      chipname = "FT232R PowerMeter";
      break;
    case BT_powermeterproto:
      chipname = "FT232R PowerMeter";
      break;
    default:
      chipname = "FT232R - R8C";
      break;
    }

  Fail (FT_OpenEx (chipname, FT_OPEN_BY_DESCRIPTION, &handle));
  if (verbose > 2)
    printf("handle: 0x%x\n", (int)handle);
  atexit (serial_close);

  ProgramMode (1);
  Reset (1);
  /* >3 mSec reset pulse */
  usleep(3*1000);

  Reset (0);

  Fail (FT_SetBaudRate (handle, 9600));
  Fail (FT_SetDataCharacteristics (handle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE));
  FT_SetFlowControl (handle, FT_FLOW_NONE, 0, 0);

  /* 3 mSec delay to let board "wake up" after reset */
  usleep(3*1000);
}

void
serial_change_baud (int baud)
{
  Fail (FT_SetBaudRate (handle, baud));
}

void
serial_set_timeout (int mseconds)
{
  if (verbose > 2)
  dprintf("timeout = %d\n", mseconds);
  if (mseconds == -1)
    mseconds = 0x7fffffff;
  timeout_val = mseconds;
  FT_SetTimeouts (handle, timeout_val, 0x7fffffff);
}

void
serial_use_cnvss (int use_it)
{
  use_cnvss = use_it;
}

void
serial_sync ()
{
  DWORD rx, tx, ev;
  while (1)
    {
      FT_GetStatus (handle, &rx, &tx, &ev);
      if (tx == 0)
	break;
      usleep (1*1000);
    }
  if (verbose > 2)
    printf("\033[31m(S)\033[0m");
}

void
serial_drain ()
{
  FT_Purge (handle, FT_PURGE_RX);
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
  DWORD num;

  if (verbose > 1)
    dw (ch);
  FT_Write (handle, &ch, 1, &num);
  serial_sync ();
  usleep(100);
}

void
serial_write_block(unsigned char *ch, int len)
{
#if 1
  while (len--)
    serial_write(*ch++);
  serial_sync ();
#else
  DWORD num;
  if (verbose > 1)
    {
      int i;
      printf("\033[36m[%d]\033[0m", len);
      for (i=0; i<len; i++)
	dw (ch[i]);
    }
  while (len > 32)
    {
      FT_Write (handle, ch, 32, &num);
      ch += 32;
      len -= 32;
      serial_sync ();
    }
  if (len)
    FT_Write (handle, ch, len, &num);
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
  DWORD num;
  if (verbose > 1)
    {
      int i;
      for (i=0; ch[i]; i++)
	dw (ch[i]);
    }
  FT_Write (handle, &ch, strlen(ch), &num);
#endif
}

/* returns -1 for timeout, else 0..255 */
int
serial_read(void)
{
  struct timeval tv1, tv2;
  long et, oet;
  DWORD rv, stat;
  unsigned char buf;

  gettimeofday(&tv1, 0);
  do {
    stat = FT_Read (handle, &buf, 1, &rv);
    if (rv == 0)
      usleep (10);
    gettimeofday(&tv2, 0);
    et = tv2.tv_usec - tv1.tv_usec;
    et += (tv2.tv_sec - tv1.tv_sec) * 1000000;
    et /= 1000;
    oet = et;
  } while (stat == FT_OK
	   && rv == 0
	   && et < timeout_val);

  if (stat != FT_OK)
    {
      printf("[read failed? %ld]", stat);
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
  DWORD rxqueue, txqueue, stat, estat;
  stat = FT_GetStatus (handle, &rxqueue, &txqueue, &estat);
  return rxqueue;
}

int
serial_setup_console ()
{
  ProgramMode (0);
  Reset (1);
  /* >3 mSec reset pulse */
  usleep(3*1000);
  serial_drain ();
  Reset (0);
  FT_SetUSBParameters (handle, 64, 64);
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
