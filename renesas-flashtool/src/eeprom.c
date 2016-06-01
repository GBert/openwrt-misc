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

/* This is the start of a program to read and write the EEPROM in the
   FT232R family of chips.  It currently works partly; it doesn't set
   all the fields that the FTDI utility does.  Mostly, this exists for
   non-Windows platforms.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <getopt.h>

#include <ftdi.h>

typedef unsigned char byte;

extern int verbose;
#define dprintf if (verbose>1) printf

static struct ftdi_context handle_s;
static struct ftdi_context *handle = &handle_s;

#define MAX_EEPROM 256

int read_mode = 0;
int write_mode = 0;
const char *ept_filename;
FILE *ept_file;

char *read_file = NULL;
char *write_file = NULL;

typedef struct {
  byte b0;			/* 00 */
#define EE_B0_HIGH_CURRENT	0x04
#define EE_B0_D2XX_DRIVER	0x08
  byte b1;			/* 01 */
  byte idVendor[2];		/* 02-03 */
  byte idProduct[2];		/* 04-05 */
  byte device[2];		/* 06-07 */
  byte config;			/* 08 */
#define EE_CONFIG_BASE		0x80
#define EE_CONFIG_SELFPOWER	0x40
#define EE_CONFIG_REMOTEWAKE	0x20
#define EE_CONFIG_BATTERYPOWER	0x10
  byte max_power;		/* 09 */
  byte chip_config;		/* 0A */
#define EE_CHIP_USB_VERSION	0x10
#define EE_CHIP_SERIALNO	0x08
#define EE_CHIP_SUSPEND_PD	0x04
#define EE_CHIP_OUT_ISOCHRON	0x02
#define EE_CHIP_IN_ISOCHRON	0x01
  byte invert;			/* 0B */
#define EE_INVERT_RI		0x80
#define EE_INVERT_DCD		0x40
#define EE_INVERT_DSR		0x20
#define EE_INVERT_DTR		0x10
#define EE_INVERT_CTS		0x08
#define EE_INVERT_RTS		0x04
#define EE_INVERT_RXD		0x02
#define EE_INVERT_TXD		0x01
  byte usb_version_low;		/* 0C */
  byte usb_version_high;	/* 0D */
  byte manuf_str_ofs;		/* 0E */
  byte manuf_str_len;		/* 0F */
  byte device_str_ofs;		/* 10 */
  byte device_str_len;		/* 11 */
  byte serial_str_ofs;		/* 12 */
  byte serial_str_len;		/* 13 */
  byte c0_c1;			/* 14 */
  byte c2_c3;			/* 15 */
  byte c4;			/* 16 */
  byte strings[0x7e - 0x17];	/* 17 - 7D */
#define STRING_BASE 0x17;
  byte checksum_low;		/* 7E */
  byte checksum_high;		/* 7F */
} EEProm;

typedef struct {
  union {
    byte buf[MAX_EEPROM];
    EEProm eeprom;
  };
  int buf_size;
  char manuf[MAX_EEPROM];
  char device[MAX_EEPROM];
  char serial[MAX_EEPROM];
} EEPROM;

EEPROM rbuf;
EEPROM wbuf;

static int
Fail1(int status, const char *file, int line, const char *sline)
{
  const char *er = "Unknown error";

  if (status >= 0)
    return status;

  er = ftdi_get_error_string (handle);

  printf("%s:%d: Error %s (%d) doing %s\n", file, line, er, status, sline);
  exit(1);
}

#define Fail(x) Fail1(x,__FILE__,__LINE__,#x)

/* ------------------------------------------------------------ */

#ifdef __WIN32
#undef isatty
#define isatty(x) 0
#endif

static void
color (int n)
{
  static int tty = -2;
  static int last_color = -2;
  if (tty == -2)
    tty = isatty (1);
  if (!tty)
    return;
  if (n == last_color)
    return;
  last_color = n;
  printf("\033[%dm", n);
}

static int
min3 (int a, int b, int c)
{
  if (a < b && a < c)
    return a;
  if (b < c)
    return b;
  return c;
}

static const char *cbus_purposes[16] = {
  "txden",	"pwron",	"rxled",	"txled",
  "txrx",	"sleep",	"clk48",	"clk24",
  "clk12",	"clk6",		"io",		"bbWR",
  "bbRD",	"rxf",		"?14",		"?15"
};

