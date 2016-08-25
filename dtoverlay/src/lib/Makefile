#
# This is not a complete Makefile of itself.  Instead, it is designed to
# be easily embeddable into other systems of Makefiles.
#
#LIBFDT_soname = libfdt.$(SHAREDLIB_EXT).1
#LIBFDT_INCLUDES = fdt.h libfdt.h libfdt_env.h
#LIBFDT_VERSION = version.lds
#LIBFDT_SRCS = fdt.c fdt_ro.c fdt_wip.c fdt_sw.c fdt_rw.c fdt_strerror.c fdt_empty_tree.c fdt_addresses.c
#LIBFDT_OBJS = $(LIBFDT_SRCS:%.c=%.o)

TARGET = libfdt_my.a
LIBFDT_SRCS  = fdt.c fdt_empty_tree.c fdt_ro.c fdt_rw.c fdt_sw.c fdt_strerror.c fdt_wip.c
LIBOBJS = $(LIBFDT_SRCS:%.c=%.o)

%.o: %.c
#	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) -c $<
	$(CC) $(CFLAGS) -c $<

all: $(TARGET)

$(TARGET): $(LIBOBJS)
	$(AR) rvs $@ $+
#        cp cs2parse.h $(INCLUDE_PATH)
#        cp write_cs2.h $(INCLUDE_PATH)
#        cp cs2_types.h $(INCLUDE_PATH)

#fdt_empty_tree.o: fdt_empty_tree.c libfdt.h

#cs2p_destroy.o: cs2p_destroy.c cs2parse.h

#cs2p_init.o: cs2p_init.c cs2parse.h

#cs2p_parse.o: cs2p_parse.c cs2parse.h

#write_cs2.o: write_cs2.c write_cs2.h

clean:
	rm -f $(TARGET)
	rm -f $(LIBOBJS)


