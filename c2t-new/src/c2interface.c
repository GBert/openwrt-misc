/*
 * Copyright 2014 Dirk Eibach <eibach@gdsys.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include "c2family.h"
#include "c2interface.h"
#include "c2tool.h"
#include "c2tool-gpio.h"
#include "hexdump.h"
/*
 * C2 registers & commands defines
 */

/* C2 registers */
#define C2_DEVICEID		0x00
#define C2_REVID		0x01
#define C2_FPCTL		0x02

/* C2 interface commands */
#define C2_GET_VERSION	0x01
#define C2_DEVICE_ERASE	0x03
#define C2_BLOCK_READ	0x06
#define C2_BLOCK_WRITE	0x07
#define C2_PAGE_ERASE	0x08

#define C2_FPDAT_GET_VERSION	0x01
#define C2_FPDAT_GET_DERIVATIVE	0x02
#define C2_FPDAT_DEVICE_ERASE	0x03
#define C2_FPDAT_BLOCK_READ	0x06
#define C2_FPDAT_BLOCK_WRITE	0x07
#define C2_FPDAT_PAGE_ERASE	0x08
#define C2_FPDAT_DIRECT_READ	0x09
#define C2_FPDAT_DIRECT_WRITE	0x0a
#define C2_FPDAT_INDIRECT_READ	0x0b
#define C2_FPDAT_INDIRECT_WRITE	0x0c

#define C2_FPDAT_RETURN_INVALID_COMMAND	0x00
#define C2_FPDAT_RETURN_COMMAND_FAILED	0x02
#define C2_FPDAT_RETURN_COMMAND_OK	0x0D

#define C2_FPCTL_HALT		0x01
#define C2_FPCTL_RESET		0x02
#define C2_FPCTL_CORE_RESET	0x04

void c2_reset(struct c2interface *c2if)
{
	/* To reset the device we have to keep clock line low for at least
	 * 20us.
	 */
	ioctl(c2if->c2port_fd, C2PORT_RESET);

	usleep(1);
}

static void c2_write_ar(struct c2interface *c2if, unsigned char addr)
{
	struct c2port_command *c2pc = malloc(sizeof(struct c2port_command));
	c2pc->ar = addr;
	ioctl(c2if->c2port_fd, C2PORT_WRITE_AR, c2pc);
	free(c2pc);

	/* printf("%s: 0x%02X\n", __func__, addr); */
}

static int c2_read_ar(struct c2interface *c2if, unsigned char *addr)
{
	int ret;
	struct c2port_command *c2pc = malloc(sizeof(struct c2port_command));
	ret = ioctl(c2if->c2port_fd, C2PORT_READ_AR, c2pc);
	*addr = c2pc->ar;
	/* printf("%s: 0x%02X\n", __func__, *addr); */
	free(c2pc);

	return ret;
}

static int c2_write_dr(struct c2interface *c2if, unsigned char data)
{
	int ret;
	struct c2port_command *c2pc = malloc(sizeof(struct c2port_command));
	c2pc->dr = data;
	ret = ioctl(c2if->c2port_fd, C2PORT_WRITE_DR, c2pc);
	/* printf("%s: 0x%02X\n", __func__, data); */

	return ret;
}

static int c2_read_dr(struct c2interface *c2if, unsigned char *data)
{
	int ret;
	struct c2port_command *c2pc = malloc(sizeof(struct c2port_command));
	ret = ioctl(c2if->c2port_fd, C2PORT_READ_DR, c2pc);
	*data = c2pc->dr;
	/* printf("%s: 0x%02X\n", __func__, *data); */

	return ret;
}

static int c2_poll_in_busy(struct c2interface *c2if)
{
	unsigned char addr;
	int ret, timeout = 20;

	do {
		ret = (c2_read_ar(c2if, &addr));
		if (ret < 0)
			return -EIO;

		if (!(addr & 0x02))
			break;

		usleep(1);
	} while (--timeout > 0);
	if (timeout == 0)
		return -EIO;

	return 0;
}

