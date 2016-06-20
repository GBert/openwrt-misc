BINS := sunxi_candumper

CFLAGS = -g -Wall -pedantic
BIN = $@

%.o : %.c
	$(CC) $(CFLAGS) -o $@

all:	$(BINS)

clean:
	$(RM) $(BINS)

