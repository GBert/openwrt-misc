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

#ifndef __MFLASH_H__
#define __MFLASH_H__

typedef unsigned char byte;
typedef unsigned long mcu_addr_t;

typedef enum {
  UNKNOWN_CHIP,
  R8C_CHIP,
  M16C_CHIP,
  M32C_CHIP,
  R32C_CHIP
} ChipType;

extern ChipType chip_type;
extern int verbose;

void progmem_putc (mcu_addr_t addr, byte v);
void progmem_puts (mcu_addr_t addr, byte *vp, int len);
byte progmem_getc (mcu_addr_t addr);
int progmem_present (mcu_addr_t addr);
int region_used (mcu_addr_t a1, mcu_addr_t a2);

void section_info (char *action, int vma, int base, int size, char *name);

/* Return ZERO if read, NONZERO if error.  */
int read_elf_file (FILE *prog);
int read_srec_file (FILE *prog);
int read_ieee695_file (FILE *prog);

#endif
