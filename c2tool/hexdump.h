#ifndef __HEXDUMP__
#define __HEXDUMP__

enum {
	DUMP_PREFIX_NONE,
	DUMP_PREFIX_ADDRESS,
	DUMP_PREFIX_OFFSET
};
extern void hex_dump_to_buffer(const void *buf, size_t len,
			       int rowsize, int groupsize,
			       char *linebuf, size_t linebuflen, int ascii);
extern void print_hex_dump(const char *prefix_str, int prefix_type,
			   unsigned int addr, int rowsize, int groupsize,
			   const void *buf, size_t len, int ascii);
extern void print_hex_dump_bytes(const char *prefix_str, int prefix_type,
				 unsigned int addr, const void *buf, size_t len);

#endif
