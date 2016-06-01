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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mflash.h"
#include "elf.h"

int big_endian;

static mcu_addr_t
fetch_word (byte *b)
{
  if (big_endian)
    return (b[0] << 8) | b[1];
  else
    return (b[1] << 8) | b[0];
}
#define FW(x) fetch_word (x)

static mcu_addr_t
fetch_dword (byte *b)
{
  if (big_endian)
    return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
  else
    return (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];
}
#define FD(x) fetch_dword (x)

int
read_elf_file (FILE *prog)
{
  Elf32_External_Ehdr ehdr;
  Elf32_External_Phdr *phdr;
  byte *buf;
  int i;

  fseek (prog, 0, SEEK_SET);
  fread (&ehdr, sizeof(ehdr), 1, prog);

  /* 32-bit ELF */
  if (memcmp (ehdr.e_ident, "\177ELF\001", 5))
    return 1;

  if (ehdr.e_ident[5] == 2)
    big_endian = 1;
  else
    big_endian = 0;

  fseek (prog, FD(ehdr.e_phoff), SEEK_SET);

  phdr = (Elf32_External_Phdr *) malloc (FW(ehdr.e_phnum) * sizeof(Elf32_External_Phdr));

  fread (phdr, sizeof(Elf32_External_Phdr), FW(ehdr.e_phnum), prog);

  if (verbose)
    section_info (0, 0, 0, 0, 0);

  for (i=0; i<FW(ehdr.e_phnum); i++)
    {
      mcu_addr_t base = FD(phdr[i].p_paddr);
      mcu_addr_t vma = FD(phdr[i].p_vaddr);
      mcu_addr_t size = FD(phdr[i].p_filesz);

      if (FD(phdr[i].p_type) != PT_LOAD)
	continue;

      section_info ("load", vma, base, size, "");

      buf = (byte *) malloc (size);

      fseek (prog, FD(phdr[i].p_offset), SEEK_SET);
      fread (buf, 1, size, prog);

      progmem_puts (base, buf, size);
      free (buf);
    }

  return 0;
}
