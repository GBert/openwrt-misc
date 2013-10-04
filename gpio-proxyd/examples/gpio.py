#!/usr/bin/env python
"""
   Helper module for gpio-proxy.

   bifferos@yahoo.co.uk  9.6.2008
"""


import os, sys, socket, struct, time

class Error(Exception) :
  def __init__(self, val) :
    self.args = val


class Proxy :
  "Class to manage communication with the router"
  def __init__(self, host, port=5122) :
    self.host = host
    self.port = port
    self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0 )
    self.sock.bind( ("",5122) )

  def Transmit( self, operation, rval, wval, pin, data="" ) :
    if not data.endswith("\x00"):
      data += "\x00"
    pkt = struct.pack("<cBxBL", operation, 0xff, wval, pin) + data
    self.sock.sendto(pkt, (self.host, self.port))
    # Block for response of correct length
    buffer, host = self.sock.recvfrom(len(pkt))
    op, err, val = struct.unpack("<cBBx", buffer[:4])
    if err !=0 :
      print op,val
      raise Error([err])
    return val

  def Get( self, pin ) :
    "Read the specified pin value"	
    val = self.Transmit("G", 0, 0, pin)
    return struct.unpack("<L", value)[0]
    
  def Set( self, pin, value ) :
    "Write to the specified pin"
    self.Transmit("S", 0, value, pin)
    
  def Request( self, pin, name ) :
    "Request an IO pin"
    val = self.Transmit("R", 0, 0, pin, name)
    if val != 1:
      raise IOError, "Pin %d not available" % pin

  def Free( self, pin ) :
    "Free an IO pin"
    self.Transmit("F", 0, 0, pin)
    
  def In(self, pin) :
    "Set an IO pin to act as input"
    val = self.Transmit("I", 0, 0, pin)
    if val != 1:
      raise IOError, "Pin %d cannot be configured as input" % pin

  def Out(self, pin, initial) :
    "Set an IO pin to act as output, providing an initial value"
    val = self.Transmit("O", 0, initial, pin)
    if val != 1:
      raise IOError, "Pin %d cannot be configured as input" % pin



if __name__ == "__main__":

  # Address of the router
  adm = Proxy("10.0.0.44")

