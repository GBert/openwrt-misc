UNAME := $(shell uname)
CFLAGS +=  -D_REENTRANT -I../sml/include/ 
OBJS = snmp.o sml_snmp.o sml_server.o
LIBSML = ../sml/lib/libsml.a

ifeq ($(UNAME), Linux)
LIBS = -lm -lpthread
endif

sml_server : $(OBJS) $(LIBSML)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) $(LIBSML) -o sml_server

%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	@rm -f *.o
	@rm -f sml_server
