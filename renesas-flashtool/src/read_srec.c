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

#include "mflash.h"

static char line[600];

static unsigned int
unhex(char *ptr, int digits)
{
  unsigned int rv = 0;
  while (digits--)
    {
      char c = *ptr++;
      rv <<= 4;
      if ('0' <= c && c <= '9')
	rv += c - '0';
      else if ('A' <= c && c <= 'F')
	rv += c - 'A' + 10;
      else if ('a' <= c && c <= 'f')
	rv += c - 'a' + 10;
    }
  return rv;
}

static void
dataline(int count, int abytes)
{
  unsigned int addr = unhex(line+4, abytes*2);
  int i;
  char *cp;

  count -= abytes + 1;
  cp = line + 4 + abytes * 2;
  for (i=0; i<count; i++)
    progmem_putc (addr+i, unhex(cp+i*2, 2));

  section_info (0, addr, addr, count, 0);
}

int
read_srec_file (FILE *prog)
{
  fseek (prog, 0, SEEK_SET);

  while (fgets(line, 600, prog) != NULL)
    {
      char type;
      unsigned int count;

      if (line[0] != 'S')
	return 1;
      type = line[1];
      count = unhex(line+2, 2);
      switch (type)
	{
	case '0':
	  break;
	case '1':
	  dataline(count, 2);
	  break;
	case '2':
	  dataline(count, 3);
	  break;
	case '3':
	  dataline(count, 4);
	  break;
	}
    }

  return 0;
}
