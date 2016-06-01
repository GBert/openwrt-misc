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

#ifndef _RXLIB_H_
#define	_RXLIB_H_


#define RX_CMD_BAUD_RATE_MATCHING	0x00
#define RX_CMD_ACK			0x06
#define	RX_CMD_DEVICE_SELECTION		0x10
#define	RX_CMD_CLOCK_MODE_SELECTION	0x11
#define	RX_CMD_INQ_SUPPORTED_DEVS	0x20
#define	RX_CMD_INQ_CLOCK_MODES		0x21
#define	RX_CMD_INQ_FREQ_MULTIPLIERS	0x22
#define	RX_CMD_INQ_OPERATING_FREQ	0x23
#define	RX_CMD_INQ_BOOT_FLASH		0x24	/* RX only */
#define	RX_CMD_INQ_USER_FLASH		0x25
#define	RX_CMD_INQ_ERASURE_BLOCK	0x26
#define	RX_CMD_INQ_PROG_SIZE		0x27
#define	RX_CMD_INQ_DATA_FLASH		0x2A	/* RX only */
#define	RX_CMD_INQ_DATA_FLASH_INFO	0x2B	/* RX only */
#define	RX_CMD_NEW_BAUD_RATE		0x3F
#define	RX_CMD_DATA_SETTING_COMPLETE	0x40
#define	RX_CMD_PREP_BOOT_FLASH		0x42	/* RX only */
#define	RX_CMD_PREP_USER_FLASH		0x43
#define	RX_CMD_PREP_ERASURE		0x48
#define	RX_CMD_CHECKSUM_BOOT_FLASH	0x4A	/* RX only */
#define	RX_CMD_CHECKSUM_USER_FLASH	0x4B
#define	RX_CMD_BLANKCHK_BOOT_FLASH	0x4C	/* RX only */
#define	RX_CMD_BLANKCHK_USER_FLASH	0x4D
#define	RX_CMD_INQ_BOOT_PROGRAM_STATUS	0x4F
#define	RX_CMD_N_BYTE_PROGRAMMING	0x50
#define	RX_CMD_MEMORY_READ		0x52
#define	RX_CMD_GENERIC_BOOT_CHECK	0x55
#define	RX_CMD_BLOCK_ERASE		0x58
#define RX_CMD_ID_CHECK			0x60	/* RX only */
#define	RX_CMD_LOCKBIT_STATUS_READ	0x71	/* RX only */
#define	RX_CMD_LOCKBIT_DISABLE		0x75	/* RX only */
#define	RX_CMD_LOCKBIT_PROGRAM		0x77	/* RX only */
#define	RX_CMD_LOCKBIT_ENABLE		0x7A	/* RX only */

#define RX_ERR_NONE			0x00
#define RX_ERR_ACK			0x06
#define	RX_ERR_CHECKSUM			0x11
#define RX_ERR_PROGRAM_SIZE		0x12	/* RX only */
#define RX_ERR_ID_REQUIRED		0x16	/* RX only */
#define RX_ERR_NONMATCH_DEVICE_CODE	0x21
#define RX_ERR_NONMATCH_CLOCK_MODE	0x22
#define RX_ERR_NONNEEDED_CLOCK_MODE	0x23	/* RX only */
#define RX_ERR_BAUD_RATE_FAILURE	0x24
#define RX_ERR_CLOCK			0x25
#define RX_ERR_FREQ_MULTIPLIER		0x26
#define RX_ERR_OPERATING_FREQ		0x27
#define RX_ERR_CTL_DOWNLOAD_ADDRESS	0x28	/* RX only */
#define RX_ERR_BLOCK_NUMBER		0x29
#define RX_ERR_ADDRESS			0x2A
#define RX_ERR_DATA_LENGTH		0x2B
#define RX_ERR_ERASURE			0x51
#define RX_ERR_NONERASED		0x52
#define RX_ERR_PROGRAMMING		0x53
#define RX_ERR_NONMATCH_ID		0x54
#define RX_ERR_ERASURE_VIA_ID		0x63	/* RX only */
#define RX_ERR_UNDEFINED_CMD		0x80
#define RX_ERR_BITRATE_MATCH		0xFF
#define	RX_ERR_TIMEOUT			-1

#define RX_BWAIT_DEVICE_SELECTION	0x11
#define	RX_BWAIT_CLOCK_MODE		0x12
#define RX_BWAIT_BIT_SELECTION		0x13
#define	RX_BWAIT_TRANS_PROGRAMMING	0x1F
#define RX_BWAIT_ERASING_FLASH		0x31
#define RX_BWAIT_PROGRAMMING_SELECTION	0x3F
#define RX_BWAIT_PROGRAMMING_DATA	0x4F
#define RX_BWAIT_ERASURE_BLOCKS		0x5F

typedef struct {
  unsigned long start;
  unsigned long end;
  unsigned long size;
} RxMemRange;

/* Most functions here return an RX_ERR_* code, or zero (RX_ERRO_NONE)
   for success.  */

/* Call once, ever, before anything else.  */
void rxlib_init ();

/* Call whenever you want to reset the chip and re-sync the baud rate.
   Returns nonzero on error.  */
int rxlib_reset ();

/* Normally this returns one device... */
int rxlib_inquire_device (int *num_devices, char ***device_codes, char ***produce_names);
int rxlib_select_device (char *device_code);

int rxlib_inquire_clock_modes (int *num, int **modes);
int rxlib_select_clock_mode (int mode);

int rxlib_inquire_frequency_multipliers ();
int rxlib_inquire_operating_frequency ();
int rxlib_set_baud_rate (int baud);
int rxlib_data_setting_complete ();

/* These return the flash memory configuration.  */
int rxlib_inquire_user_flash_size (int *num_areas, RxMemRange **areas);
int rxlib_inquire_data_flash_size (int *num_areas, RxMemRange **areas);
int rxlib_inquire_boot_flash_size (int *num_areas, RxMemRange **areas);

int rxlib_inquire_flash_erase_blocks (int *num_areas, RxMemRange **areas);

/* We only support 256 byte units.  */
int rxlib_inquire_programming_size (int *size);

int rxlib_status (int *boot_status, int *error_status);

/* These are for actual programming.  Call one of these, then
   rxlib_program() block as needed, then rxlib_done_programming().  */
int rxlib_begin_user_flash ();
int rxlib_begin_boot_flash ();

/* block size is from rxlib_inquire_programming_size().  */
int rxlib_program_block (unsigned long address, unsigned char *data);
int rxlib_done_programming ();

int rxlib_read_memory (unsigned long address, unsigned long count, unsigned char *data);

/* These return RX_ERR_NONE if the given area is blank.  */
int rxlib_blank_check_user ();
int rxlib_blank_check_boot ();
int rxlib_blank_check_data ();

#endif

