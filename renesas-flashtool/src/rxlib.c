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
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "serial.h"

#include "rxlib.h"

extern int verbose;

typedef unsigned char byte;

typedef struct {
  int response;		/* if 0x06, standard ack/nak */
  int pkt_cmd;		/* if nonzero, number of length bytes */
  int pkt_response;	/* ditto */
  int long_timeout;
  int cmd;
} RXcmd;

RXcmd rxcmds[] = {
  { 0x00, 0, 0, 0, RX_CMD_BAUD_RATE_MATCHING },
  { 0x06, 0, 0, 0, RX_CMD_ACK },
  { 0xe6, 0, 0, 0, RX_CMD_GENERIC_BOOT_CHECK },

  { 0x30, 0, 1, 0, RX_CMD_INQ_SUPPORTED_DEVS },
  { 0x06, 1, 0, 0, RX_CMD_DEVICE_SELECTION },
  { 0x31, 0, 1, 0, RX_CMD_INQ_CLOCK_MODES },
  { 0x06, 1, 0, 0, RX_CMD_CLOCK_MODE_SELECTION },
  { 0x32, 0, 1, 0, RX_CMD_INQ_FREQ_MULTIPLIERS },
  { 0x33, 0, 1, 0, RX_CMD_INQ_OPERATING_FREQ },
  { 0x34, 0, 1, 0, RX_CMD_INQ_BOOT_FLASH },
  { 0x35, 0, 1, 0, RX_CMD_INQ_USER_FLASH },
  { 0x36, 0, 2, 0, RX_CMD_INQ_ERASURE_BLOCK },
  { 0x37, 0, 1, 0, RX_CMD_INQ_PROG_SIZE },
  { 0x3A, 0, 1, 0, RX_CMD_INQ_DATA_FLASH },
  { 0x3B, 0, 1, 0, RX_CMD_INQ_DATA_FLASH_INFO },
  { 0x06, 1, 0, 0, RX_CMD_NEW_BAUD_RATE },
  { 0x06, 0, 0, 1, RX_CMD_DATA_SETTING_COMPLETE },
  { 0x06, 1, 0, 0, RX_CMD_ID_CHECK },

  { 0x06, 0, 0, 0, RX_CMD_PREP_BOOT_FLASH },
  { 0x06, 0, 0, 0, RX_CMD_PREP_USER_FLASH },
  { 0x06, 7, 0, 10, RX_CMD_N_BYTE_PROGRAMMING }, /* This one is handled specially */
  { 0x06, 0, 0, 0, RX_CMD_PREP_ERASURE },
  { 0x06, 1, 0, 1, RX_CMD_BLOCK_ERASE },

  { 0x52, 1, 4, 0, RX_CMD_MEMORY_READ },

  { 0x5F, 0, 1, 0, RX_CMD_INQ_BOOT_PROGRAM_STATUS },
  { 0x5A, 0, 1, 1, RX_CMD_CHECKSUM_BOOT_FLASH },
  { 0x5B, 0, 1, 1, RX_CMD_CHECKSUM_USER_FLASH },
  { 0x06, 0, 0, 1, RX_CMD_BLANKCHK_BOOT_FLASH },
  { 0x06, 0, 0, 1, RX_CMD_BLANKCHK_USER_FLASH },

  { 0x00, 1, 0, 0, RX_CMD_LOCKBIT_STATUS_READ },
  { 0x06, 1, 0, 0, RX_CMD_LOCKBIT_PROGRAM },
  { 0x06, 0, 0, 0, RX_CMD_LOCKBIT_ENABLE },
  { 0x06, 0, 0, 0, RX_CMD_LOCKBIT_DISABLE }
};
#define NUM_RXCMDS (sizeof(rxcmds)/sizeof(rxcmds[0]))

static int
rxcmd_sort (const void *va, const void *vb)
{
  RXcmd *a = (RXcmd *)va;
  RXcmd *b = (RXcmd *)vb;
  return b->cmd - a->cmd;
}

static RXcmd *lookup_cmd (int cmd)
{
  int i;
  for (i=0; i<NUM_RXCMDS; i++)
    if (rxcmds[i].cmd == cmd)
      return rxcmds + i;
  assert (cmd);
  return NULL;
}

void
rxlib_init ()
{
  qsort (rxcmds, NUM_RXCMDS, sizeof(rxcmds[0]), rxcmd_sort);
}

static RXcmd *cmd;
static int pkt_code;
static int pkt_length, pkt_buf_size=0;
static byte *wpkt=NULL, *pkt=NULL;
static int pkt_checksum;