#define CBUS_TXDEN	0
#define	CBUS_PWRON	1
#define	CBUS_RXLED	2
#define	CBUS_TXLED	3
#define	CBUS_TXRX	4
#define	CBUS_SLEEP	5
#define	CBUS_CLK48	6
#define	CBUS_CLK24	7
#define	CBUS_CLK12	8
#define	CBUS_CLK6	9
#define	CBUS_IO		10
#define	CBUS_BBWR	11
#define	CBUS_BBRD	12
#define	CBUS_RXF	13

static const char *inverts[8] = {
  "txd", "rxd", "rts", "cts", "dtr", "dsr", "dcd", "ri"
};

static char *cb, *ci, *cs, *cr;

static void
cbus (int which, int purpose)
{
  printf("CBUS%d: %s%s%s", which, cb, cbus_purposes[purpose], cr);
  if (which == 4)
    printf("\n");
  else
    printf("\t");
}

static void
decode_imagebuf (EEPROM *eeprom)
{
  EEProm *e = & eeprom->eeprom;
  int i;
  unsigned short ck;

  if (isatty (1))
    {
      cb = "\033[34m";
      ci = "\033[36m";
      cs = "\033[32m";
      cr = "\033[0m";
    }
  else
    {
      cb = ci = cs = cr = "";
    }

  printf("High Current: %s%s%s\n", cb, (e->b0 & EE_B0_HIGH_CURRENT) ? "enabled" : "disabled", cr);
  printf("Driver: %s%s%s\n", cb, (e->b0 & EE_B0_D2XX_DRIVER) ? "D2XX" : "VCP", cr);
  printf("idVendor:  %s0x%02x%02x%s\t", ci, e->idVendor[1], e->idVendor[0], cr);
  printf("idProduct: %s0x%02x%02x%s\t", ci, e->idProduct[1], e->idProduct[0], cr);
  printf("idDevice:  %s0x%02x%02x%s\n", ci, e->device[1], e->device[0], cr);
  printf("config:%s", cb);
  if (e->config & EE_CONFIG_SELFPOWER)     printf(" self-powered");
  if (e->config & EE_CONFIG_REMOTEWAKE)    printf(" remote-wake");
  if (e->config & EE_CONFIG_BATTERYPOWER)  printf(" battery-power");
  printf("%s\n", cr);
  printf("Max power: %s%d%s mA\n", ci, e->max_power * 2, cr);
  printf("Chip config:%s", cb);
  if (e->chip_config & EE_CHIP_USB_VERSION)   printf(" use-usb-version");
  if (e->chip_config & EE_CHIP_SERIALNO)      printf(" use-serial-no");
  if (e->chip_config & EE_CHIP_SUSPEND_PD)    printf(" suspend-pull-down");
  if (e->chip_config & EE_CHIP_OUT_ISOCHRON)  printf(" out-isochronos");
  if (e->chip_config & EE_CHIP_IN_ISOCHRON)   printf(" in-isochronos");
  printf("%s\n", cr);
  printf("Invert:%s", cb);
  for (i=0; i<8; i++)
    if (e->invert & (1 << i))   printf(" %s", inverts[i]);
  printf("%s\n", cr);
  printf("USB Version: %s%d.%d%s\n", ci, e->usb_version_high, e->usb_version_low, cr);
  printf ("Manufacturer: \"%s%s%s\"\n", cs, eeprom->manuf, cr);
  printf ("Device: \"%s%s%s\"\n",  cs, eeprom->device, cr);
  printf ("Serial number: \"%s%s%s\"\n", cs, eeprom->serial, cr);
  cbus (0, e->c0_c1 & 0x0f);
  cbus (1, e->c0_c1 >> 4);
  cbus (2, e->c2_c3 & 0x0f);
  cbus (3, e->c2_c3 >> 4);
  cbus (4, e->c4    & 0x0f);

  ck = 0xAAAA;
  for (i=0; i<126; i+=2)
    {
      unsigned short v = eeprom->buf[i] + 256 * eeprom->buf[i+1];
      ck ^= v;
      ck = (ck << 1) | (ck >> 15);
    }
  if (e->checksum_low == (ck & 0xff)
      && e->checksum_high == (ck >> 8))
    printf("Checksum: %sValid%s\n", cb, cr);
  else
    printf("Checksum: %sINVALID%s\n", cb, cr);
}

static void
dump_imagebuf (byte *buf)
{
  int i, j;
  for (i=0; i<MAX_EEPROM; i += 16)
    {
      printf("%02x  : ", i);
      for (j=i; j<i+16; j++)
	{
	  if (j % 16 == 8)
	    putchar(' ');
	  color (buf[j] == 0xff ? 34 : buf[j] ? 32 : 0);
	  printf(" %02x", buf[j]);
	}
      color (0);
      printf ("  :  ");
      for (j=i; j<i+16; j++)
	{
	  if (j % 16 == 8)
	    putchar(' ');
	  if (isgraph (buf[j]))
	    {
	      color (32);
	      putchar (buf[j]);
	    }
	  else
	    {
	      color (0);
	      putchar('.');
	    }
	}
      color (0);
      printf("  :  %02x\n", i);
    }
}

