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
#include <getopt.h>
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

#include "mflash.h"
#include "serial.h"
#include "rxlib.h"
#include "console.h"

ChipType chip_type = UNKNOWN_CHIP;

int verbose = 0;
int user_verbose = 0;
int readback = 0;
int just_query = 0;
int test_connection = 0;
int console = 0;
int save_data_flash = 0;
int data_flash_start = 0, data_flash_end = 0;
BoardTypes board_type = BT_rx;
static int rxusb_mode = 0;

static struct {
  const char *name;
  BoardTypes type;
} board_type_map[] = {
  { "gpio", BT_unknown },
  { "pm", BT_powermeter },
  { "pmp", BT_powermeterproto },
  { "powermeter", BT_powermeter },
  { "rx", BT_rx },
  { "rx600", BT_rx600 },
  { "rx610", BT_rx },
};
#define BTM_LEN (sizeof(board_type_map)/sizeof(board_type_map[0]))

//-----------------------------------------------------------------------------

#define HIGH(a) (((a) >> 24) & 0xff)
#define MIDH(a) (((a) >> 16) & 0xff)
#define MIDL(a) (((a) >>  8) & 0xff)
#define LOW(a)  (((a) >>  0) & 0xff)

// byte    progmem[aa][bb][cc][dd] = 0xaabbccdd
// byte   *progmem[aa][bb][cc] = 0xaabb..
// byte  **progmem[aa][bb] = 0xaa....
// byte ***progmem[aa] = 0xaa....
static byte ***progmem[256];

void
progmem_putc (mcu_addr_t addr, byte v)
{
  int a = HIGH(addr);
  int b = MIDH(addr);
  int c = MIDL(addr);
  int d = LOW(addr);

  if (! progmem[a])
    progmem[a] = (byte ***) calloc (256, sizeof (byte **));
  if (! progmem[a][b])
    progmem[a][b] = (byte **) calloc (256, sizeof (byte *));
  if (! progmem[a][b][c])
    {
      int i;
      progmem[a][b][c] = (byte *) malloc (256 * sizeof (byte));
      memset (progmem[a][b][c], 0xff, 256);
      for (i=0; i<256; i++)
	progmem[a][b][c][i] = i;
    }
  progmem[a][b][c][d] = v;
}

void
progmem_puts (mcu_addr_t addr, byte *vp, int len)
{
  while (len > 0)
    {
      progmem_putc (addr, *vp);
      addr++;
      vp++;
      len--;
    }
}

/* from read_elf.c */
extern int big_endian;

void
progmem_putl (mcu_addr_t addr, int val)
{
  if (big_endian)
    {
      progmem_putc (addr+0, val >> 24);
      progmem_putc (addr+1, val >> 16);
      progmem_putc (addr+2, val >> 8);
      progmem_putc (addr+3, val);
    }
  else
    {
      progmem_putc (addr+3, val >> 24);
      progmem_putc (addr+2, val >> 16);
      progmem_putc (addr+1, val >> 8);
      progmem_putc (addr+0, val);
    }
}

byte
progmem_getc (mcu_addr_t addr)
{
  int a = HIGH(addr);
  int b = MIDH(addr);
  int c = MIDL(addr);
  int d = LOW(addr);

  if (! progmem[a])
    return 0;
  if (! progmem[a][b])
    return 0;
  if (! progmem[a][b][c])
    return 0;
  return progmem[a][b][c][d];
}

void
progmem_gets (mcu_addr_t addr, byte *vp, int len)
{
  while (len > 0)
    {
      *vp = progmem_getc (addr);
      addr++;
      vp++;
      len--;
    }
}

int
progmem_present (mcu_addr_t addr)
{
  int a = HIGH(addr);
  int b = MIDH(addr);
  int c = MIDL(addr);

  if (! progmem[a])
    return 0;
  if (! progmem[a][b])
    return 0;
  if (! progmem[a][b][c])
    return 0;
  return 1;
}

#define PROGCOLMASK 0x0f

int
region_used (mcu_addr_t a1, mcu_addr_t a2)
{
  mcu_addr_t d;
  for (d=a1; d<=a2; d += 256)
    if (progmem_present (d))
      return 1;
  return 0;
}

//-----------------------------------------------------------------------------

#define BAUD_RATE 9600

#define BAD { printf("Bad byte?\n"); reset_me (); return; }

static void
read_part_number ()
{
  int nparts, i;
  char **codes;
  char **products;

  rxlib_inquire_device (&nparts, &codes, &products);
  if (verbose > 1 || test_connection)
    for (i=0; i<nparts; i++)
      fprintf(stderr, "Chip %d: code %s product %s\n",
	      i, codes[i], products[i]);

  rxlib_select_device (codes[0]);
}