static int c2_poll_out_ready(struct c2interface *c2if)
{
	unsigned char addr;
	int ret, timeout = 10000;	/* erase flash needs long time... */

	do {
		ret = (c2_read_ar(c2if, &addr));
		if (ret < 0)
			return -EIO;

		if (addr & 0x01)
			break;

		usleep(1);
	} while (--timeout > 0);
	if (timeout == 0)
		return -EIO;

	return 0;
}

int c2_read_sfr(struct c2interface *c2if, unsigned char sfr)
{
	unsigned char data;

	c2_write_ar(c2if, sfr);

	if (c2_read_dr(c2if, &data) < 0)
		return -EIO;

	return data;
}

int c2_write_sfr(struct c2interface *c2if, unsigned char sfr, unsigned char data)
{
	c2_write_ar(c2if, sfr);

	if (c2_write_dr(c2if, data) < 0)
		return -EIO;

	return 0;
}

/*
 * Programming interface (PI)
 * Each command is executed using a sequence of reads and writes of the FPDAT register.
 */

static int c2_pi_write_command(struct c2interface *c2if, unsigned char command)
{
	if (c2_write_dr(c2if, command) < 0)
		return -EIO;

	if (c2_poll_in_busy(c2if) < 0)
		return -EIO;

	return 0;
}

static int c2_pi_get_data(struct c2interface *c2if, unsigned char *data)
{
	if (c2_poll_out_ready(c2if) < 0)
		return -EIO;

	if (c2_read_dr(c2if, data) < 0)
		return -EIO;

	return 0;
}

static int c2_pi_check_command(struct c2interface *c2if)
{
	unsigned char response;

	if (c2_pi_get_data(c2if, &response) < 0)
		return -EIO;

	if (response != C2_FPDAT_RETURN_COMMAND_OK)
		return -EIO;

	return 0;
}

static int c2_pi_command(struct c2interface *c2if, unsigned char command, int verify,
			 unsigned char *result)
{
	if (c2_pi_write_command(c2if, command) < 0)
		return -EIO;

	if (!verify)
		return 0;

	if (c2_pi_check_command(c2if) < 0)
		return -EIO;

	if (!result)
		return 0;

	if (c2_pi_get_data(c2if, result) < 0)
		return -EIO;

	return 0;
}

int c2_read_direct(struct c2tool_state *state, unsigned char reg)
{
	unsigned char data;
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	if (c2_pi_command(c2if, C2_FPDAT_DIRECT_READ, 1, NULL))
		return -EIO;

	if (c2_pi_write_command(c2if, reg))
		return -EIO;

	if (c2_pi_write_command(c2if, 0x01))
		return -EIO;
	if (c2_poll_out_ready(c2if) < 0)
		return -EIO;
	if (c2_read_dr(c2if, &data) < 0)
		return -EIO;

	return data;
}

int c2_write_direct(struct c2tool_state *state, unsigned char reg, unsigned char value)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	if (c2_pi_command(c2if, C2_FPDAT_DIRECT_WRITE, 1, NULL))
		return -EIO;

	if (c2_pi_write_command(c2if, reg))
		return -EIO;

	if (c2_pi_write_command(c2if, 0x01))
		return -EIO;

	if (c2_pi_write_command(c2if, value))
		return -EIO;

	return 0;
}

