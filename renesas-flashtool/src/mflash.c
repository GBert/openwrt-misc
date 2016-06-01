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
#include <unistd.h>
#endif

#include "mflash.h"
#include "serial.h"
#include "console.h"

static int tty;

ChipType chip_type = UNKNOWN_CHIP;

int trace = 0;
#define tprintf if(trace)printf

int verbose = 0;
int user_verbose = 0;
int readback = 0;
int just_query = 0;
int console = 0;
int save_data_flash = 0;
int data_flash_start = 0, data_flash_end = 0;
BoardTypes board_type = BT_unknown;

static struct {
  const char *name;
  BoardTypes type;
} board_type_map[] = {
  { "gr8c", BT_unknown },
  { "gpio", BT_unknown },
  { "pm", BT_powermeter },
  { "pmp", BT_powermeterproto },
  { "powermeter", BT_powermeter },
  { "m32c10p", BT_m32c10p },
  { "m32c4m", BT_m32c10p },
  { "m32c", BT_m32c10p }
};
#define BTM_LEN (sizeof(board_type_map)/sizeof(board_type_map[0]))

static int id_d;

#define BAUD_RATE 9600

//-----------------------------------------------------------------------------

static int serial_boot_mode = 2;
static int mode3_chars = 0;

static int
my_serial_read (void)
{
  while (mode3_chars)
    {
      if (serial_read () != -1)
	mode3_chars --;
    }
  return serial_read ();
}

static int
my_serial_ready (void)
{
  while (mode3_chars)
    {
      if (!serial_ready ())
	return 0;
      if (serial_read () != -1)
	mode3_chars --;
    }
  return serial_ready ();
}

static void
my_serial_write (unsigned char ch)
{
  if (serial_boot_mode == 3)
    mode3_chars ++;
  serial_write (ch);
}

static void
my_serial_write_block (unsigned char *ch, int len)
{
  if (serial_boot_mode == 3)
    mode3_chars += len;
  serial_write_block (ch, len);
}

#define serial_read my_serial_read
#define serial_ready my_serial_ready
#define serial_write my_serial_write
#define serial_write_block my_serial_write_block

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
    progmem[a] = (byte ***)calloc (256, sizeof (byte **));
  if (! progmem[a][b])
    progmem[a][b] = (byte **)calloc (256, sizeof (byte *));
  if (! progmem[a][b][c])
    progmem[a][b][c] = (byte *)calloc (256, sizeof (byte));
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

static void
addr_msb (int msb)
{
  if (chip_type == R32C_CHIP)
    {
      serial_write (0x48);
      serial_write (msb);
    }
}

void
check_status (int verbose)
{
  static char *id_string[] = { "\033[31mNot ID'd",
			       "\033[33mID not matched",
			       "Res",
			       "\033[32mID'd" };
  int srd0, srd1;
  int count = 30;
  int lcount = 2;

  tprintf("checking status\n");

  while (1)
    {
      serial_write (0x70);
      srd0 = serial_read ();
      if (srd0 == -1)
	{
	  if (count--)
	    goto try_again;
	  exit (1);
	}
      srd1 = serial_read ();
      if (srd1 == -1)
	{
	  if (count--)
	    goto try_again;
	  exit (1);
	}
      id_d = (srd1>>2) & 0x03;

      /* Don't bother printing anything if we're in a known-good
	 state, even if verbose.  */
      if ((srd0 & 0x83) == 0x80
	  && id_d == 3)
	break;

      if (verbose)
	{
	  if (tty)
	    {
	      printf("Status: Seq: %s\033[0m  Erase: %s\033[0m  Program: %s\033[0m  ID: %s\033[0m\n",
		     srd0 & 0x80 ? "\033[32mReady" : "\033[34mBusy",
		     srd0 & 0x20 ? "\033[31mError" : "\033[32mNormal",
		     srd0 & 0x10 ? "\033[31mError" : "\033[32mNormal",
		     id_string[id_d]);
	    }
	  else
	    {
	      printf("Status: Seq: %s  Erase: %s  Program: %s  ID: %s\n",
		     srd0 & 0x80 ? "Ready" : "Busy",
		     srd0 & 0x20 ? "Error" : "Normal",
		     srd0 & 0x10 ? "Error" : "Normal",
		     id_string[id_d] + 5);
	    }
	}
      if (srd0 & 0x80)
	break;
      if (--lcount == 0)
	{
	  printf("Timed out.\n");
	  exit (1);
	}
    try_again:
      serial_pause(100);
      serial_drain ();
    };
  tprintf("done checking status\n");
}

