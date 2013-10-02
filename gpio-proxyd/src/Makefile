# $Id$

all: gpio-proxyd

%.o: %.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $^

gpio-proxy: gpio-proxyd.o
	$(CC) -o $@ $^

clean:
	rm -f *.o *~ gpio-proxyd