int c2_flash_read(struct c2tool_state *state, unsigned int addr, unsigned int length,
			 unsigned char *dest)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	while (length) {
		unsigned int blocksize;
		unsigned int k;

		if (c2_pi_command(c2if, C2_FPDAT_BLOCK_READ, 1, NULL) < 0)
			return -EIO;

		if (c2_pi_command(c2if, addr >> 8, 0, NULL) < 0)
			return -EIO;
		if (c2_pi_command(c2if, addr & 0xff, 0, NULL) < 0)
			return -EIO;

		if (length > 255) {
			if (c2_pi_command(c2if, 0, 1, NULL) < 0)
				return -EIO;
			blocksize = 256;
		} else {
			if (c2_pi_command(c2if, length, 1, NULL) < 0)
				return -EIO;
			blocksize = length;
		}

		for (k = 0; k < blocksize; ++k) {
			unsigned char data;

			if (c2_pi_get_data(c2if, &data) < 0)
				return -EIO;
			if (dest)
				*dest++ = data;
		}

		length -= blocksize;
		addr += blocksize;
	}

	return 0;
}

static int c2_flash_write(struct c2tool_state *state, unsigned int addr, unsigned int length,
			 unsigned char *src)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	while (length) {
		unsigned int blocksize;
		unsigned int k;

		if (c2_pi_command(c2if, C2_FPDAT_BLOCK_WRITE, 1, NULL) < 0)
			return -EIO;

		if (c2_pi_command(c2if, addr >> 8, 0, NULL) < 0)
			return -EIO;
		if (c2_pi_command(c2if, addr & 0xff, 0, NULL) < 0)
			return -EIO;

		if (length > 255) {
			if (c2_pi_command(c2if, 0, 1, NULL) < 0)
				return -EIO;
			blocksize = 256;
		} else {
			if (c2_pi_command(c2if, length, 1, NULL) < 0)
				return -EIO;
			blocksize = length;
		}

		for (k = 0; k < blocksize; ++k) {
			if (c2_pi_command(c2if, *src++, 0, NULL) < 0)
				return -EIO;
		}

		length -= blocksize;
		addr += blocksize;
	}

	return 0;
}

int c2_flash_erase(struct c2tool_state *state, unsigned char page)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	if (c2_pi_command(c2if, C2_FPDAT_PAGE_ERASE, 1, NULL) < 0)
		return -EIO;

	if (c2_pi_command(c2if, page, 1, NULL) < 0)
		return -EIO;

	if (c2_pi_command(c2if, 0, 1, NULL) < 0)
		return -EIO;

	return 0;
}

int c2_halt(struct c2interface *c2if)
{
	c2_reset(c2if);

	usleep(2);

	c2_write_ar(c2if, C2_FPCTL);

	if (c2_write_dr(c2if, C2_FPCTL_RESET) < 0)
		return -EIO;

	if (c2_write_dr(c2if, C2_FPCTL_CORE_RESET) < 0)
		return -EIO;

	if (c2_write_dr(c2if, C2_FPCTL_HALT) < 0)
		return -EIO;

	usleep(20000);

	return 0;
}

int c2_get_device_info(struct c2interface *c2if, struct c2_device_info *info)
{
	unsigned char data;
	int ret;

	/* Select DEVID register for C2 data register accesses */
	c2_write_ar(c2if, C2_DEVICEID);

	/* Read and return the device ID register */
	ret = c2_read_dr(c2if, &data);
	if (ret < 0)
		return -EIO;

	info->device_id = data;

	/* Select REVID register for C2 data register accesses */
	c2_write_ar(c2if, C2_REVID);

	/* Read and return the revision ID register */
	ret = c2_read_dr(c2if, &data);
	if (ret < 0)
		return -EIO;

	info->revision_id = data;

	return 0;
}

int c2_get_pi_info(struct c2tool_state *state, struct c2_pi_info *info)
{
	unsigned char data;
	int ret;
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	/* Select FPDAT register for C2 data register accesses */
	c2_write_ar(c2if, family->fpdat);

	ret = c2_pi_command(c2if, C2_FPDAT_GET_VERSION, 1, &data);
	if (ret < 0)
		return -EIO;

	info->version = data;

	ret = c2_pi_command(c2if, C2_FPDAT_GET_DERIVATIVE, 1, &data);
	if (ret < 0)
		return -EIO;

	info->derivative = data;

	return 0;
}

