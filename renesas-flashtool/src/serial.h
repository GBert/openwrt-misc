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

typedef enum {
  BT_unknown,
  BT_powermeterproto,
  BT_powermeter,
  BT_sdram,
  BT_rx600,
  BT_rx,
  BT_m32c10p
} BoardTypes;

extern BoardTypes board_type;

void serial_param (char *parameter);

/* Note that the definition of "port" here depends on the driver.  For
   USB-based interfaces, strings of small integers ("0"-"99") usually
   mean the Nth appropriate interface, while larger strings (like
   "000") usually mean the device's "filename".  */
void serial_init(char *port, int baud);

/* Returns nonzero if the link has an actual UART in it somewhere, and
   thus the chip needs the autobaud sequence.  True for serial mode 2
   etc, false for RX's USB mode.  */
int serial_need_autobaud ();

void serial_reset ();

void serial_change_baud (int baud);

/* 1 = synchronous (not supported*
   2 = full duplex with separate MODE
   3 = half duplex on MODE */
void serial_set_mode (int serial_boot_mode);

void serial_use_cnvss (int use_cnvss);

/* If you don't use the actual console, you should call this to reset
   the chip into "user mode" if the driver supports it.  */
int serial_setup_console ();

void serial_set_timeout (int mseconds);

/* write out all output bytes and wait for them to finish transmitting. */
void serial_sync ();
/* read in any pending input bytes and discard them.  */
void serial_drain ();

void serial_pause (int msec);

void serial_write(unsigned char ch);
void serial_write_block(unsigned char *ch, int len);
void serial_write_string(char *ch);

/* returns -1 for timeout, else 0..255 */
int serial_read(void);
int serial_ready ();

