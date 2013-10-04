#!/usr/bin/env python
#
# This should work on most adm5120 based routers and flash LEDs on and off.
#

import os, sys, socket, struct, time
import gpio


# Change this to the ip address of your router.
router = gpio.Proxy("10.0.0.13")


def Enumerate():
  # Enumerate all unassigned IO pins
  avail = []
  for i in xrange(0,50):
    try:
      router.Request(i, "TEMP_PIN")
      print "Got pin %d" % i
      avail.append(i)
    except IOError:
      pass
    finally:
      router.Free(i)
  return avail
      


if __name__ == "__main__":

  pins = Enumerate()

  for i in pins:
    print "Pin: %d" % i
    router.Request(i,"TEMP_PIN")
    router.Out(i, 0)
    for j in xrange(0,10):
      router.Set(i,j % 2)
      time.sleep(0.1)
    router.Free(i)