int c2_flash_erase_device(struct c2tool_state *state)
{
	struct c2interface *c2if = &state->c2if;
	struct c2family *family = state->family;

	c2_write_ar(c2if, family->fpdat);

	if (c2_pi_command(c2if, C2_FPDAT_DEVICE_ERASE, 1, NULL) < 0)
		return -EIO;

	if (c2_pi_command(c2if, 0xde, 0, NULL) < 0)
		return -EIO;

	if (c2_pi_command(c2if, 0xad, 0, NULL) < 0)
		return -EIO;

	if (c2_pi_command(c2if, 0xa5, 1, NULL) < 0)
		return -EIO;

	return 0;
}

int flash_chunk(struct c2tool_state *state, unsigned int addr, unsigned int length,
		       unsigned char *src)
{
	/* struct c2interface *c2if = &state->c2if; */
	struct c2family *family = state->family;
	unsigned int page_size = family->page_size;
	unsigned int page = addr / page_size;
	unsigned char buf[page_size];
	int must_read = (addr % page_size) || (length < page_size);
	unsigned int page_base = page * page_size;
	unsigned int chunk_start = addr - page_base;
	unsigned int chunk_len = (chunk_start + length > page_size) ?
				 (page_size  - chunk_start) : length;

	if (must_read) {
		if (c2_flash_read(state, page_base, page_size, buf))
			return -EIO;
	}

	memcpy(buf + addr - page_base, src, chunk_len);

	if (c2_flash_erase(state, page))
		return -EIO;

	if (c2_flash_write(state, page_base, page_size, buf))
		return -EIO;

	return chunk_len;
}

#if 0
int main(int argc, char **argv)
{
	unsigned char data;
	int ret;

	struct c2interface *c2if = malloc(sizeof(struct c2interface));

	c2if->c2port_fd = open("/dev/c2port-gpio", O_RDWR);
	if (c2if->c2port_fd < 0) {
		fprintf(stderr, "%s: error: open failed [%s]\n", __func__, strerror(errno));
		exit(-1);
	}

	c2_halt(c2if);

	/* Select REVID register for C2 data register accesses */
	c2_write_ar(c2if, C2_REVID);

	/* Read and return the revision ID register */
	ret = c2_read_dr(c2if, &data);
	if (ret < 0) {
		fprintf(stderr, "%s: can't read REV_ID [%s]\n", __func__, strerror(errno));
		return 1;
	}

	printf("REVID 0x%02x\n", data);

	/* Select DEVID register for C2 data register accesses */
	c2_write_ar(c2if, C2_DEVICEID);

	/* Read and return the device ID register */
	ret = c2_read_dr(c2if, &data);
	if (ret < 0)
		return 1;

	printf("DEVID 0x%02x\n", data);

	/* Select FPDAT register for C2 data register accesses */
	c2_write_ar(c2if, 0xad);

	ret = c2_pi_command(c2if, C2_FPDAT_GET_DERIVATIVE, 1, &data);
	if (ret < 0)
		return 1;

	printf("derivative %02x\n", data);

	ret = c2_pi_command(c2if, C2_FPDAT_GET_VERSION, 1, &data);
	if (ret < 0)
		return 1;

	printf("version %02x\n", data);
/*

	c2_flash_read(0, 256, buf);
	print_hex_dump("", DUMP_PREFIX_ADDRESS, 0, 16, 1, buf, 256, 1);

	printf("Erase...\n");
	c2_flash_erase_device();

	c2_flash_read(0, 256, buf);
	print_hex_dump("", DUMP_PREFIX_ADDRESS, 0, 16, 1, buf, 256, 1);

	flash_file("test.hex", "");

	c2_flash_read(0, 256, buf);
	print_hex_dump("", DUMP_PREFIX_ADDRESS, 0, 16, 1, buf, 256, 1);
*/
	close(c2if->c2port_fd);
	return 0;
}
#endif