static void
read_clock_modes ()
{
  int n, i;
  int *m;
  rxlib_inquire_clock_modes (&n, &m);
  for (i=0; i<n; i++)
    fprintf(stderr, "Clock mode[%d]: %02x\n", i, m[i]);
  rxlib_select_clock_mode (n ? m[0] : 0);
}

static int flash_map_size;
static int programming_size;
static RxMemRange *flash_map;

static void
read_program_memory_size ()
{
  int i, n;
  RxMemRange *a;

  rxlib_inquire_user_flash_size (&n, &a);

  if (verbose > 1 || test_connection)
    for (i=0; i<n; i++)
      fprintf(stderr, "ProgMem[%02d]:   0x%08lx - 0x%08lx, size 0x%08lx (%ld kb)\n",
	      i, a[i].start, a[i].end, a[i].size, a[i].size/1024);
}

static void
read_userboot_memory_size ()
{
  int i, n;
  RxMemRange *a;

  rxlib_inquire_boot_flash_size (&n, &a);

  if (verbose > 1 || test_connection)
    for (i=0; i<n; i++)
      fprintf(stderr, "UBootMem[%02d]:  0x%08lx - 0x%08lx, size 0x%08lx (%ld kb)\n",
	      i, a[i].start, a[i].end, a[i].size, a[i].size/1024);
}

static void
read_dataflash_memory_size ()
{
  int i, n;
  RxMemRange *a;

  rxlib_inquire_data_flash_size (&n, &a);

  if (verbose > 1 || test_connection)
    for (i=0; i<n; i++)
      fprintf(stderr, "DataFlash[%02d]: 0x%08lx - 0x%08lx, size 0x%08lx (%ld kb)\n",
	      i, a[i].start, a[i].end, a[i].size, a[i].size/1024);
}

static void
read_flash_map ()
{
  int i;

  rxlib_inquire_flash_erase_blocks (&flash_map_size, &flash_map);

  if (verbose > 2)
    for (i=0; i<flash_map_size; i++)
      fprintf(stderr, "FlashPage[%02d]: 0x%08lx - 0x%08lx, size 0x%08lx\n",
	      i, flash_map[i].start, flash_map[i].end, flash_map[i].size);
}

static void
show_used_memory_map ()
{
  int a, b, c, d;

  if (verbose <= 1)
    return;
  for (a=0; a<256; a++)
    {
      if (!progmem[a])
	continue;
      for (b=0; b<256; b++)
	{
	  if (!progmem[a][b])
	    continue;
	  for (c=0; c<256; c += PROGCOLMASK+1)
	    if (region_used ((a<<24)+b*65536+c*256, (a<<24)+b*65536+(c+PROGCOLMASK)*256))
	      {
		fprintf(stderr, "0x%02x%02x---- ", a, b);
		for (d=0; d<PROGCOLMASK+1; d++)
		  {
		    if (progmem[a][b][c+d])
		      fprintf(stderr, " %02x", c+d);
		    else
		      fprintf(stderr, " --");
		  }
		fprintf(stderr, "\n");
	      }
	}
    }
}

void
section_info (char *action, int vma, int base, int size, char *name)
{
  /* These calls are just for detecting the chip type.  */
  if (size && !action)
    return;

  if (!verbose)
    return;

  if (action)
    fprintf (stderr, "[%s %8x %8x-%8x %8x-%8x %s]\n",
	     action, (int) size,
	     (int) vma, (int)(vma+size-1),
	     (int) base, (int)(base+size-1), name);
  else
    //                [load 12345678 12345678-12345678 12345678-12345678 name
    fprintf (stderr, "      --size-- -------vma------- -------lma-------\n");
}

static unsigned char *pbuf = NULL;
static unsigned char *rbuf = NULL;

static void
program_one_area (unsigned char *pbuf, unsigned long addr)
{
  int i, j;
  int boot_wait;

  if (rxlib_status (&boot_wait, 0) != RX_ERR_NONE)
    exit (1);
  if (boot_wait != RX_BWAIT_PROGRAMMING_SELECTION
      && boot_wait != RX_BWAIT_PROGRAMMING_DATA)
    exit (1);

  if (readback)
    {
      if (rxlib_read_memory (addr, programming_size, rbuf) != RX_ERR_NONE)
	{
	  printf("can't read memory at 0x%08lx\n", addr);
	  exit (1);
	}
      for (i=0; i<programming_size; i++)
	if (rbuf[i] != 0xff)
	  {
	    printf("unerased byte at 0x%08lx: 0x%02x\n", addr+i, rbuf[i]);
	    exit (1);
	  }
    }

  for (i=0; i<3; i++)
    {
      rxlib_begin_user_flash ();
      j = rxlib_program_block (addr, pbuf);
      rxlib_done_programming ();
      if (j != RX_ERR_NONE)
	goto try_again;

      if (readback)
	{
	  if (rxlib_read_memory (addr, programming_size, rbuf) != RX_ERR_NONE)
	    {
	      printf("can't read memory at 0x%08lx\n", addr);
	      exit (1);
	    }
	  for (i=0; i<programming_size; i++)
	    if (rbuf[i] != pbuf[i])
	      {
		printf("unprogrammed byte at 0x%08lx: 0x%02x vs 0x%02x\n",
		       addr+i, rbuf[i], pbuf[i]);
		goto try_again;
	      }
	}

      goto good_program;
    try_again:
      ;
    }
  printf("unable to program address 0x%08lx\n", addr);
  exit(1);
 good_program:
  ;
}

