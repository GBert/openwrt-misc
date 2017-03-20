BINS := omega2-ctrl

BIN = $@

all: $(BINS)

CFLAGS+=-Wall -pedantic -std=gnu99 -g

omega2-ctrl: gpiomux.o refclk.o main.o
	$(CC) $(CFLAGS) -o omega2-ctrl gpiomux.o refclk.o main.o

clean:
	$(RM) *.o $(BINS)
