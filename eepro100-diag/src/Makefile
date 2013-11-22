#

all:	eepro100-diag

show-gpio: eepro100-diag.o
	$(CC) $(LDFLAGS) -O -o $@ $^ $(LIBS)

clean:
	rm -f *.o eepro100-diag
	rm -f tags cscope.*
