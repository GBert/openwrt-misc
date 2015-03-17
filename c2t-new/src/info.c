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
#include <errno.h>

#include "c2family.h"
#include "c2tool.h"

int handle_info(struct c2tool_state *state, int argc, char **argv)
{
	struct c2_device_info info;
	struct c2_pi_info pi_info;
	struct c2family *family = state->family;

	if (c2_get_device_info(&state->c2if, &info) < 0)
		return -EIO;

	printf("DEVICEID %02x, REVID %02x\n", info.device_id, info.revision_id);
	printf("%s family: %d byte pagesize, FPDAT at 0x%02x\n",
	       family->name, family->page_size, family->fpdat);

	if (c2_get_pi_info(state, &pi_info) < 0)
		return -EIO;

	printf("PI version %02x, derivative %02x\n",
	       pi_info.version, pi_info.derivative);

	return 0;
}

COMMAND(info, NULL, handle_info, "Show information about connected device.");
