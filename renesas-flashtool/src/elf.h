/* ELF support for BFD.  (Trimmed down by DJ)
   Copyright 1991, 1992, 1993, 1995, 1997, 1998, 1999, 2001, 2003, 2005,
   2008 Free Software Foundation, Inc.

   Written by Fred Fish @ Cygnus Support, from information published
   in "UNIX System V Release 4, Programmers Guide: ANSI C and
   Programming Support Tools".

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */


/* This file is part of ELF support for BFD, and contains the portions
   that describe how ELF is represented externally by the BFD library.
   I.E. it describes the in-file representation of ELF.  It requires
   the elf/common.h file which contains the portions that are common to
   both the internal and external representations. */

/* The 64-bit stuff is kind of random.  Perhaps someone will publish a
   spec someday.  */

#ifndef _ELF_EXTERNAL_H
#define _ELF_EXTERNAL_H

/* ELF Header (32-bit implementations) */

#define UC byte
#define ATTRIBUTE_PACKED __attribute__((packed));

typedef struct {
  UC e_ident[16];	/* ELF "magic number" */
  UC e_type[2];		/* Identifies object file type */
  UC e_machine[2];	/* Specifies required architecture */
  UC e_version[4];	/* Identifies object file version */
  UC e_entry[4];	/* Entry point virtual address */
  UC e_phoff[4];	/* Program header table file offset */
  UC e_shoff[4];	/* Section header table file offset */
  UC e_flags[4];	/* Processor-specific flags */
  UC e_ehsize[2];	/* ELF header size in bytes */
  UC e_phentsize[2];	/* Program header table entry size */
  UC e_phnum[2];	/* Program header table entry count */
  UC e_shentsize[2];	/* Section header table entry size */
  UC e_shnum[2];	/* Section header table entry count */
  UC e_shstrndx[2];	/* Section header string table index */
} Elf32_External_Ehdr;


/* Program header */

typedef struct {
  UC p_type[4];		/* Identifies program segment type */
  UC p_offset[4];	/* Segment file offset */
  UC p_vaddr[4];	/* Segment virtual address */
  UC p_paddr[4];	/* Segment physical address */
  UC p_filesz[4];	/* Segment size in file */
  UC p_memsz[4];	/* Segment size in memory */
  UC p_flags[4];	/* Segment flags */
  UC p_align[4];	/* Segment alignment, file & memory */
} Elf32_External_Phdr;

#undef UC

#define PT_NULL		0		/* Program header table entry unused */
#define PT_LOAD		1		/* Loadable program segment */
#define PT_DYNAMIC	2		/* Dynamic linking information */
#define PT_INTERP	3		/* Program interpreter */
#define PT_NOTE		4		/* Auxiliary information */
#define PT_SHLIB	5		/* Reserved, unspecified semantics */
#define PT_PHDR		6		/* Entry for header table itself */
#define PT_TLS		7		/* Thread local storage segment */
#define PT_LOOS		0x60000000	/* OS-specific */
#define PT_HIOS		0x6fffffff	/* OS-specific */
#define PT_LOPROC	0x70000000	/* Processor-specific */
#define PT_HIPROC	0x7FFFFFFF	/* Processor-specific */

#define EM_RX		173	/* Renesas RX family */

#define PF_X		(1 << 0)	/* Segment is executable */
#define PF_W		(1 << 1)	/* Segment is writable */
#define PF_R		(1 << 2)	/* Segment is readable */

#endif /* _ELF_EXTERNAL_H */
