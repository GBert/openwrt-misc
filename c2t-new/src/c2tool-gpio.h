/*
 *  Silicon Labs C2 port Linux support for c2tool
 *
 *  Copyright (c) 2015 Gerhard Bertelsmann <info@gerhard-bertelsmann.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation
 */

#ifndef _C2TOOL_GPIO_H
#define _C2TOOL_GPIO_H

struct c2port_command {
	uint8_t ar;
	uint8_t dr;
	uint8_t length;		/* not used */
	uint8_t access;
};

#define C2PORT_GPIO_MAJOR (180)
#define C2PORT_RESET		  _IO(C2PORT_GPIO_MAJOR, 100)
#define C2PORT_WRITE_AR		 _IOW(C2PORT_GPIO_MAJOR, 101, struct c2port_command *)
#define C2PORT_READ_AR		_IOWR(C2PORT_GPIO_MAJOR, 102, struct c2port_command *)
#define C2PORT_WRITE_DR		 _IOW(C2PORT_GPIO_MAJOR, 103, struct c2port_command *)
#define C2PORT_READ_DR		_IOWR(C2PORT_GPIO_MAJOR, 104, struct c2port_command *)

#ifndef __KERNEL__

#endif

#endif /* _C2TOOL_GPIO_H */