static void
program_all_areas ()
{
  int a, b, c;
  int total_pages = 0, pages_so_far = 0;
  unsigned long pbuf_addr;
  unsigned long addr_mask;
  int pbuf_dirty;

  addr_mask = ~ ((unsigned long)programming_size - 1);

  if (pbuf == NULL)
    {
      pbuf = (unsigned char *) malloc (programming_size);
      rbuf = (unsigned char *) malloc (programming_size);
    }
  pbuf_dirty = 0;

  for (a=0; a<256; a++)
    if (progmem[a])
      for (b=0; b<256; b++)
	if (progmem[a][b])
	  for (c=0; c<256; c++)
	    if (progmem[a][b][c])
	      total_pages ++;

  for (a=0; a<256; a++)
    if (progmem[a])
      for (b=0; b<256; b++)
	if (progmem[a][b])
	  for (c=0; c<256; c++)
	    if (progmem[a][b][c])
	      {
		unsigned long addr = ((unsigned)a<<24) | (b<<16) | (c<<8);

		/* Since we scan the pages in order, if we have N
		   pages in a block to program, we know we'll get
		   those in order.  Thus, if we've stored a page to be
		   programmed but we're now in a different block,
		   we've seen all the pages for the old block and can
		   write it out.  */

		if (verbose == 1)
		  {
		    pages_so_far ++;
		    fprintf(stderr, "  0x%08lx  %3d %%\r", addr, 100 * pages_so_far / total_pages);
		  }

		if (pbuf_dirty && (addr & addr_mask) != pbuf_addr)
		  {
		    program_one_area (pbuf, pbuf_addr);
		    pbuf_dirty = 0;
		  }

		pbuf_addr = (addr & addr_mask);
		memcpy (pbuf + (addr % programming_size), progmem[a][b][c], 256);
		pbuf_dirty = 1;
	      }

  if (pbuf_dirty)
    program_one_area (pbuf, pbuf_addr);

  if (verbose)
    {
      if (verbose > 1)
	printf("\n");
      fprintf(stderr, "all pages programmed and verified!\n");
    }
}

static void
my_exit ()
{
  exit (1);
}

