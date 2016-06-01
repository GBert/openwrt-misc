/*
  Copyright (C) 2010 DJ Delorie <dj@redhat.com>

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
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>
#ifdef WIN32
#include <windows.h>
#define pthread_t HANDLE
#else
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#endif


#include "serial.h"
#include "console.h"

#ifndef _WIN32
static struct termios attr, oattr;
#endif

static int console_timeout_val = -1;

void
console_timeout (int seconds)
{
  console_timeout_val = seconds;
}

static void *
console2 (void *x __attribute__((unused)))
{
  int c;
  int ctrl_c_count = 0;

  serial_set_timeout (-1);
  setbuf (stdout, 0);
  fflush(stdout);

  while (1)
    {
      c = serial_read ();
      if (c == 3)
	{
	  if (++ctrl_c_count == 2)
	    {
	      int rv = serial_read ();
#ifndef _WIN32
	      if (isatty (0))
		tcsetattr(0, TCSANOW, &oattr);
#endif
	      exit (rv);
	    }
	}
      else
	ctrl_c_count = 0;

      if (c == 6)
	{
	  struct timeval tv;
	  gettimeofday (&tv, NULL);
	  printf("[%ld.%03ld]", tv.tv_sec % 1000, tv.tv_usec / 1000);
	}
      else
	write (1, &c, 1);
    }
}

static void
console_cleanup ()
{
#ifndef _WIN32
  tcsetattr(0, TCSANOW, &oattr);
#endif
}

static void
console_alarm (int n)
{
  printf("timeout! (%d seconds)\n", console_timeout_val);
  fprintf(stderr, "timeout! (%d seconds)\n", console_timeout_val);
  exit (1);
}

void
do_console (int use_cnvss, int baud)
{
  pthread_t tid;
  char c;
  int ctrl_c = 0;

#ifndef _WIN32
  if (console_timeout_val > 0)
    {
      alarm (console_timeout_val);
      signal (SIGALRM, console_alarm);
    }
#endif

  if (use_cnvss != -1)
    serial_use_cnvss (use_cnvss);
  serial_change_baud (baud);
  if (! serial_setup_console ())
    {
      fprintf(stderr, "no console support for this board\n");
      return;
    }

  setbuf (stdout, NULL);

#ifdef WIN32
  tid = CreateThread(NULL, 0, console2, NULL, 0, NULL);
#else
  pthread_create (&tid, 0, console2, 0);
#endif

#ifndef _WIN32
  if (isatty (0))
    {
      tcgetattr(0, &attr);
      tcgetattr(0, &oattr);
#ifndef __CYGWIN__
      cfmakeraw (&attr);
#endif
      tcsetattr(0, TCSANOW, &attr);
      atexit (console_cleanup);
    }
#endif

  while (read (0, &c, 1) == 1)
    {
      if (c == 3)
	{
	  ctrl_c ++;
	  if (ctrl_c == 2)
	    break;
	}
      else
	ctrl_c = 0;
       serial_write (c);
    }

#ifdef WIN32
  TerminateThread(tid, 0);
#else
  pthread_cancel(tid);
#endif
#ifndef _WIN32
  if (isatty (0))
    tcsetattr(0, TCSANOW, &oattr);
#endif
}
