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

#include <stddef.h>
#include <errno.h>
#include <unistd.h>

#include "c2family.h"
#include "c2interface.h"
#include "c2tool.h"

enum {
	C2_DONE,
	C2_WRITE_SFR,
	C2_WRITE_DIRECT, /* for devices with SFR paging */
	C2_READ_SFR,
	C2_READ_DIRECT, /* for devices with SFR paging */
	C2_WAIT_US,
};

static struct c2_setupcmd c2init_f30x[] = {
	{ C2_WRITE_SFR, 0xb2, 0x07 }, /* set intosc to 24.5 MHz */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f31x[] = {
	{ C2_WRITE_DIRECT, 0xef, 0x00 }, /* set SFRPAGE to 0x00 */
	{ C2_WRITE_DIRECT, 0xb2, 0x83 }, /* set intosc to 24.5 MHz */
	{ C2_DONE },
};
static struct c2_setupcmd c2init_f32x[] = {
	{ C2_WRITE_SFR, 0xb2, 0x83 }, /* set intosc to 24.5 MHz */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f33x[] = {
	{ C2_WRITE_DIRECT, 0xb2, 0x83 }, /* set intosc to 24.5 MHz */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f35x[] = {
	{ C2_WRITE_SFR, 0xb6, 0x10 }, /* Set Flash read timing for 50 MHz */
	{ C2_WRITE_SFR, 0xab, 0x00 }, /* Reset Clk multiplier to intosc/2 */
	{ C2_WRITE_SFR, 0xbe, 0x80 }, /* Enable Clk multiplier */
	{ C2_WAIT_US, .time = 5 }, /* wait for 5 us */
	{ C2_WRITE_SFR, 0xbe, 0xc0 }, /* Initialize the multiplier */
	{ C2_WAIT_US, .time = 10000 }, /* wait for 10 ms */
	{ C2_READ_SFR, 0xbe }, /* read clk multiplier value */
	{ C2_WRITE_SFR, 0xa9, 0x02 }, /* Set clk mul as system clock */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f41x[] = {
	{ C2_WRITE_SFR, 0xb6, 0x10 }, /* Set Flash read timing for 50 MHz */
	{ C2_WRITE_SFR, 0xc9, 0x10 }, /* Set Voltage regulator for 2.5 V */
	{ C2_WRITE_SFR, 0xff, 0xa0 }, /* Enable VDD monitor at high threshold */
	{ C2_WRITE_SFR, 0xef, 0x02 }, /* Enable VDDmon as a reset source */
	{ C2_WRITE_SFR, 0xb2, 0x87 }, /* set intosc to 24.5 MHz */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_si8250[] = {
	{ C2_WRITE_SFR, 0xb2, 0x87 }, /* set intosc to 24.5 MHz */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f34x[] = {
	{ C2_WRITE_SFR, 0xb6, 0x90 }, /* Set Flash read timing for 50 MHz, one shot enabled */
	{ C2_WRITE_SFR, 0xff, 0x80 }, /* Enable VDD monitor */
	{ C2_WRITE_SFR, 0xef, 0x02 }, /* Enable VDDmon as a reset source */
	{ C2_WRITE_SFR, 0xb9, 0x00 }, /* Reset Clk multiplier to intosc */
	{ C2_WRITE_SFR, 0xb9, 0x80 }, /* Enable Clk multiplier */
	{ C2_WAIT_US, .time = 5 }, /* wait for 5 us */
	{ C2_WRITE_SFR, 0xb9, 0xc0 }, /* Initialize the multiplier */
	{ C2_WAIT_US, .time = 0x10000 }, /* wait for 10 ms */
	{ C2_READ_SFR, 0xb9 }, /* read clk multiplier value */
	{ C2_WRITE_SFR, 0xa9, 0x03 }, /* Set clk mul as system clock */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_t60x[] = {
	{ C2_WRITE_SFR, 0xb2, 0x07 }, /* set intosc to 24.5 MHz */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f52x[] = {
//	{ C2_WRITE_DIRECT, 0xb2, 0x87 }, /* set intosc to 24.5 MHz */
//	{ C2_WRITE_DIRECT, 0xff, 0xa0 }, /*enable VDD monitor to high setting */
	{ C2_WRITE_SFR, 0xb2, 0x87 }, /* set intosc to 24.5 MHz */
	{ C2_WRITE_SFR, 0xff, 0xa0 }, /* enable VDD monitor to high setting */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f36x[] = {
	{ C2_WRITE_DIRECT, 0xa7, 0x0f }, /* SFRPAGE = 0x0f;  Set SFRPAGE to 0xf */
	{ C2_WRITE_DIRECT, 0xb7, 0x83 }, /* OSCICN = 0x83;   Set internal osc to 24.5 MHZ */
	{ C2_WRITE_DIRECT, 0x8f, 0x00 }, /* CLKSEL = 0x00;   Set sysclk to intosc */
	{ C2_WRITE_DIRECT, 0xa7, 0x00 }, /* SFRPAGE = 0x00;  Set SFRPAGE to 0x0 */
	{ C2_DONE },
#if 0
	/*
	 * The following is the sequence to operate from a 98 MHz system clock.  Flash read time testing
	 * indicates that this provides no speed benefit over operating from a 24.5 MHz clock,
	 * so we leave this out and just use the above.
	 */
	{ C2_WRITE_DIRECT, 0xa7, 0x0f }, /* SFRPAGE = 0x0f;  Set SFRPAGE to 0xf */
	{ C2_WRITE_DIRECT, 0x84, 0x00 }, /* CCH0CN = 0x00;   Disable prefetch engine while writing to FLRD bits */
	{ C2_WRITE_DIRECT, 0xa7, 0x00 }, /* SFRPAGE = 0x00;  Set SFRPAGE to 0x0 */
	{ C2_WRITE_DIRECT, 0xb6, 0x30 }, /* FLSCL = 0x30;    Set Flash read timing for 100 MHz */
	{ C2_WRITE_DIRECT, 0xa7, 0x0f }, /* SFRPAGE = 0x0f;  Set SFRPAGE to 0xf */
	{ C2_WRITE_DIRECT, 0xb7, 0x83 }, /* OSCICN = 0x83;   Set internal osc to 24.5 MHZ */
	{ C2_WRITE_DIRECT, 0xb3, 0x00 }, /* PLL0CN = 0x00;   Select intosc as pll clock source */
	{ C2_WRITE_DIRECT, 0xb3, 0x01 }, /* PLL0CN = 0x01;   Enable PLL power */
	{ C2_WRITE_DIRECT, 0xa9, 0x01 }, /* PLL0DIV = 0x01;  Divide by 1 */
	{ C2_WRITE_DIRECT, 0xa9, 0x01 }, /* PLL0DIV = 0x01;  Divide by 1 */
	{ C2_WRITE_DIRECT, 0xb1, 0x04 }, /* PLL0MUL = 0x04;  Multiply by 4 */
	{ C2_WRITE_DIRECT, 0xb2, 0x01 }, /* PLL0FLT = 0x01;  ICO = 100 MHz, PD clock 25 MHz */
	{ C2_WAIT_US, .time = 5 }, /* wait for 5 us */
	{ C2_WRITE_DIRECT, 0xb3, 0x03 }, /* PLL0CN = 0x03;   Enable the PLL */
	{ C2_WAIT_US, .time = 10000 }, /* wait for 10 ms for PLL to start */
	{ C2_READ_DIRECT, 0xb3 }, /* PLL0CN = ??;     read lock value */
	{ C2_WRITE_DIRECT, 0x8f, 0x04 }, /* CLKSEL = 0x04;   Set PLL as system clock */
	{ C2_WRITE_DIRECT, 0x84, 0xe6 }, /* CCH0CN = 0xe6;   0xe7 for block write enable; re-enable prefetch */
	{ C2_WRITE_DIRECT, 0xa7, 0x00 }, /* SFRPAGE = 0x00;  Set SFRPAGE to 0x0 */
#endif
};

static struct c2_setupcmd c2init_t61x[] = {
	{ C2_WRITE_DIRECT, 0xcf, 0x00 }, /* set SFRPAGE to 0x00 */
	{ C2_WRITE_DIRECT, 0xb2, 0x83 }, /* set intosc to 24.5 MHz */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f336[] = {
	{ C2_WRITE_DIRECT, 0xb2, 0x83 }, /* set intosc to 24.5 MHz */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f50x[] = {
//	{ C2_WRITE_SFR, 0xff, 0xa0 }, /* set VDD monitor to high threshold */
	{ C2_WRITE_SFR, 0xef, 0x00 }, /* disable VDD monitor as a reset source */
	{ C2_WRITE_SFR, 0xff, 0xa0 }, /* set VDD monitor to high threshold */
	{ C2_WAIT_US, .time = 100 }, /* wait 100 us for VDD monitor to stabilize */
	{ C2_WRITE_SFR, 0xef, 0x02 }, /* enable VDD monitor as a reset source */
	{ C2_WRITE_SFR, 0xa7, 0x0f }, /* set SFRPAGE to 0x0f */
	{ C2_WRITE_SFR, 0xa1, 0xc7 }, /* set OSCICN to 24 MHz */
	{ C2_WRITE_SFR, 0x8f, 0x00 }, /* set CLKSEL to intosc */
	{ C2_WRITE_SFR, 0xa7, 0x00 }, /* set SFRPAGE to 0x00 */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f92x[] = {
	{ C2_WRITE_DIRECT, 0xa7, 0x00 }, /* set SFRPAGE to 0x00 */
	{ C2_WRITE_DIRECT, 0xb2, 0x8F }, /* enable precision oscillator */
	{ C2_WRITE_DIRECT, 0xa9, 0x00 }, /* select precision oscillator divide by 1 as system clock source */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_t63x[] = {
	{ C2_WRITE_DIRECT, 0xcf, 0x00 }, /* set SFRPAGE to 0x00 */
	{ C2_WRITE_DIRECT, 0xb2, 0x83 }, /* set intosc to 24.5 MHz */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f90x[] = {
	{ C2_WRITE_DIRECT, 0xa7, 0x00 }, /* set SFRPAGE to 0x00 */
	{ C2_WRITE_DIRECT, 0xb2, 0x8F }, /* enable precision oscillator */
	{ C2_WRITE_DIRECT, 0xa9, 0x00 }, /* select precision oscillator divide by 1 as system clock source */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f58x[] = { // VDD monitor high; VREG high; INTOSC high; clksel INTOSC
//	{ C2_WRITE_DIRECT, 0xc9, 0x10 }, /* set VREG to high value */
//	{ C2_WRITE_DIRECT, 0xff, 0xa0 }, /* set VDD monitor to high threshold */
	{ C2_WRITE_SFR, 0xff, 0xa0 }, /* set VDD monitor to high threshold */
//	{ C2_WRITE_DIRECT, 0xa7, 0x0f }, /* set SFRPAGE to 0x0F */
//	{ C2_WRITE_DIRECT, 0xa1, 0xc7 }, /* set OSCICN to 0xC7 (24 MHz) */
//	{ C2_WRITE_DIRECT, 0xa7, 0x00 }, /* set SFRPAGE to 0x00 */
//	{ C2_WRITE_DIRECT, 0xb6, 0x10 }, /* set FLRT to >25 MHz operation */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f80x[] = {
//	{ C2_WRITE_SFR, 0xb2, 0x83 }, /* set OSCICN to 0x83 (24.5 MHz) */
	{ C2_DONE },
};

static struct c2_setupcmd c2init_f38x[] = {
	{ C2_WRITE_SFR, 0xb6, 0x90 }, /* Set Flash read timing for 50 MHz, one shot enabled */
	{ C2_WRITE_SFR, 0xff, 0x80 }, /* Enable VDD monitor */
	{ C2_WRITE_SFR, 0xef, 0x02 }, /* Enable VDDmon as a reset source */
	{ C2_WRITE_SFR, 0xb2, 0x83 }, /* set OSCICN to 0x83 (24.5 MHz) */
	{ C2_DONE },
};

struct c2family families [] = {
	{ 0x04, "C8051F30x",             C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_1, 0xb4, c2init_f30x },
	{ 0x08, "C8051F31x",             C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_f31x },
	{ 0x09, "C8051F32x",             C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_f32x },
	{ 0x0a, "C8051F33x",             C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_f33x },
	{ 0x0b, "C8051F35x",             C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_f35x },
	{ 0x0c, "C8051F41x",             C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_f41x },
	{ 0x0d, "C8051F326/7",           C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, NULL },
	{ 0x0e, "Si825x",                C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_si8250 },
	{ 0x0f, "C8051F34x",             C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xad, c2init_f34x },
	{ 0x10, "C8051T60x",               C2_MEM_TYPE_OTP,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_t60x },
	{ 0x11, "C8051F52x",             C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_f52x },
	{ 0x12, "C8051F36x",             C2_MEM_TYPE_FLASH, 1024, 0, C2_SECURITY_C2_2, 0xb4, c2init_f36x },
	{ 0x13, "C8051T61x",               C2_MEM_TYPE_OTP,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_t61x },
	{ 0x14, "C8051F336/7",           C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_f336 },
	{ 0x15, "Si8100",                C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, NULL },
	{ 0x16, "C8051F92x/93x",         C2_MEM_TYPE_FLASH, 1024, 0, C2_SECURITY_C2_2, 0xb4, c2init_f92x },
	{ 0x17, "C8051T63x",               C2_MEM_TYPE_OTP,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_t63x },
	{ 0x18, "C8051T62x",               C2_MEM_TYPE_OTP,  512, 0, C2_SECURITY_C2_2, 0xad, NULL },
	{ 0x19, "C8051T622/3",             C2_MEM_TYPE_OTP,  512, 0, C2_SECURITY_C2_2, 0xad, NULL },
	{ 0x1a, "C8051T624/5",             C2_MEM_TYPE_OTP,  512, 0, C2_SECURITY_C2_2, 0xad, NULL },
	{ 0x1b, "C8051T606",               C2_MEM_TYPE_OTP,  512, 0, C2_SECURITY_C2_2, 0xb4, NULL },
	{ 0x1c, "C8051F50x",             C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_f50x },
	{ 0x1d, "C8051F338POE",          C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, NULL },
	{ 0x1e, "C8051F70x/71x",         C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, NULL },
	{ 0x1f, "C8051F90x/91x",         C2_MEM_TYPE_FLASH,  512, 1, C2_SECURITY_C2_2, 0xb4, c2init_f90x },
	{ 0x20, "C8051F58x/59x",         C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_f58x },
	{ 0x21, "C8051F54x",             C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, NULL },
	{ 0x22, "C8051F54x/55x",         C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, NULL },
	{ 0x23, "C8051F80x/81x/82x/83x", C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, c2init_f80x },
	{ 0x24, "Si4010",                  C2_MEM_TYPE_OTP,  512, 0, C2_SECURITY_C2_2, 0xb4, NULL },
	{ 0x25, "C8051F99x",             C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, NULL },
	{ 0x26, "C8051F80x/81x/82x/83x", C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xb4, NULL },
	{ 0x28, "C8051F38x",             C2_MEM_TYPE_FLASH,  512, 0, C2_SECURITY_C2_2, 0xad, c2init_f38x },
};

static int process_setupcmd(struct c2tool_state *state,
			    struct c2_setupcmd *setupcmd)
{
	struct c2interface *c2if = &state->c2if;
	int res;

	switch (setupcmd->token) {
	case C2_DONE:
		return 1;
	case C2_WRITE_SFR:
		res = c2_write_sfr(c2if, setupcmd->reg, setupcmd->value);
		break;
	case C2_WRITE_DIRECT:
		res = c2_write_direct(state, setupcmd->reg, setupcmd->value);
		break;
	case C2_READ_SFR:
		res = c2_read_sfr(c2if, setupcmd->reg);
		break;
	case C2_READ_DIRECT:
		res = c2_read_direct(state, setupcmd->reg);
		break;
	case C2_WAIT_US:
		usleep(setupcmd->time);
		res = 0;
		break;
	default:
		res = -1;
		break;
	}

	return res;
}

int c2family_find(unsigned int device_id, struct c2family **family)
{
	unsigned int k;

	for (k = 0; k < ARRAY_SIZE(families); ++k) {
		if (families[k].device_id == device_id) {
			*family = &families[k];
			return 0;
		}
	}

	return -EINVAL;
}

int c2family_setup(struct c2tool_state *state)
{
	/* struct c2interface *c2if = &state->c2if; */
	struct c2family *family = state->family;
	int res;
	struct c2_setupcmd *setupcmd = family->setup;

	if (!setupcmd)
		return 0;

	while ((res = process_setupcmd(state, setupcmd++)) == 0) {}

	if (res == 1)
		return 0;
	else
		return -EIO;
}
