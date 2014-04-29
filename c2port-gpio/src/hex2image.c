/*
 * hex2image.c -- Convert an 8150 HEX file into binary form
 *
 * Copyright (C) 2007   Rodolfo Giometti <giometti@linux.it>
 * Copyright (C) 2007   Eurotech S.p.A. <support@eurotech.it>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

/* --- Misc macros ---------------------------------------------------------- */

#define NAME            "hex2image"
#define VERSION         "0.10.0"

#ifdef DEBUG
   #define CDEBUG(code) do { code } while (0)
   #define PDEBUG(fmt, args...) fprintf(stderr, "%s:%s[%d]: " fmt "\n", \
                                        NAME, __FILE__, __LINE__ , ## args)
   #define PINFO(fmt, args...) fprintf(stderr, "%s:%s[%d]: " fmt "\n", \
                                       NAME, __FILE__, __LINE__ , ## args)
   #define PERR(fmt, args...) fprintf(stderr, "%s:%s[%d]: " fmt "\n", \
                                      NAME, __FILE__, __LINE__ , ## args)
   #define PDATA(fmt, args...) printf("%s:%s[%d]: " fmt "\n", \
                                      NAME, __FILE__, __LINE__ , ## args)
#else   /* DEBUG */
   #define CDEBUG(code) /* do nothing! */
   #define PDEBUG(fmt, args...) /* do nothing! */
   #define PINFO(fmt, args...) fprintf(stderr, "%s: " fmt "\n", NAME , ## args)
   #define PERR(fmt, args...) fprintf(stderr, "%s: " fmt "\n", NAME , ## args)
   #define PDATA(fmt, args...) printf("%s: " fmt "\n", NAME , ## args)
#endif   /* DEBUG */


/* --- Local functions ------------------------------------------------------ */

#define str2hex(b1, b0) ({ 						\
	b1 = tolower(b1);						\
	b0 = tolower(b0);						\
	if (b1 >= 'a' && b1 <= 'f')					\
		b1 -= 'a' - 10;						\
	else								\
		b1 -= '0';						\
	if (b0 >= 'a' && b0 <= 'f')					\
		b0 -= 'a' - 10;						\
	else								\
		b0 -= '0';						\
	b1*16 + b0;							\
})

#if 0
unsigned char compute_checksum(const unsigned char buf[], int num)
{
	unsigned char checksum = 0;
	int i;

	for (i = 0; i < num; i++)
		checksum += buf[i];

	return 0 - checksum;
}
#endif

void print_version(void)
{
	fprintf(stderr,
		"%s - HEX to binary conversion - version %s\n", NAME, VERSION);
	fprintf(stderr,
		"Copyright (C) 2007 Rodolfo Giometti <giometti@linux.it>\n");
}

void print_help(void)
{
	print_version();
	fprintf(stderr,
		"\nUsage: %s [OPTIONS]\n"
		"where OPTIONS are:\n"
		"  -h,  --help            Print help information\n"
		"  -v,  --version         Print version information\n"
		"  -S,  --flash-size      Sets the flash size in bytes\n",
		NAME);
}


/* --- Main ----------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	char *line = NULL;
	int len = 0, num = 0, n, rlen, type;
	unsigned char am, al, checksum;
	unsigned int addr;

	int flash_size = 0;

	int ch, option_index;
	static struct option long_options[] = {
		{"flash-size",	1, 0, 'S'},
		{"help",	0, 0, 'h'},
		{"version",	0, 0, 'v'},
		{ 0,		0, 0,  0 }
	};

	unsigned char *ptr;

	int ret;

	/* Process command line arguments */
	while (1) {
		ch = getopt_long(argc, argv, "S:hv",
			long_options, &option_index);
    
		if (ch == -1)
			break;
    
		switch (ch) {
		case 'S' :	/* flash-size */
			ret = sscanf(optarg, "%d", &flash_size);
			if (ret !=1) {
				PERR("set-control: Invalid value");
				exit(EXIT_FAILURE);
			}
			break;

		case 'h' :		/* help */
			print_help();
			exit(EXIT_SUCCESS);
			break;

		case 'v' :		/* version */
			print_version();
			exit(EXIT_SUCCESS);
			break;

		default :
			print_help();
			exit(EXIT_FAILURE);
		}
	}

	/* Sanity checks */
	if (flash_size == 0) {
		PERR("flash size unknow");
		exit(EXIT_FAILURE);
	}

	/* Build the flash image */
	ptr = (unsigned char *) malloc(flash_size);
	if (!ptr) {
		PERR("unable to allocate memory for flash image");
		exit(EXIT_FAILURE);
	}
	memset(ptr, 0xff, flash_size);

	/* Do the job */
	while (1) {
		/* Read ASCII strings from stdin */
		n = getline(&line, &len, stdin);
		if (n < 0) {
			if (feof(stdin))
				break;   /* EOF */

			PERR("error reading input data file");
			return -1;
		}
		n = strlen(line)-1;
		if (n >= 0)
			line[n] = '\0';
		num++;

		PDEBUG("[%05d]> %s", num, line);

		if (line[0] != ':') {
			PDEBUG("no data line, skipped");
			continue;
		}

		rlen = str2hex(line[1], line[2]);
		if (rlen > 16) {
			PERR("record too long at line %d", num);
			return -1;
		}
		am = str2hex(line[3], line[4]);
		al = str2hex(line[5], line[6]);
		addr = (am<<8)+al;
		if (addr >= flash_size) {
			PERR("invalid addres \"%x\"", addr);
			exit(EXIT_FAILURE);
		}
		type = str2hex(line[7], line[8]);
		checksum = str2hex(line[9+rlen*2], line[10+rlen*2]);

		PDEBUG("len %d - addr %04x - type %s - checksum %02x",
			rlen, addr, type == 0 ? "data" : "end", checksum);

		if (type == 1) {
			PDEBUG("end");
			break;
		}

		/* Save the data */
		for (n = 0; n < rlen; n++)
			ptr[addr+n] = str2hex(line[9+n*2], line[10+n*2]);

#if 0
		/* FIXME: should check checksum??? */
		checksum = compute_checksum(&wbuf[2], rlen+5);
#endif

#ifndef DEBUG
		fprintf(stderr, ".");
		fflush(stdout);
#endif
	}
#ifndef DEBUG
		fprintf(stderr, "\n");
#endif

	/* Write the flash image to the stdout */
	ret = fwrite(ptr, flash_size, 1, stdout);
	if (ret < 0) {
	   PERR("cannot write flash image");
	   return -1;
	}

	return 0;
}