/* ------------------------------------------------------------ */

static void
close_chip ()
{
  ftdi_deinit (handle);
}

static void
choose_chip (int chosen)
{
  int i;
  int num_devices;
  struct ftdi_device_list *device_info;
  struct usb_device *u_chose = NULL;

  Fail (ftdi_init (handle));

  num_devices = Fail (ftdi_usb_find_all (handle, &device_info, 0x0403, 0x6001));
  printf("%d device%s connected", num_devices, num_devices?"":"s");
  if (num_devices > 1 && chosen >= 0)
    printf(", you chose device %d", chosen);
  printf("\n");

  if (num_devices == 0)
    {
      printf("um, no FTDI devices present.\n");
      exit(1);
    }

  for (i=0; i<num_devices; i++)
    {
      char manufacturer[200], description[200], serial[200];
      struct usb_device *u = device_info->dev;

      ftdi_usb_get_strings (handle, u, manufacturer, 200, description, 200, serial, 200);

      if (i == chosen)
	u_chose = u;

      printf("Dev %2d: ", i);
      printf("Filename \"%s\", " , device_info->dev->filename);
      printf("Manf \"%s\", ", manufacturer);
      printf("Desc \"%s\", ", description);
      printf("Ser# \"%s\"\n", serial);
      device_info = device_info->next;
    }

  if (num_devices > 1 && !u_chose)
    {
      printf("Too many devices to choose from!  Use \"-choose <n>\" to choose\n");
      exit (1);
    }

  if (u_chose)
    Fail (ftdi_usb_open_dev (handle, u_chose));
  else
    Fail (ftdi_usb_open (handle, 0x0403, 0x6001));

  atexit (close_chip);
}

static int
read_eeprom (byte *buf)
{
  int s;
  s = ftdi_read_eeprom_getsize (handle, buf, MAX_EEPROM);
  return s;
}

static void
write_eeprom (byte *buf)
{
  ftdi_write_eeprom (handle, buf);
}

static void
read_imagefile (char *filename, byte *buf)
{
  FILE *f = fopen(filename, "rb");
  if (!f)
    {
      perror (filename);
      exit (1);
    }
  fread (buf, 1, MAX_EEPROM, f);
  fclose (f);
}

static void
write_imagefile (char *filename, byte *buf)
{
  FILE *f = fopen(filename, "wb");
  if (!f)
    {
      perror (filename);
      exit (1);
    }
  fwrite (buf, 1, MAX_EEPROM, f);
  fclose (f);
}

static void
extract_string (EEPROM *e, int ofs, char *str)
{
  int len;
  ofs &= 0x7f;
  len = e->buf[ofs] - 2;
  if (len < 0)
    {
      *str = 0;
      return;
    }
  ofs += 2;
  while (len)
    {
      *str++ = e->buf[ofs];
      ofs += 2;
      len -= 2;
    }
  *str = 0;
}

static void
extract_strings (EEPROM *e)
{
  extract_string (e, e->eeprom.manuf_str_ofs, e->manuf);
  extract_string (e, e->eeprom.device_str_ofs, e->device);
  extract_string (e, e->eeprom.serial_str_ofs, e->serial);
}

static int
store_string (EEPROM *e, int i, byte *ofs, byte *len, char *str)
{
  int slen = strlen (str);
  int save_ofs = i;
  char *save_str = str;
  *ofs = i | 0x80;
  *len = slen * 2 + 2;
  e->buf[i] = slen * 2 + 2;
  e->buf[i+1] = 3;
  i += 2;
  while (slen)
    {
      e->buf[i] = *str++;
      e->buf[i+1] = 0;
      i += 2;
      if (i > 125)
	{
	  fprintf(stderr, "Error: string table overflow storing \"%s\" at %d\n",
		  save_str, save_ofs);
	  exit(1);
	}
      slen --;
    }
  return i;
}

static void
store_strings (EEPROM *e)
{
  int i = min3 (e->eeprom.manuf_str_ofs,
		e->eeprom.device_str_ofs,
		e->eeprom.serial_str_ofs);
  i &= 0x7f;
  i = store_string (e, i, &e->eeprom.manuf_str_ofs, &e->eeprom.manuf_str_len, e->manuf);
  i = store_string (e, i, &e->eeprom.device_str_ofs, &e->eeprom.device_str_len, e->device);
  i = store_string (e, i, &e->eeprom.serial_str_ofs, &e->eeprom.serial_str_len, e->serial);
}

