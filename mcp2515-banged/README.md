# Bit Banged MCP2515 driver

this code is an example how you should **not** write Linux kernel modules.
The CAN interface device driver is tightly interlocked with the GPIO SPI
bitbanging code. The interclock avoids task switching which speed up the whole
driver. Drawback: driver blocks CPU during reading the MCP2515. On high
CAN-Bus load the driver is claiming lots of CPU cycles.

## TODO

 * speed up bitbanging (tend to get ugly though)

### Outlook

Challenge: get the RX0BUF in 47 us starting at INT.
1/47us is the fastest CAN frame repeat rate possible - 1 MBit with
min 47 bits (no bit stuffing)

![SPI Performance](https://github.com/GBert/openwrt-misc/blob/master/mcp2515-banged/pictures/mcp2515_b_perf_04.png)

Sequence
- interrupt
- reading (0x03) CANINTF (0x2c) and EFLG ( ++ -> 0x2d)
- response buffer 0 is full ( 0x.. 0x.. 0x01 0x..)
- reading RXB0 buffer (0x90 ...)

### Performance

In this sequence the interrupt is cleared fast enough, because no data bytes needs to be read (DLC=0)

![SPI Performance](https://github.com/GBert/openwrt-misc/blob/master/mcp2515-banged/pictures/mcp2515_b_dlc_a_01.png)

Statistics for a fully saturated CAN-Bus at 250k:
```
root@ar9331:~# ip -s -d link show can0
4: can0: <NOARP,UP,LOWER_UP,ECHO> mtu 16 qdisc fq_codel state UNKNOWN mode DEFAULT group default qlen 10
    link/can  promiscuity 0
    can state ERROR-ACTIVE restart-ms 0
	  bitrate 250000 sample-point 0.875
	  tq 250 prop-seg 6 phase-seg1 7 phase-seg2 2 sjw 1
	  mcp2515-banged: tseg1 3..16 tseg2 2..8 sjw 1..4 brp 1..64 brp-inc 1
	  clock 8000000
	  re-started bus-errors arbit-lost error-warn error-pass bus-off
	  0          0          0          0          0          0
    RX: bytes  packets  errors  dropped overrun mcast
    6216354    5313978  0       0       0       0
    TX: bytes  packets  errors  dropped carrier collsns
    5          1        0       0       0       0
```
Errors and overrun: none :-)