void
clear_status()
{
  tprintf("clear status\n");
  serial_write (0x50);
}

void
check_id ()
{
  printf("ID needed\n");
  exit (0);
}

int
read_page (int page, unsigned char *buf)
{
  int i;
  int tries = 3;

  tprintf("read page %06x--\n", page);
  while (tries --)
    {
      addr_msb (page >> 16);
      serial_write (0xff);
      serial_write (page % 256);
      serial_write (page / 256);
      for (i=0; i<256; i++)
	{
	  int c = serial_read ();
	  if (c == -1)
	    goto try_again;
	  buf[i] = c;
	}
      return 0;
    try_again:
      ;
    }
  return -1;
}

void
program_page (int page, unsigned char *bytes)
{
  unsigned char buf[256];
  int i, j, tries=0;

  tprintf("program page %06x--\n", page);
  serial_drain ();
  if (tty)
    printf("\r%06x", page*256);
  else
    {
      switch (chip_type)
	{
	case UNKNOWN_CHIP:
	case R8C_CHIP:
	  printf(" %02x", page);
	  break;
	case M16C_CHIP:
	  printf(" %03x", page);
	  break;
	case M32C_CHIP:
	  printf(" %04x", page);
	  break;
	case R32C_CHIP:
	  printf(" %06x", page);
	  break;
	}
    }
  fflush(stdout);
  while (tries < 3)
    {
      clear_status ();
      addr_msb (page >> 16);
      serial_write (0x41);
      serial_write (page % 256);
      serial_write (page / 256);
      serial_write_block (bytes, 256);
      serial_sync ();
      check_status (0);

      if (!readback)
	break;

      if (read_page (page, buf))
	goto page_failed;

      if (memcmp (buf, bytes, 256) == 0)
	break;

      printf("\r%06x: compare mismatch, trial %d\n", page*256, tries);
      for (i=0; i<256; i+=16)
	if (memcmp (buf+i, bytes+i, 16))
	  {
	    printf("\033[32mW %06x:", i);
	    for (j=0; j<16; j++)
	      printf(" %02x", bytes[i+j]);
	    printf("\n");
	    printf("\033[31mR %06x:", i);
	    for (j=0; j<16; j++)
	      printf(" %02x", buf[i+j]);
	    printf("\033[0m\n");
	    break;
	  }

    page_failed:
      tries ++;
    }
  if (tries == 3)
    exit(1);
}

static int
try_unlocking (unsigned char *key)
{
  unsigned char cmd[12] = { 0xf5, 0xdf, 0xff, 0, 0x07 };

  tprintf("try unlocking...\n");
  clear_status ();
  switch (chip_type) {
  case UNKNOWN_CHIP:
  case R8C_CHIP:
    cmd[3] = 0;
    break;
  case M16C_CHIP:
    cmd[3] = 0x0f;
    break;
  case M32C_CHIP:
    cmd[3] = 0xff;
    break;
  case R32C_CHIP:
    addr_msb (0xff);
    cmd[1] = 0xe8;
    cmd[3] = 0xff;
    break;
  }
  memcpy (cmd+5, key, 7);
  serial_write_block (cmd, 12);
  check_status (verbose);
  return id_d == 3;
}

static int
try_unlocking_byte (int byte)
{
  unsigned char key[7];
  memset (key, byte, 7);
  return try_unlocking (key);
}

static int
try_unlocking_string (char *str)
{
  int i;
  int k[7];
  unsigned char key[7];

  sscanf (str, "%x%*c%x%*c%x%*c%x%*c%x%*c%x%*c%x",
	  k+0,
	  k+1, k+2, k+3, k+4, k+5, k+6);
  for (i=0; i<7; i++)
    key[i] = k[i];
  return try_unlocking (key);
}

static void
my_exit ()
{
  exit(1);
}


