/*
  Copyright (C) 2009 DJ Delorie <dj@redhat.com>

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

/* WARNING: This is a very trivial implementation of the IEEE-695
   object format specification, intended to be just enough to read a
   fully linked absolute binary image so that we can flash a chip.
   For a more complete implementation, see the file bfd/ieee.c in the
   binutils source distribution.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <setjmp.h>
#ifndef __WIN32__
#include <unistd.h>
#endif

#include "mflash.h"
#include "ieee695.h"

static jmp_buf ieee_failure;
#define FAIL longjmp(ieee_failure, 1);

typedef struct {
  int load_me;
  char *name;
  unsigned addr;
  unsigned len;
  unsigned pc;
} Section;

static FILE *prog;
static byte *data;
static byte *d;

static int bits_per_byte;
static int bytes_per_addr;
static ieee_w_variable_type w;

static int max_sections, num_sections;
static Section *sections;

static int
parse_int ()
{
  if (*d >= 0x80 && *d <= 0x88)
    {
      int i, l;
      l = *d++ & 0x0f;
      i = 0;
      while (l)
	{
	  i <<= 8;
	  i |= *d++;
	  l--;
	}
      return i;
    }
  return *d++;
}

static char *
parse_string ()
{
  int length;
  char *rv;

  if (*d == 0xde)
    {
      length = *++d;
    }
  else if (*d == 0xdf)
    {
      length = *++d;
      length = length * 256 + *++d;
    }
  else
    length = *d;
  d++;

  rv = (char *) malloc (length + 1);
  memcpy (rv, d, length);
  rv[length] = 0;
  d += length;
  return rv;
}

static int
parse_expression ()
{
  if (*d > 0x88)
    {
      printf("expressions not supported, at file offset %d\n", d - data);
      FAIL;
    }
  return parse_int ();
}

static int
dword2 ()
{
  int rv;

  rv = *d++;
  rv = rv * 256 + *d++;
  return rv;
}

static Section *
get_section (int secno)
{
  int i;
  if (secno >= max_sections)
    {
      i = max_sections;
      max_sections = secno + 10;
      sections = (Section *) realloc (sections, max_sections * sizeof (Section));
      memset (sections+i, 0, (max_sections-i) * sizeof(Section));
    }
  if (secno >= num_sections)
    num_sections = secno + 1;
  return sections + secno;
}

static void
read_sections ()
{
  int secno;
  Section *sec;

  max_sections = 0;
  num_sections = 0;
  sections = (Section *) malloc (sizeof (Section));

  d = data + w.r.section_part;
  while (1)
    {
      switch (*d++)
	{
	case ieee_section_type_enum:
	  secno = parse_int ();
	  sec = get_section (secno);

	  switch (*d)
	    {
	    case 0xC1:
	      switch (*++d)
		{
		case 0xD3:
		  switch (*++d)
		    {
		    case 0xD0:
		    case 0xD2:
		      d++;
		      sec->load_me = 1;
		      break;
		    case 0xC4:
		      d++;
		      break;
		    }
		  break;
		}
	      break;
	    case 0xC3:
	      switch (*++d)
		{
		case 0xD0:
		case 0xD2:
		  d++;
		  sec->load_me = 1;
		  break;
		case 0xC4:
		  d++;
		  break;
		}
	      break;
	    }

	  sec->name = parse_string ();
	  parse_int ();
	  parse_int ();
	  parse_int ();
	  break;

	case ieee_section_alignment_enum:
	  parse_int ();
	  parse_int ();
	  parse_int ();
	  break;

	case ieee_e2_first_byte_enum:
	  d--;
	  switch (dword2 ())
	    {
	    case ieee_section_size_enum:
	    case ieee_physical_region_size_enum:
	      sec = get_section (parse_int ());
	      sec->len = parse_int ();
	      break;
	    case ieee_region_base_address_enum:
	    case ieee_section_base_address_enum:
	      sec = get_section (parse_int ());
	      sec->addr = parse_int();
	      break;
	    case ieee_mau_size_enum:
	    case ieee_m_value_enum:
	    case ieee_section_offset_enum:
	      parse_int ();
	      parse_int ();
	      break;
	    }
	  break;

	default:
	  return;
	}
    }
}

static void
read_some_data (Section *sec)
{
  int i;

  switch (*d++)
    {
    case ieee_load_constant_bytes_enum:
      i = parse_int ();
      progmem_puts (sec->pc, d, i);
      sec->pc += i;
      d += i;
      break;
    case ieee_load_with_relocation_enum:
      printf("Relocations not supported\n");
      FAIL;
    }
}

static void
read_section_data ()
{
  int i;
  Section *sec;
  byte *save_d;

  d = data + w.r.data_part;

  while (1)
    {
      switch (*d++)
	{
	case ieee_set_current_section_enum:
	  sec = get_section (parse_int ());
	  sec->pc = sec->addr;
	  break;

	  case ieee_e2_first_byte_enum:
	    d--;
	    switch (dword2 ())
	      {
	      case ieee_set_current_pc_enum:
		parse_int ();
		sec->pc = parse_expression ();
		break;
	      case ieee_value_starting_address_enum:
		return;
	      }
	    break;

	case ieee_repeat_data_enum:
	  i = parse_int ();
	  save_d = d;
	  while (i --)
	    {
	      d = save_d;
	      read_some_data (sec);
	    }
	  break;

	case ieee_load_constant_bytes_enum:
	case ieee_load_with_relocation_enum:
	  d--;
	  read_some_data (sec);
	  break;

	default:
	  return;
	}
    }
}

int
read_ieee695_file (FILE *_prog)
{
  int magic;
  int file_length;
  char *chip;
  char *module;
  int varno;
  int i;

  prog = _prog;

  if (setjmp (ieee_failure))
    return 1;

  fseek (prog, 0, SEEK_SET);
  magic = fgetc (prog);
  if (magic != Module_Beginning)
    return 1;

  fseek (prog, 0, SEEK_END);
  file_length = ftell (prog);
  fseek (prog, 0, SEEK_SET);

  data = (byte *) malloc (file_length);
  d = data + 1;

  fread (data, 1, file_length, prog);

  /* At this point, we're all set up to parse the file.  */

  chip = parse_string ();
  module = parse_string ();
  if (verbose)
    printf("IEEE695: Chip \033[32m%s\033[0m module \033[34m%s\033[0m\n", chip, module);

  if (*d++ != ieee_address_descriptor_enum)
    return 1;

  bits_per_byte = parse_int ();
  bytes_per_addr = parse_int ();

  /* Endian */
  if (*d == ieee_variable_L_enum
      || *d == ieee_variable_M_enum)
    d++;

  for (varno = 0; varno < N_W_VARIABLES; varno++)
    {
      if (dword2 () != ieee_assign_value_to_variable_enum)
	return 1;
      if (*d++ != varno)
	return 1;
      w.offset[varno] = parse_int ();
    }

  if (w.r.section_part == 0)
    return 1;
  if (w.r.data_part == 0)
    return 1;

  read_sections ();
  read_section_data ();

  section_info (0, 0, 0, 0, 0);
  for (i=0; i<num_sections; i++)
    if (sections[i].len > 0 && sections[i].load_me)
      {
	section_info ("load", sections[i].addr, sections[i].addr, sections[i].len, sections[i].name);
	if (verbose > 2)
	  {
	    int a, j;
	    for (a=sections[i].addr; a!=sections[i].addr+sections[i].len; a++)
	      {
		if ((a & 0x1f) == 0 || a == sections[i].addr)
		  printf("\033[32m%08x:\033[33m", a);
		if (a == sections[i].addr)
		  for (j=0; j < (a & 0x1f); j++)
		    printf("   ");
		printf(" %02x", progmem_getc (a));
		if ((a & 0x1f) == 0x1f || a == sections[i].addr + sections[i].len - 1)
		  printf("\033[0m\n");
	      }
	  }
      }

  return 0;
}