int
main(int argc, char **argv)
{
  int o, i;
  int boot_status, error_status;
  int baud = 115200;
  char *port = NULL; /* The default, and syntax, is driver-dependent.  */
  int trace = 0;
  int use_cnvss = -1;
  char *key = 0;
  FILE *prog;
  char *me = argv[0];

  signal(SIGINT, my_exit);

  while ((o = getopt (argc, argv, "cuvtTrp:b:qCMk:B:F:S:P:")) != -1)
    switch (o)
      {
      case 'q':
	just_query = 1;
	break;
      case 'c':
	console = 1;
	break;
      case 'T':
	test_connection = 1;
	break;
      case 'v':
	verbose ++;
	break;
      case 'u':
	user_verbose ++;
	break;
      case 'p':
	port = optarg;
	break;
      case 'r':
	readback = 1;
	break;
      case 't':
	trace ++;
	break;
      case 'b':
	baud = atoi(optarg);
	break;
      case 'C':
	use_cnvss = 1;
	break;
      case 'M':
	use_cnvss = 0;
	break;
      case 'P':
	serial_param (optarg);
	if (strcmp (optarg, "rxusb") == 0)
	  rxusb_mode = 1;
	break;
      case 'F':
	save_data_flash = 1;
	sscanf (optarg, "%x%*c%x", &data_flash_start, &data_flash_end);
	data_flash_start = (data_flash_start + 0xff) & ~0xff;
	data_flash_end = (data_flash_end + 0xff) & ~0xff;
	printf("saving data flash: %x to %x, size %x\n", data_flash_start, data_flash_end, data_flash_end - data_flash_start);
	break;
      case 'k':
	key = optarg;
	break;
      case 'B':
	board_type = BT_unknown;
	for (i=0; i<BTM_LEN; i++)
	  if (strcasecmp (optarg, board_type_map[i].name) == 0)
	    board_type = board_type_map[i].type;
	if (board_type == BT_unknown)
	  {
	    printf("Unknown board type `%s'.  Known board types:\n", optarg);
	    for (i=0; i<BTM_LEN; i++)
	      printf("\t%s\n", board_type_map[i].name);
	    exit(1);
	  }
	break;
      case 'S':
	console_timeout (atoi (optarg));
	break;
      case '?':
	printf("usage: board-run [options] srec-file\n");
	printf("  -v            verbose (multiple times increases verbosity)\n");
	printf("  -q            just query the chip's version number\n");
	printf("  -c            start console after programmer (or without programming)\n");
	printf("  -T            just test the connection\n");
	printf("  -u            user-verbose (unused)\n");
	printf("  -p <port>     specify port (if needed)\n");
	printf("  -r            read back flash data to verify it\n");
	printf("  -t            trace mode (unused)\n");
	printf("  -b <baud>     specify baud rate (default 57600)\n");
	printf("  -C            CNVss chip (0=user, 1=program)\n");
	printf("  -M            MODE chip (1=user, 0=program)\n");
	printf("  -R            R32C mode (longer key)\n");
	printf("  -F start,end  portion of flash to save (hex addresses)\n");
	printf("  -k KEY        ID key, as XX.XX.XX.XX.XX.XX.XX (hex)\n");
	printf("  -B <board>    board type - pm, pmp, powermeter, gpio (default gpio)\n");
	printf("  -S <seconds>  set console mode timeout\n");
	exit(1);
      }

  if (verbose)
    setbuf(stdout, 0);


  if (argv[optind] && !rxusb_mode)
    {
      prog = fopen (argv[optind], "rb");
      if (!prog)
	{
	  fprintf (stderr, "Can't read %s\n", argv[optind]);
	  exit (1);
	}

      if (read_elf_file (prog))
	if (read_ieee695_file (prog))
	  if (read_srec_file (prog))
	    {
	      fprintf (stderr, "Unrecognized file format: %s\n", argv[optind]);
	      exit (1);
	    }
      show_used_memory_map ();
    }
  else
    prog = NULL;

  if (use_cnvss != -1)
    serial_use_cnvss (use_cnvss);
  else
    serial_use_cnvss (0);

  chip_type = BT_rx;

  serial_init (port, BAUD_RATE);

  if (argv[optind])
    {
#ifndef _WIN32
      if (rxusb_mode)
	{
	  char *pname, *slash;
	  char *av[10];
	  int ac = 0;
	  pid_t p;
	  int status;

	  slash = strrchr (me, '/');
	  if (slash)
	    {
	      pname = malloc (strlen(me) + 7);
	      strcpy (pname, me);
	      strcpy (pname + (slash-me) + 1, "rxusb");
	    }
	  else
	    pname = "rxusb";

	  av[ac++] = pname;
	  if (verbose)
	    av[ac++] = "-v";
	  if (verbose > 1)
	    av[ac++] = "-v";
	  av[ac++] = "-P";
	  av[ac++] = "wait";
	  av[ac++] = argv[optind];
	  av[ac] = 0;

	  p = fork ();
	  switch (p)
	    {
	    case -1:
	      perror("fork");
	      exit (1);

	    case 0:
	      execvp (av[0], av);
	      perror (av[0]);
	      exit(1);

	    default:
	      wait (&status);
	      if (status)
		exit (1);
	      break;
	    }
	}
      else
#endif
	{
	  rxlib_init ();

	  rxlib_reset ();

	  if (verbose || test_connection)
	    {
	      rxlib_inquire_operating_frequency ();
	      read_program_memory_size ();
	      read_userboot_memory_size ();
	      read_dataflash_memory_size ();
	      read_flash_map ();
	    }

	  rxlib_inquire_programming_size (&programming_size);
	  if (programming_size < 256)
	    {
	      printf("Um, I don't know how to deal with programming units less than 256 byte pages.\n");
	      printf("(the chip wants %d byte pages\n", programming_size);
	      exit(1);
	    }

	  read_part_number ();
	  read_clock_modes ();
	  rxlib_set_baud_rate (baud);
	  rxlib_data_setting_complete ();
	  if (verbose > 1)
	    printf("blank check: user %d boot %d\n",
		   rxlib_blank_check_user (),
		   rxlib_blank_check_boot ());
	  if (rxlib_blank_check_user())
	    {
	      printf("chip not automatically erased!\n");
	      exit (1);
	    }

	  if (test_connection)
	    exit (0);

	  program_all_areas ();

	  rxlib_status (&boot_status, &error_status);
	}
    }

  if (console)
    do_console (0, baud);
  else
    serial_setup_console ();

  exit(0);
}