static void
recompute_checksum (EEPROM *e)
{
  int i;
  unsigned short ck = 0xAAAA;
  for (i=0; i<126; i+=2)
    {
      unsigned short v = e->buf[i] + 256 * e->buf[i+1];
      ck ^= v;
      ck = (ck << 1) | (ck >> 15);
    }
  e->eeprom.checksum_low = ck & 0xff;
  e->eeprom.checksum_high = ck >> 8;
}

/* ------------------------------------------------------------ */

static void
usage (char *argv0)
{
  int i;
  fprintf (stderr, "Usage: EEProm [-choose <num>] [options]   (short options in parens)\n");

  fprintf (stderr, "\nIf these are given, the device is updated:\n");
  fprintf (stderr, "  -cbus0 <purpose>    choose cbus0 pin's purpose.  Same for 1,2,3,4 (-0,-1,etc)\n");
  fprintf(stderr, "   ");
  for (i=0; i<16; i++)
    fprintf(stderr, " %s", cbus_purposes[i]);
  fprintf(stderr, "\n");

  fprintf (stderr, "  -invert <pin,...>   say which pins are inverted.  Use +pin or -pin to change\n");
  fprintf (stderr, "                      just the listed, else all not listed are cleared. (-i)\n");
  fprintf(stderr, "   ");
  for (i=0; i<8; i++)
    fprintf(stderr, " %s", inverts[i]);
  fprintf(stderr, "\n");
  fprintf (stderr, "  -manuf[acturer] \"<str>\"\n");
  fprintf (stderr, "  -device \"<str>\"\n");
  fprintf (stderr, "  -serial \"<str>\"     set the manufacturer, device, or serial number strings\n");
  fprintf (stderr, "  -power <mAmps>      set max power (-p)\n");
  fprintf (stderr, "  -self               device is self-powered\n");
  fprintf (stderr, "  -bus                device is bus-powered\n");

  fprintf (stderr, "\nIf these are given, nothing is written back to the device:\n");
  fprintf (stderr, "  -decode             decode the eeprom rather than update it\n");
  fprintf (stderr, "  -dump               hex dump the eeprom rather than update it\n");
  fprintf (stderr, "  -backup <file>      back up the eeprom data to <file>\n");
  fprintf (stderr, "  -restore <file>     restore the eeprom data from <file>\n");

  fprintf (stderr, "\nExamples:\n\n");
  fprintf (stderr, "  %s -choose 2 -invert +dtr,ri -device \"FT232R - R8C\" -2 clk48\n", argv0);
  fprintf (stderr, "  %s -decode\n", argv0);
  fprintf (stderr, "  %s -backup r8c-adapter.ee\n", argv0);
  exit (1);
}

typedef enum {
  OPT_MANUF,
  OPT_DEVICE,
  OPT_SERIAL,
  OPT_SELF,
  OPT_BUS,
  OPT_BACKUP,
  OPT_RESTORE,
  OPT_DECODE,
  OPT_DUMP
} OptionVals;

static struct option EEProm_options[] = {
  { "cbus0", 1, 0, '0' },
  { "cbus1", 1, 0, '1' },
  { "cbus2", 1, 0, '2' },
  { "cbus3", 1, 0, '3' },
  { "cbus4", 1, 0, '4' },
  { "invert", 1, 0, 'i' },
  { "manuf", 1, 0, OPT_MANUF },
  { "manufacturer", 1, 0, OPT_MANUF },
  { "device", 1, 0, OPT_DEVICE },
  { "serial", 1, 0, OPT_SERIAL },
  { "power", 1, 0, 'p' },
  { "self", 0, 0, OPT_SELF },
  { "bus", 0, 0, OPT_BUS },

  { "decode", 0, 0, OPT_DECODE },
  { "dump", 0, 0, OPT_DUMP },
  { "backup", 1, 0, OPT_BACKUP },
  { "restore", 1, 0, OPT_RESTORE },
  { NULL, 0, 0, 0 }
};

typedef enum {
  INV_NONE, INV_ADD, INV_SUB
} Invert_Mode;

