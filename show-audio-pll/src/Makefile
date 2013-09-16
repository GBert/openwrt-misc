#
# Copyright (C) 2012 Žilvinas Valinskas
# See LICENSE for more information.
#

all:	show-audio-pll

libmmio.o: libmmio.c
	$(CC) $(CFLAGS) -c libmmio.c -o libmmio.o

utils.o: utils.c
	$(CC) $(CFLAGS) -c utils.c -o utils.o

strbuf.o : strbuf.c
	$(CC) $(CFLAGS) -c strbuf.c -o strbuf.o

mmio: mmio.o utils.o strbuf.o libmmio.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

show-audio-pll: show-audio-pll.o utils.o strbuf.o libmmio.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o show-audio-pll
	rm -f tags cscope.*
