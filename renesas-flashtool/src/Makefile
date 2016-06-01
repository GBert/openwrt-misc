# Copyright (C) 2008, 2010 DJ Delorie <dj@redhat.com>

# This file is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3, or (at your option) any later
# version.
# 
# This file is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this file; see the file COPYING3.  If not see
# <http://www.gnu.org/licenses/>.

prefix = ${HOME}/m32c/install
lib = ${prefix}/lib

FTDI = ../ftdi

CC = gcc
CFLAGS =  -Wall -MMD -g -I $(FTDI)
LDFLAGS = 
LIBS = 

FLIBS = $(FTDI)/static_lib/libftd2xx.a.0.4.* -ldl -lpthread
#FLIBS = $(FTDI)/ftd2xx.lib -lpthread
LFLIBS = -lftdi
#LFLIBS = ${HOME}/dl/libftdi-0.17/src/.libs/libftdi.a -lusb
ULIBS = -lusb

.PHONY : all serial

all : uflash sflash rx rxs rxusb eeprom
	@true
serial : sflash rxs
	@true

FMTS = read_elf.o read_srec.o read_ieee695.o
COMMON = mflash.o console.o $(FMTS)

RXOBJ = rx.o rxlib.o console.o $(FMTS)

rx : $(RXOBJ) libftdi.o
	$(CC) $(LDFLAGS) $(RXOBJ) libftdi.o $(LIBS) $(LFLIBS) -o $@ -lpthread

rxs : $(RXOBJ) serial.o
	$(CC) $(LDFLAGS) $(RXOBJ) serial.o $(LIBS) -o $@ -lpthread

rxusb : $(RXOBJ) rxusb.o
	$(CC) $(LDFLAGS) $(RXOBJ) rxusb.o $(ULIBS) -o $@ -lpthread

uflash : $(COMMON)  libftdi.o
	$(CC) $(LDFLAGS) $(COMMON) libftdi.o $(LIBS) $(LFLIBS) -o $@ -lpthread

sflash : $(COMMON) serial.o
	$(CC) $(LDFLAGS) $(COMMON) serial.o $(LIBS) -o $@ -lpthread

eeprom : eeprom.o
	$(CC) $(LDFLAGS) eeprom.o $(LIBS) $(LFLIBS) -o $@ -lpthread

clean : tidy
	-rm -rf uflash sflash rx rxs rxusb eeprom

tidy :
	-rm -rf *.o *~ *.d

TARFILES = \
	COPYING3 Makefile Makefile.mingw \
	mflash.c mflash.h \
	rx.c rxusb.c rxlib.c rxlib.h \
	eeprom.c factory-eeprom gr8c-eeprom grx-eeprom \
	read_*.c console.c serial.c d2xx.c libftdi.c \
	console.h elf.h ieee695.h serial.h

.PHONY: tar
tar : flash-tool.tar.gz ; @true;

flash-tool.tar.gz : $(TARFILES)
	tar cf - $(TARFILES) \
	| gzip -9 > flash-tool.tar.gz.tmp
	mv flash-tool.tar.gz.tmp flash-tool.tar.gz

D=$(wildcard *.d)
ifneq ($D,)
include $D
endif
