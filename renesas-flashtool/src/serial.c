/*
  Copyright (C) 2008, 2010 DJ Delorie <dj@redhat.com>

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
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>

#include "serial.h"

extern int verbose;

static char *port;
static int fd;
static int timeout = 10;
static int saved_baud = 0;
static int in_flash_mode = 0;
static int rx_stick = 0;

static int
baud_to_baud (int baud)
{
  switch (baud)
    {
    case 1200:
    case 12:
      baud = B1200;
      break;
    case 2400:
    case 24:
      baud = B2400;
      break;
    case 9600:
    case 96:
      baud = B9600;
      break;
    case 19200:
    case 192:
    case 19:
      baud = B19200;
      break;
    case 0:
    case 38400:
    case 384:
    case 38:
      baud = B38400;
      break;
    case 57600:
    case 576:
    case 57:
      baud = B57600;
      break;
    case 115200:
    case 1152:
    case 115:
      baud = B115200;
      break;
    default:
      printf("Unsupported baud rate %d\n", baud);
      exit(1);
    }
  return baud;
}

static char *lockn;

static void
set_stick_mode (int flash)
{
  int old_timeout = timeout;
  int old_baud = saved_baud;
  int rv, count;

  timeout = 100;

  serial_drain ();
  serial_change_baud (1200);
  serial_write (0);
  serial_pause (10); /* about a 7 ms break, so wait for it.  */
  serial_change_baud (9600);

  count = 10;
  do {
    if (--count == 0)
      {
	fprintf(stderr, "Unable to contact R8C on RX-Stick\n");
	exit(1);
      }
    serial_write (flash ? 'c' : 'b');
    rv = serial_read ();
  } while ((rv & ~1) != 'B');

  serial_change_baud (old_baud);
  timeout = old_timeout;
  in_flash_mode = flash;
}


void
serial_set_mode (int sbm)
{
}

static void
serial_close ()
{
  close (fd);
  unlink (lockn);
}

void
serial_param (char *parameter)
{
  if (strcmp (parameter, "rxstick") == 0)
    rx_stick = 1;
}

int
serial_need_autobaud ()
{
  return 1;
}

void
serial_init(char *_port, int baud)
{
  int lockf;
  char *portbase;
  struct termios attr;

  saved_baud = baud;
  baud = baud_to_baud (baud);

  if (!_port)
    _port = "/dev/ttyUSB0";

  portbase = strrchr(_port, '/');
  if (portbase == 0)
    portbase = _port;
  else
    portbase ++;
  lockn = (char *) malloc (strlen(portbase) + 16);
  sprintf(lockn, "/var/lock/LCK..%s", portbase);
  lockf = open(lockn, O_RDWR|O_CREAT|O_EXCL, 0666);
  if (lockf < 0)
    {
      printf("cannot lock %s - is someone else using it?\n", _port);
      printf("lock %s\n", lockn);
      perror("The error was");
      exit(1);
    }

  port = _port;
  fd = open(port, O_RDWR|O_NOCTTY|O_NDELAY);
  if (fd < 0)
    {
      printf("can't open %s\n", port);
      perror("The error was");
      exit(1);
    }

  atexit (serial_close);

  fcntl(fd, F_SETFL, 0);

  /* Reset by DTR - might work :-) */

  tcgetattr(fd, &attr);
  cfsetispeed(&attr, 0);
  cfsetospeed(&attr, 0);
  tcsetattr(fd, TCSAFLUSH, &attr);

  usleep(3*1000);

  tcgetattr(fd, &attr);
  cfsetispeed(&attr, baud);
  cfsetospeed(&attr, baud);
  cfmakeraw(&attr);

  if (rx_stick)
    set_stick_mode (1);

  attr.c_iflag &= ~(IXON | IXOFF | IXANY);
  attr.c_iflag |= IGNBRK;

  attr.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

  attr.c_cflag &= ~(PARENB | CSIZE | CRTSCTS);
  attr.c_cflag |= CS8 | CLOCAL | CSTOPB;
  attr.c_cc[VTIME] = 5;

  tcsetattr(fd, TCSANOW, &attr);

  /* 1 mSec delay to let board "wake up" after reset */
  usleep(3*1000);
}

void
serial_change_baud (int baud)
{
  struct termios attr;

  saved_baud = baud;

  if (verbose > 1)
    printf("[B%d]", baud);

  baud = baud_to_baud (baud);
  tcgetattr(fd, &attr);
  cfsetispeed(&attr, baud);
  cfsetospeed(&attr, baud);
  tcsetattr(fd, TCSANOW, &attr);
}

void
serial_use_cnvss (int use_it)
{
}

void
serial_set_timeout (int mseconds)
{
  timeout = mseconds;
}

void
serial_pause (int msec)
{
  tcdrain(fd);
  if (msec)
    usleep (msec*1000);
}

void
serial_sync ()
{
  tcdrain(fd);
}

void
serial_write (unsigned char ch)
{
  if (verbose > 1)
    {
      if (isgraph(ch))
	printf("\033[32m%c\033[0m", ch);
      else
	printf("\033[36m%02x\033[0m", ch);
    }
  write(fd, &ch, 1);
}

void
serial_write_block (unsigned char *ch, int len)
{
  while (len--)
    serial_write(*ch++);
  tcdrain(fd);
}

void
serial_write_string (char *ch)
{
  while (*ch)
    serial_write(*ch++);
  tcdrain(fd);
}

void
serial_drain ()
{
  int old_timeout = timeout;
  timeout = 1;
  while (serial_read() != -1)
    ;
  timeout = old_timeout;
}

int
serial_read (void)
{
  unsigned char ch;
  if (timeout)
    {
      fd_set rd;
      struct timeval tv;
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = (timeout % 1000) * 1000;
      FD_ZERO(&rd);
      FD_SET(fd, &rd);
      if (select(fd+1, &rd, 0, 0, &tv) == 0)
	return -1;
    }
  int r = read(fd, &ch, 1);
  if (r == 1)
    {
       if (verbose > 1)
	{
	  if (isgraph(ch))
	    printf("\033[31m%c\033[0m", ch);
	  else
	    printf("\033[35m%02x\033[0m", ch);
	  fflush(stdout);
	}
      return ch;
    }
  return -1;
}

int
serial_ready ()
{
  fd_set rd;
  struct timeval tv;

  tv.tv_sec = 0;
  tv.tv_usec = 0;
  FD_ZERO(&rd);
  FD_SET(fd, &rd);
  if (select(fd+1, &rd, 0, 0, &tv) == 1)
    return 1;
  return 0;
}

int
serial_setup_console ()
{
  if (rx_stick)
    set_stick_mode (0);
  return 1;
}

void
serial_reset ()
{
  if (rx_stick)
    set_stick_mode (in_flash_mode);
}
