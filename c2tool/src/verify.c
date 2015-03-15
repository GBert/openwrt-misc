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

#define PACKAGE
#include <bfd.h>

#include "c2tool.h"

struct flash_section_data {
	struct c2tool_state *state;
	unsigned int offset;
	unsigned int errctr;
};

static void verify_section(bfd * ibfd, sec_ptr isection, void *arg)
{
	bfd_size_type size = bfd_section_size(ibfd, isection);
	/* bfd_vma vma = bfd_section_vma(ibfd, isection); */
	bfd_vma lma = bfd_section_lma(ibfd, isection);
	const char *section_name = bfd_section_name(ibfd, isection);
	bfd_byte *data = 0;
	struct flash_section_data *fsdata = arg;
	struct c2tool_state *state = fsdata->state;
	unsigned int offset = fsdata->offset;
	unsigned int flash_addr = lma + offset;
	unsigned char buf[256];

	printf("section %s: lma %016" BFD_VMA_FMT "x, size %d -> flash %08x\n",
	       section_name, lma, (int)size, flash_addr);

	if (!bfd_get_full_section_contents (ibfd,isection, &data)) {
		fprintf(stderr, "Reading section failed\n");
		return;
	}

	while (size) {
		int res;
		unsigned int chunk = size > sizeof(buf) ? sizeof(buf) : size;

		res = c2_flash_read(state, flash_addr, chunk, buf);
		if (res)
			return;
		if (memcmp(data, buf, chunk)) {
			printf("verify failed in %d byte chunk at %08x\n", chunk, flash_addr);
			fsdata->errctr++;
		}

		flash_addr += chunk;
		size -= chunk;
		data += chunk;
	}

	printf("section %s: verify ok\n", section_name);
}

static int c2_verify_file(struct c2tool_state *state, const char *filename, const char *target, unsigned int offset)
{
	bfd *ibfd;
	struct flash_section_data fsdata = { state, offset, 0 };

	ibfd = bfd_openr(filename, target);
	if (ibfd == NULL) {
		fprintf(stderr, "%s\n", bfd_errmsg(bfd_get_error()));
		return -EINVAL;
	}

	if (!bfd_check_format(ibfd, bfd_object)) {
		fprintf(stderr, "%s\n", bfd_errmsg(bfd_get_error()));
		return -EINVAL;
	}

	bfd_map_over_sections(ibfd, verify_section, &fsdata);

	return fsdata.errctr ? -EINVAL : 0;
}

int handle_verify(struct c2tool_state *state, int argc, char **argv)
{
	/* unsigned char buf[256]; */
	unsigned int offset = 0;
	char *end;
	char *target = NULL;
	char *filename;

	while (argc >= 3) {
		if (strcmp(argv[0], "target") == 0) {
			argc--;
			argv++;

			if (!argc)
				return 1;

			target = argv[0];
			argv++;
			argc--;
		} else if (strcmp(argv[0], "adjust-start") == 0) {
			argc--;
			argv++;

			if (!argc)
				return 1;

			offset = strtoul(argv[0], &end, 0);
			if (*end != '\0')
				return 1;

			argv++;
			argc--;
		} else {
			break;
		}
	}

	if (argc == 1)
		filename = argv[0];
	else
		return 1;

	if (c2family_setup(state) < 0)
		return -EIO;

	return c2_verify_file(state, filename, target, offset);
}

COMMAND(verify, "[target <bfdname>] [adjust-start <incr>] <file>", handle_verify, "Verify file to flash memory of connected device.");