int
main(int argc, char **argv)
{
  char optch;
  EEProm *e;
  int just_exit = 0;
  int i, c;
  int chosen = -1;

  if (argc > 2
      && (strcmp (argv[1], "-choose") == 0
	  || strcmp (argv[1], "--choose") == 0))
    {
      char *argv0 = argv[0];
      chosen = atoi (argv[2]);
      argv += 2;
      argc -= 2;
      argv[0] = argv0;
    }

  choose_chip (chosen);

  rbuf.buf_size = read_eeprom (rbuf.buf);
  extract_strings (&rbuf);

  memcpy (&wbuf, &rbuf, sizeof(EEPROM));
  e = & wbuf.eeprom;

  while ((optch = getopt_long_only (argc, argv, "0:1:2:3:4:i:p:",
				    EEProm_options, NULL)) != -1)
    {
      switch (optch)
	{
	case OPT_DECODE:
	  store_strings (&wbuf);
	  recompute_checksum (&wbuf);
	  decode_imagebuf (&wbuf);
	  just_exit = 1;
	  break;

	case OPT_DUMP:
	  store_strings (&wbuf);
	  recompute_checksum (&wbuf);
	  dump_imagebuf (wbuf.buf);
	  just_exit = 1;
	  break;

	case OPT_BACKUP:
	  printf("backing up to %s\n", optarg);
	  write_imagefile (optarg, rbuf.buf);
	  exit (0);

	case OPT_RESTORE:
	  printf("restoring from %s\n", optarg);
	  read_imagefile (optarg, wbuf.buf);
	  write_eeprom (wbuf.buf);
	  exit (0);

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	  c = optch - '0';
	  for (i=0; i<16; i++)
	    if (strcmp (optarg, cbus_purposes[i]) == 0)
	      break;
	  if (i == 16)
	    {
	      fprintf(stderr, "Error: \"%s\" is not a valid CBUS purpose:\n", optarg);
	      for (i=0; i<16; i++)
		fprintf(stderr, " %s", cbus_purposes[i]);
	      fprintf(stderr, "\n");
	      exit (1);
	    }
	  switch (c)
	    {
	    case 0:
	      e->c0_c1 = (e->c0_c1 & 0xf0) | i;
	      break;
	    case 1:
	      e->c0_c1 = (e->c0_c1 & 0x0f) | (i << 4);
	      break;
	    case 2:
	      e->c2_c3 = (e->c2_c3 & 0xf0) | i;
	      break;
	    case 3:
	      e->c2_c3 = (e->c2_c3 & 0x0f) | (i << 4);
	      break;
	    case 4:
	      e->c4 = i;
	      break;
	    }
	  break;

	case 'i':
	  {
	    char *opt, save;
	    Invert_Mode imode;

	    imode = INV_NONE;
	    opt = optarg;
	    while (*opt)
	      {
		if (*opt == '+')
		  {
		    imode = INV_ADD;
		    opt ++;
		  }
		else if (*opt == '-')
		  {
		    imode = INV_SUB;
		    opt ++;
		  }
		else
		  {
		    char *sp = opt;
		    while (*sp && *sp != ',')
		      sp++;
		    save = *sp;
		    *sp = 0;

		    for (i=0; i<8; i++)
		      if (strcmp (inverts[i], opt) == 0)
			break;
		    if (i == 8)
		      {
			fprintf(stderr, "Error: \"%s\" is not a valid invertable pin:\n", opt);
			for (i=0; i<8; i++)
			  fprintf(stderr, " %s", inverts[i]);
			fprintf(stderr, "\n");
			exit (1);
		      }
		    switch (imode)
		      {
		      case INV_NONE:
			imode = INV_ADD;
			e->invert = 0;
		      case INV_ADD:
			e->invert |= 1 << i;
			break;
		      case INV_SUB:
			e->invert &= ~(1 << i);
			break;
		      }
		    *sp = save;
		    opt = sp;
		    if (*opt == ',')
		      opt ++;
		  }
	      }
	  }
	  break;

	case OPT_MANUF:
	  strcpy (wbuf.manuf, optarg);
	  break;
	case OPT_DEVICE:
	  strcpy (wbuf.device, optarg);
	  break;
	case OPT_SERIAL:
	  strcpy (wbuf.serial, optarg);
	  break;

	case 'p':
	  e->max_power = atoi (optarg) / 2;
	  break;

	case OPT_SELF:
	  e->config |= EE_CONFIG_SELFPOWER;
	  break;
	case OPT_BUS:
	  e->config &= ~EE_CONFIG_SELFPOWER;
	  break;
	  
	default:
	  usage (argv[0]);
	  exit (0);
	}
    }

  if (just_exit)
    exit (0);

  store_strings (&wbuf);
  recompute_checksum (&wbuf);

  if (memcmp (rbuf.buf, wbuf.buf, 127))
    {
      printf("writing out new EEProm...\n");
      write_eeprom (wbuf.buf);
    }
  else
    printf("no changes need to be written out\n");

  return 0;
}
