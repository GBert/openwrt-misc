#!/usr/bin/env python
#
# HD44780 driver.
#

import os, sys, socket, struct, time
import gpio, traceback


# Definitions for pin connections

# hd44780 pin   logical GPIO    Omnima LED    
HD_RS         =      12       #     D6          
HD_RW         =      14       #     D7
HD_E          =      15       #     D8
HD_DB4        =      17       #     D9
HD_DB5        =      18       #     D10
HD_DB6        =      20       #     D11
HD_DB7        =      21       #     D12


# Change this to the ip address of your router.
router = gpio.Proxy("10.0.0.13")


def RequestPins(): 
  router.Request(HD_RS,  "HD_RS" )
  router.Request(HD_RW,  "HD_RW" )
  router.Request(HD_E,   "HD_E"  )
  router.Request(HD_DB4, "HD_DB4")
  router.Request(HD_DB5, "HD_DB5")
  router.Request(HD_DB6, "HD_DB6")
  router.Request(HD_DB7, "HD_DB7")


def FreePins():
  router.Free(HD_RS )
  router.Free(HD_RW )
  router.Free(HD_E  )
  router.Free(HD_DB4)
  router.Free(HD_DB5)
  router.Free(HD_DB6)
  router.Free(HD_DB7)


def EN_LOW():
  router.Set(HD_E, 0)
  
def EN_HIGH():
  router.Set(HD_E, 1)
  
def RS_LOW():
  router.Set(HD_RS, 0)
  
def RS_HIGH():
  router.Set(HD_RS, 1)

def Nibble(n):
  router.Set(HD_DB4, (n&0x1))
  router.Set(HD_DB5, (n&0x2)>>1)
  router.Set(HD_DB6, (n&0x4)>>2)
  router.Set(HD_DB7, (n&0x8)>>3)
  EN_LOW()
  EN_HIGH()
  

def Command(n):
  RS_LOW()
  Nibble((n>>4)&0xf)
  Nibble(n&0xf)


def Data(txt):
  RS_HIGH()
  for i in txt:
    n = ord(i)
    Nibble((n>>4)&0xf)
    Nibble(n&0xf)


def Init():
  router.Out(HD_RS, 0)
  router.Out(HD_RW, 0)   # always low - we only write
  router.Out(HD_E,  0)
  router.Out(HD_DB4, 0)
  router.Out(HD_DB5, 0)
  router.Out(HD_DB6, 0)
  router.Out(HD_DB7, 0)
  # RW to low

  # power on display
  EN_LOW()
  RS_LOW()
  Nibble(0x03)
  Nibble(0x03)
  Nibble(0x03)
  Nibble(0x02)
  
  Command(0x28)
  Command(0x0C)    # blinking off, cursor off
  Command(0x01)
  Command(0x06)


def DoStuff():
  Data("Hello")


if __name__ == "__main__":

  RequestPins()
  try:
    Init()
    DoStuff()
  except :
    traceback.print_exc(file=sys.stdout)

  print "Freeing pins..."    
  FreePins()
  print "Done."
