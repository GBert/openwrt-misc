PROGRAMS = dtmerge dtoverlay

all: $(PROGRAMS)

# CFLAGS+=-Wall -Wextra -Wmaybe-uninitialized -pedantic-errors -g
CFLAGS+=
LIBPATH = -L./lib -I .

dtmerge: dtmerge.o
	$(CC) $(LIBPATH) $(CFLAGS) -o dtmerge dtmerge.o -lfdt_my

dtoverlay: dtoverlay_main.o utils.o
	$(CC) $(LIBPATH) $(CFLAGS) -o dtoverlay dtoverlay_main.o utils.o -lfdt

clean:
	rm -rf *.o dtmerge dtoverlay