static unsigned long
get_long (byte *buf)
{
  return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static unsigned long
get_short (byte *buf)
{
  return (buf[0] << 8) | buf[1];
}

static void
put_long (byte *buf, unsigned long v)
{
  buf[0] = v >> 24;
  buf[1] = v >> 16;
  buf[2] = v >> 8;
  buf[3] = v >> 0;
}

#if 0
static void
put_short (byte *buf, unsigned short v)
{
  buf[0] = v >> 8;
  buf[1] = v >> 0;
}
#endif

static void
need_pkt_size (int len)
{
  pkt_length = len;
  if (pkt_buf_size < pkt_length)
    {
      pkt_buf_size = pkt_length;
      wpkt = (byte *) realloc (wpkt, pkt_buf_size + 6);
      pkt = wpkt + 5;
    }
}

static int
rx_read_packet ()
{
  int i, a, b;
  int csum = 0;

  if (verbose > 1)
    printf("\n\033[32mResponse: \033[0m");

  csum = 0;
  pkt_code = serial_read ();
  if (pkt_code == -1)
    return RX_ERR_TIMEOUT;
  csum += (byte)pkt_code;

  if (pkt_code != cmd->response)
    {
      return serial_read ();
    }

  switch (cmd->pkt_response)
    {
    case 4:
    case 2:
    case 1:
      pkt_length = 0;
      for (a=0; a<cmd->pkt_response; a++)
	{
	  b = serial_read ();
	  if (b == -1)
	    return RX_ERR_TIMEOUT;
	  pkt_length = pkt_length * 256 + b;
	  csum += (byte) b;
	}
      break;

    default:
      pkt_length = 0;
      break;
    }

  need_pkt_size (pkt_length);

  for (i=0; i<pkt_length; i++)
    {
      pkt[i] = serial_read ();
      csum += pkt[i];
    }
  pkt_checksum = serial_read ();
  csum += (byte)pkt_checksum;

  csum &= 0xff;
  if (csum != 0)
    {
      fprintf(stderr, "checksum failure: got 0x%02x\n", csum);
      return RX_ERR_CHECKSUM;
    }
  return RX_ERR_NONE;
}

static int
rx_command (int command)
{
  cmd = lookup_cmd (command);
  int csum, i, x;
  int tries = 5;

  serial_drain ();

  serial_set_timeout (50 + 1000 * cmd->long_timeout);

 try_again:
  if (verbose > 1)
    printf("\n\033[32mCommand: \033[0m");
  switch (cmd->pkt_cmd)
    {
    case 0:
      serial_write (command);
      break;

    case 1:
      pkt[-2] = command;
      pkt[-1] = pkt_length;
      csum = command;
      csum += pkt_length;
      for (i=0; i<pkt_length; i++)
	csum += pkt[i];
      pkt[pkt_length] = -csum;
      serial_write_block (pkt-2, pkt_length + 3);
      break;

    case 7: /* only for N_BYTE_PROGRAMMING.  */
      pkt[-1] = command;
      csum = command;
      for (i=0; i<pkt_length; i++)
	csum += pkt[i];
      pkt[pkt_length] = -csum;
      serial_write_block (pkt-1, pkt_length + 2);
      serial_sync ();
      x = 0;
      do
	{
	  serial_pause (10);
	  if (serial_ready ())
	    break;
	  serial_write (0);
	  serial_sync ();
	  x ++;
	} while (!serial_ready ());
#if 0
      if (x)
	printf("%d NULs\n", x);
#endif
      break;
    }

  if (cmd->pkt_response)
    {
      i = rx_read_packet ();
      if (i != RX_ERR_NONE && tries--)
	goto try_again;
    }

  if (cmd->response == RX_ERR_ACK)
    {
      /* ack/nack */
      i = serial_read ();
      if (i == RX_ERR_ACK)
	return RX_ERR_NONE;
      if (i == RX_ERR_ID_REQUIRED)
	return RX_ERR_NONE;
      i = serial_read ();
      if ((i == RX_ERR_TIMEOUT || i == RX_ERR_UNDEFINED_CMD) && tries--)
	goto try_again;
      return i;
    }

  return RX_ERR_NONE;
}

int
rxlib_reset ()
{
  int i;

  while (1)
    {
      //printf("\n\033[31mreset\033[0m\n");
      serial_reset ();

      if (serial_need_autobaud ())
	{
	  serial_set_timeout(1);
	  for (i=0; i<3; i++)
	    if (serial_read () == -1)
	      break;

	  serial_pause (40);

	  /* RESET AND SYNC*/
	  serial_set_timeout(200);
	  for (i=0; i<30; i++)
	    {
	      if (serial_ready () > 0)
		if (serial_read () == 0)
		  break;
	      serial_write(RX_CMD_BAUD_RATE_MATCHING);
	      serial_pause(40);
	    }
	  serial_drain();
	}

      serial_write (RX_CMD_GENERIC_BOOT_CHECK);
      i = serial_read ();
      if (i == 0xe6)
	{
	  //printf("yay!\n");
	  serial_set_timeout (100);
	  return RX_ERR_NONE;
	}
      else if (i == RX_ERR_BITRATE_MATCH)
	{
	  //printf("boo!\n");
	  return RX_ERR_BITRATE_MATCH;
	}
      else
	{
	  //printf("huh?\n");
	  serial_pause (100);
	}
    }
}

int
rxlib_inquire_device (int *num_devices, char ***device_codes, char ***product_names)
{
  static int _devices = 0;
  static char **_codes = NULL;
  static char **_names = NULL;
  int i;
  byte *pp;

  if (! _codes)
    {
      if ((i = rx_command (RX_CMD_INQ_SUPPORTED_DEVS)) != RX_ERR_NONE)
	{
	  if (num_devices)
	    *num_devices = _devices;
	  return i;
	}

      pp = pkt;

      _devices = *pp++;;
      _codes = (char **) malloc ((_devices+1) * sizeof (char *));
      _names = (char **) malloc ((_devices+1) * sizeof (char *));
      _codes[_devices] = NULL;
      _names[_devices] = NULL;

      for (i=0; i<_devices; i++)
	{
	  int nbytes = *pp++;

	  _codes[i] = (char *) malloc (5);
	  memcpy (_codes[i], pp, 4);
	  _codes[i][4] = 0;
	  pp += 4;
	  nbytes -= 4;

	  _names[i] = (char *) malloc (nbytes + 1);
	  memcpy (_names[i], pp, nbytes);
	  _names[i][nbytes] = 0;
	  pp += nbytes;
	}
    }

  if (num_devices)
    *num_devices = _devices;
  if (device_codes)
    *device_codes = _codes;
  if (product_names)
    *product_names = _names;

  return RX_ERR_NONE;
}

int
rxlib_select_device (char *code)
{
  need_pkt_size (4);
  memcpy (pkt, code, 4);
  return rx_command (RX_CMD_DEVICE_SELECTION);
}

int
rxlib_inquire_clock_modes (int *num, int **modes)
{
  int i;

  if ((i = rx_command (RX_CMD_INQ_CLOCK_MODES)) != RX_ERR_NONE)
    return i;

  if (num)
    *num = pkt[0];
  if (modes)
    {
      *modes = (int *) malloc (pkt[0] * sizeof (int));
      for (i=0; i<pkt[0]; i++)
	(*modes)[i] = pkt[i+1];
    }
  return RX_ERR_NONE;
}

int
rxlib_select_clock_mode (int mode)
{
  need_pkt_size (1);
  pkt[0] = mode;
  return rx_command (RX_CMD_CLOCK_MODE_SELECTION);
}

int
rxlib_inquire_frequency_multipliers ()
{
  int i, m;
  byte *p = pkt+1;
  verbose ++;
  rx_command (RX_CMD_INQ_FREQ_MULTIPLIERS);
  verbose --;
  for (i=0; i<pkt[0]; i++)
    {
      int num_multipliers = *p++;
      fprintf(stderr, "%3d:", i);
      for (m=0; m<num_multipliers; m++)
	fprintf(stderr, " %3d", *p++);
      fprintf(stderr, "\n");
    }
  return RX_ERR_NONE;
}

int
rxlib_inquire_operating_frequency ()
{
  int i;
  rx_command (RX_CMD_INQ_OPERATING_FREQ);
  for (i=0; i<pkt[0]; i++)
    {
      int a, b;
      a = get_short (pkt + i*4+1);
      b = get_short (pkt + i*4+3);
      if (verbose > 1)
	fprintf(stderr, "clock[%d]: %g - %g MHz\n", i, a / 100.0, b / 100.0);
    }
  return RX_ERR_NONE;
}

int
rxlib_set_baud_rate (int baud)
{
  int clock = (12 * 100);
  int i;

  need_pkt_size (7);
  pkt[0] = (baud/100) >> 8;
  pkt[1] = (baud/100);
  pkt[2] = clock >> 8;
  pkt[3] = clock;
  pkt[4] = 2;
  pkt[5] = 8;	/* main clock multiplier */
  pkt[6] = 4;	/* peripheral clock multiplier */

  i = rx_command (RX_CMD_NEW_BAUD_RATE);
  if (i != RX_ERR_NONE)
    return i;
  serial_pause (1);

  serial_change_baud (baud);

  return rx_command (RX_CMD_ACK);
}

int
rxlib_data_setting_complete ()
{
  return rx_command (RX_CMD_DATA_SETTING_COMPLETE);
}

static int
rxlib_inquire_memrange (int cmd, int *num_areas, RxMemRange **areas, int *_num, RxMemRange **_areas)
{
  int i;
  byte *pp;


  if (!*_areas)
    {
      if ((i = rx_command (cmd)) != RX_ERR_NONE)
	{
	  if (num_areas)
	    *num_areas = 0;
	  return i;
	}

      pp = pkt;
      *_num = *pp++;
      *_areas = (RxMemRange *) malloc (*_num * sizeof (RxMemRange));

      for (i=0; i<*_num; i++)
	{
	  (*_areas)[i].start = get_long (pp + 0);
	  (*_areas)[i].end = get_long (pp + 4);
	  (*_areas)[i].size = (*_areas)[i].end - (*_areas)[i].start + 1;

	  pp += 8;
	}
    }

  if (num_areas)
    *num_areas = *_num;
  if (areas)
    *areas = *_areas;

  return RX_ERR_NONE;
}

int
rxlib_inquire_user_flash_size (int *num_areas, RxMemRange **areas)
{
  static int _num = 0;
  static RxMemRange *_areas = NULL;
  return rxlib_inquire_memrange (RX_CMD_INQ_USER_FLASH,
				 num_areas, areas,
				 &_num, &_areas);
}

int
rxlib_inquire_data_flash_size (int *num_areas, RxMemRange **areas)
{
  static int _num = 0;
  static RxMemRange *_areas = NULL;
  return rxlib_inquire_memrange (RX_CMD_INQ_DATA_FLASH_INFO,
				 num_areas, areas,
				 &_num, &_areas);
}

int
rxlib_inquire_boot_flash_size (int *num_areas, RxMemRange **areas)
{
  static int _num = 0;
  static RxMemRange *_areas = NULL;
  return rxlib_inquire_memrange (RX_CMD_INQ_BOOT_FLASH,
				 num_areas, areas,
				 &_num, &_areas);
}

int
rxlib_inquire_flash_erase_blocks (int *num_areas, RxMemRange **areas)
{
  static int _num = 0;
  static RxMemRange *_areas = NULL;
  return rxlib_inquire_memrange (RX_CMD_INQ_ERASURE_BLOCK,
				 num_areas, areas,
				 &_num, &_areas);
}

static int programming_size = 0;

int
rxlib_inquire_programming_size (int *size)
{
  int i;

  if (programming_size == 0)
    {
      i = rx_command (RX_CMD_INQ_PROG_SIZE);
      if (i != RX_ERR_NONE)
	return i;

      programming_size = get_short (pkt);
    }
  if (size)
    *size = programming_size;

  return RX_ERR_NONE;
}

int
rxlib_status (int *boot_status, int *error_status)
{
  int i;
  if ((i = rx_command (RX_CMD_INQ_BOOT_PROGRAM_STATUS)) != RX_ERR_NONE)
    return i;

  if (boot_status)
    *boot_status = pkt[0];
  if (error_status)
    *error_status = pkt[1];

  /* printf("[\033[32m%02x %02x\033[0m]", pkt[0], pkt[1]); */
  return RX_ERR_NONE;
}

int
rxlib_begin_user_flash ()
{
  return rx_command (RX_CMD_PREP_USER_FLASH);
}

int
rxlib_begin_boot_flash ()
{
  return rx_command (RX_CMD_PREP_BOOT_FLASH);
}

int
rxlib_program_block (unsigned long address, unsigned char *data)
{
  if (programming_size == 0)
    {
      printf("You have to call rxlib_inquire_programming_size() before programming!\n");
      return RX_ERR_PROGRAMMING;
    }
  need_pkt_size (programming_size + 4);
  put_long (pkt, address);
  memcpy (pkt+4, data, programming_size);

  return rx_command (RX_CMD_N_BYTE_PROGRAMMING);
}

int
rxlib_done_programming ()
{
  need_pkt_size (4);
  pkt[0] = 0xff;
  pkt[1] = 0xff;
  pkt[2] = 0xff;
  pkt[3] = 0xff;

  return rx_command (RX_CMD_N_BYTE_PROGRAMMING);
}

int
rxlib_read_memory (unsigned long address, unsigned long count, unsigned char *data)
{
  int i;

  need_pkt_size (9);

  pkt[0] = (address >= 0xffe00000) ? 1 : 0;
  put_long (pkt+1, address);
  put_long (pkt+5, count);

  i = rx_command (RX_CMD_MEMORY_READ);
  if (i != RX_ERR_NONE)
    return i;

  memcpy (data, pkt, pkt_length);

  return RX_ERR_NONE;
}

int
rxlib_blank_check_user ()
{
  return rx_command (RX_CMD_BLANKCHK_USER_FLASH);
}

int
rxlib_blank_check_boot ()
{
  return rx_command (RX_CMD_BLANKCHK_BOOT_FLASH);
}
