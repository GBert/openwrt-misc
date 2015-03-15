CC    ?= gcc
CFLAGS ?= -O2 -g
#CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -g

RM  = rm -f

.SUFFIXES:
.SUFFIXES: .c .o

EXE = c2tool

OBJ = c2tool.o c2interface.o c2family.o hexdump.o version.o
OBJ += info.o dump.o flash.o erase.o verify.o reset.o

CFLAGS    += -Wall

.PHONY: all
all: $(EXE)

c2tool: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -lbfd -liberty -ldl -o $@

VERSION_OBJS := $(filter-out version.o, $(OBJS))

version.c: version.sh $(patsubst %.o,%.c,$(VERSION_OBJS)) c2tool.h Makefile \
		$(wildcard .git/index .git/refs/tags)
	$(Q)./version.sh $@

.PHONY: clean
clean:
	${RM} *~ *\# *.o

.PHONY: distclean
distclean: clean
	${RM} $(EXE)