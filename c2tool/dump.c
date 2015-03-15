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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "c2family.h"
#include "c2tool.h"
#include "hexdump.h"

int handle_dump(struct c2tool_state *state, int argc, char **argv)
{
	unsigned char buf[256];
	unsigned int offset = 0;
	unsigned int len = sizeof(buf);
	char *end;

	if (argc) {
		offset = strtoul(argv[0], &end, 0);
		if (*end == '\0') {
			argv++;
			argc--;
		} else {
			return 1;
		}
	}

	if (argc) {
		len = strtoul(argv[0], &end, 0);
		if (*end == '\0') {
			argv++;
			argc--;
		} else {
			return 1;
		}
	}

	while (len) {
		unsigned int chunk = len > sizeof(buf) ? sizeof(buf) : len;

		c2_flash_read(state, offset, chunk, buf);
		print_hex_dump("", DUMP_PREFIX_ADDRESS, offset, 16, 1, buf, chunk, 1);

		offset += chunk;
		len -=chunk;
	}

	return 0;
}

COMMAND(dump, "[offset] [len]", handle_dump, "Dump flash memory of connected device.");