void
section_info (char *action, int vma, int base, int size, char *name)
{
  unsigned int line_end = base + size - 1;
  if (line_end == 0xffffUL)
    chip_type = R8C_CHIP;
  else if (line_end == 0xfffffUL)
    chip_type = M16C_CHIP;
  else if (line_end == 0xffffffUL)
    chip_type = M32C_CHIP;
  else if (line_end == 0xffffffffUL)
    chip_type = R32C_CHIP;

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

/* Extra data to be stored in flash, on top of the program's data.  We
   can't store it as we process it, because we load the file after
   option processing.  */
typedef struct StoreData {
  struct StoreData *next;
  byte *data;
  mcu_addr_t start;
  mcu_addr_t length;
} StoreData;

StoreData *store_data = NULL;
StoreData **store_last = &store_data;

int
main(int argc, char **argv)
{
  int o, i, a, b, c, d;
  int baud = -1, baudcode;
  char *port = NULL;
  int test_connection = 0;
  int use_cnvss = -1;
  char *key = 0;
  FILE *prog;

  tty = isatty (1);
  if (tty)
    {
      char *term = getenv("TERM");
      if (!term
	  || strcmp (term, "") == 0
	  || strcmp (term, "dumb") == 0)
	tty = 0;
    }

  signal(SIGINT, my_exit);

  while ((o = getopt (argc, argv, "cuvtTrp:b:qCMk:B:F:S:P:3")) != -1)
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
	board_type = -1;
	for (i=0; i<BTM_LEN; i++)
	  if (strcasecmp (optarg, board_type_map[i].name) == 0)
	    board_type = board_type_map[i].type;
	if (board_type == -1)
	  {
	    printf("Unknown board type `%s'.  Known board types:\n", optarg);
	    for (i=0; i<BTM_LEN; i++)
	      printf("\t%s\n", board_type_map[i].name);
	    exit(1);
	  }
	break;

      case 'S':
	{
	  int start, size;
	  StoreData *sd;
	  int i, j;

	  j = sscanf (optarg, "%i%*c%i%*c%n", &start, &size, &i);
	  if (j < 2)
	    {
	      printf("Invalid syntax for -S: expected start,end,data, like 0xff00,16,foobar\n");
	      printf("you specified: \"%s\"\n", optarg);
	      exit(1);
	    }
	  optarg += i;
	  if (size == 0)
	    size = strlen (optarg) + 1;

	  sd = (StoreData *) malloc (sizeof (StoreData));
	  sd->next = NULL;
	  sd->data = (byte *) malloc (size + 1);
	  sd->start = start;
	  sd->length = size;
	  *store_last = sd;
	  store_last = &(sd->next);

	  for (i=0; i<size && optarg[i]; i++)
	    sd->data[i] = optarg[i];
	  for (; i<size; i++)
	    sd->data[i] = 0;
	}
	break;

      case '3':
	if (baud <= 0)
	  baud = 9600;
	serial_boot_mode = 3;
	serial_set_mode (3);
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
	printf("  -b <baud>     specify baud rate (default 57600, or 9600 for mode 3)\n");
	printf("  -C            CNVss chip (0=user, 1=program)\n");
	printf("  -M            MODE chip (1=user, 0=program)\n");
	printf("  -R            R32C mode (longer key)\n");
	printf("  -F start,end  portion of flash to save (hex addresses)\n");
	printf("  -k KEY        ID key, as XX.XX.XX.XX.XX.XX.XX (hex)\n");
	printf("  -3            Use serial mode 3\n");
	printf("  -B <board>    board type - gr8c, pm, pmp, powermeter, gpio (default gr8c)\n");
	printf("  -S start,len,data  store extra data - if len is 0, use data length with NUL terminator\n");
	printf("                example: -S 0xff00,0,\"my string\"\n");
	printf("                         -S 0xff00,8,8A4BR  (NUL padded if needed)\n");
	exit(1);
      }

  if (baud <= 0)
    baud = 57600;

  if (verbose)
    setbuf(stdout, 0);

  if (argv[optind])
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

      tprintf("chip %d cnvss %d\n", chip_type, use_cnvss);

      while (store_data)
	{
	  StoreData *tmp;
	  progmem_puts (store_data->start, store_data->data, store_data->length);
	  free (store_data->data);
	  tmp = store_data->next;
	  free (store_data);
	  store_data = tmp;
	}

      if (use_cnvss != -1)
	serial_use_cnvss (use_cnvss);
      else if (chip_type == M16C_CHIP
	       || chip_type == M32C_CHIP
	       || chip_type == R32C_CHIP)
	serial_use_cnvss (1);
      else
	serial_use_cnvss (0);

      serial_init (port, BAUD_RATE);

      {
	int saved_verbose = verbose;
	verbose = 0;
	serial_set_timeout(1);

	while (serial_ready ())
	  serial_read ();
	for (i=0; i<3; i++)
	  if (serial_read () == -1)
	    break;
	verbose = saved_verbose;
      }

      /* This next bit works for the case where we've just reset and need
	 to auto-baud, and the case where we're already sitting at the
	 monitor loop (assuming you don't change the baud rate).  */

      serial_pause (40);


      tprintf("sync\n");
      /* RESET AND SYNC*/
      serial_set_timeout(200);
      for (i=0; i<24; i++)
	{
	  serial_write(0);
	  serial_pause(40);
	  if (serial_ready () > 0)
	    {
	      if (serial_read () == 0xb0)
		break;
	    }
	}
      serial_drain();

      /* BAUD RATE 9600 */
      serial_write(0xB0);
      for (i=0; i<3; i++)
	{
	  int b = serial_read ();
	  if (b == 0xb0)
	    break;
	}

      /* BAUD RATE */
      if (baud != BAUD_RATE)
	{
	  switch (baud)
	    {
	    case 9600:
	      baudcode = 0xb0;
	      break;
	    case 19200:
	      baudcode = 0xb1;
	      break;
	    case 38400:
	      baudcode = 0xb2;
	      break;
	    case 57600:
	      baudcode = 0xb3;
	      break;
	    case 115200:
	    case 750000:
	      baudcode = 0xb4;
	      break;
	    default:
	      printf("unsupported programming speed: %d baud\n", baud);
	      exit(1);
	    }

	  serial_write (baudcode);
	  i = serial_read ();
	  if (i != baudcode)
	    {
	      printf("speed change failed! %02x\n", i);
	      exit(1);
	    }
	  serial_change_baud (baud);
	  serial_pause(20);
	}

      /* VERSION CHECK */
      {
	const char *chip_name;
	char ver[9];
	serial_write (0xfb);
	printf("Version = \"");
	for (i=0; i<8; i++)
	  {
	    ver[i] = serial_read (); 
	    putchar(ver[i]);
	  }
	ver[8] = 0;
	switch (chip_type)
	  {
	  case R8C_CHIP:  chip_name = "R8C";  break;
	  case M16C_CHIP: chip_name = "M16C"; break;
	  case M32C_CHIP: chip_name = "M32C"; break;
	  case R32C_CHIP: chip_name = "R32C"; break;
	  default: chip_name = "unknown"; break;
	  }
	printf("\"  Chip = %s\n", chip_name);
	if (ver[0] != 'V'
	    && ver[1] != 'E'
	    && ver[2] != 'R'
	    && ver[3] != '.'
	    && !isdigit (ver[4])
	    && ver[5] != '.'
	    && !isdigit (ver[6])
	    && !isdigit (ver[7]))
	  {
	    printf("Invalid version string \"%s\"\n", ver);
	    exit(1);
	  }
      }

      check_status(verbose);

      if (id_d != 3 && key)
	try_unlocking_string (key);
      if (id_d != 3)
	try_unlocking_byte (0xff);
      if (id_d != 3)
	try_unlocking_byte (0);

      if (save_data_flash)
	{
	  byte buf[256];
	  int i;

	  for (i=data_flash_start; i<data_flash_end; i+=256)
	    {
	      read_page (i >> 8, buf);
	      progmem_puts (i, buf, 256);
	    }
	}

      if (verbose > 0)
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
		      printf("0x%02x%02x---- ", a, b);
		      for (d=0; d<PROGCOLMASK+1; d++)
			{
			  if (progmem[a][b][c+d])
			    printf(" %02x", c+d);
			  else
			    printf(" --");
			}
		      printf("\n");
		    }
	      }
	  }

      if (just_query)
	exit(0);

      /* ERASE ALL */
      for (i=0; i<3; i++)
	{
	  unsigned char buf[256];

	  tprintf("erase all\n");
	  serial_set_timeout(1000);
	  clear_status ();
	  serial_write (0xA7);
	  serial_write (0xD0);
	  serial_sync ();
	  serial_pause(100);
	  check_status(verbose);
	  switch (chip_type)
	    {
	    case R8C_CHIP:
	      if (read_page (0x0000ff, buf))
		goto erase_ok;
	      break;
	    case M16C_CHIP:
	      if (read_page (0x000fff, buf))
		goto erase_ok;
	      break;
	    case M32C_CHIP:
	      read_page (0x00ffff, buf);
	      break;
	    case R32C_CHIP:
	      read_page (0xffffff, buf);
	      break;
	    default:
	      goto erase_ok;
	    }
	  if (buf[255] == 0xff
	      && buf[254] == 0xff
	      && buf[253] == 0xff
	      && buf[252] == 0xff)
	    break;
	  printf("erase failed\n");
	  exit(1);
	}
    erase_ok:

      if (id_d != 3)
	check_id ();

      /* PROGRAM */

      /* This is the flash locking key.  We force it to be all zeros
	 in case the user didn't think of this when building the
	 program image file.  */
      switch (chip_type)
	{
	case R8C_CHIP:
	  if (progmem_present (0x0000ffff))
	    {
	      progmem_putc (0x0000ffdf, 0xff);
	      progmem_putc (0x0000ffe3, 0xff);
	      progmem_putc (0x0000ffeb, 0xff);
	      progmem_putc (0x0000ffef, 0xff);
	      progmem_putc (0x0000fff3, 0xff);
	      progmem_putc (0x0000fff7, 0xff);
	      progmem_putc (0x0000fffb, 0xff);
	      progmem_putc (0x0000ffff, 0xff); /* watchdog */
	    }
	  break;
	case M16C_CHIP:
	  if (progmem_present (0x000fffff))
	    {
	      progmem_putc (0x000fffdf, 0xff);
	      progmem_putc (0x000fffe3, 0xff);
	      progmem_putc (0x000fffeb, 0xff);
	      progmem_putc (0x000fffef, 0xff);
	      progmem_putc (0x000ffff3, 0xff);
	      progmem_putc (0x000ffff7, 0xff);
	      progmem_putc (0x000ffffb, 0xff);
	    }
	  break;
	case M32C_CHIP:
	  if (progmem_present (0x00ffffff))
	    {
	      progmem_putc (0x00ffffdf, 0xff);
	      progmem_putc (0x00ffffe3, 0xff);
	      progmem_putc (0x00ffffeb, 0xff);
	      progmem_putc (0x00ffffef, 0xff);
	      progmem_putc (0x00fffff3, 0xff);
	      progmem_putc (0x00fffff7, 0xff);
	      progmem_putc (0x00fffffb, 0xff);
	    }
	  break;
	case R32C_CHIP:
	  if (progmem_present (0xffffffff))
	    {
	      progmem_putc (0xffffffe8, 0xff);
	      progmem_putc (0xffffffe9, 0xff);
	      progmem_putc (0xffffffea, 0xff);
	      progmem_putc (0xffffffeb, 0xff);
	      progmem_putc (0xffffffec, 0xff);
	      progmem_putc (0xffffffed, 0xff);
	      progmem_putc (0xffffffee, 0xff);
	    }
	  break;
	default:
	  printf("Warning: unable to set in-rom key to all zeros.\n");
	  break;
	}

      if (!tty)
	printf("Flash:");
      serial_set_timeout(100);
      for (a=0; a<256; a++)
	if (progmem[a])
	  for (b=0; b<256; b++)
	    if (progmem[a][b])
	      for (c=0; c<256; c++)
		if (progmem[a][b][c])
		  program_page (a*65536+b*256+c, progmem[a][b][c]);

      printf("\ndone!\n");
    }
  else
    {
      serial_init (port, baud);
      serial_change_baud (baud);
    }

  /* For the console; the user must unjumper CNVss and reset if the
     circuit can't do it automatically. */

  if (console)
    do_console (-1, baud);
  else
    serial_setup_console ();

  exit(0);
}
