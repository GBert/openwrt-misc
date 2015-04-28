# Bit Banged MCP2515 driver

this code is an example how you should **not** write Linux kernel modules.
The CAN interface device driver is tightly interlocked with the GPIO SPI
bitbanging code. The interclock avoids rescheduling and speed up the whole
driver.

## TODO

 * fix exit code
 * speed up bitbanging
 * speed up reading RXxBUF (only DLC bytes)

### Outlook

Challenge: get the RX0BUF in 47 us starting at INT.
1/47us is the fastest CAN frame repeat rate possible - 1 MBit with
min 47 bits (no bit stuffing)

![SPI Performance](https://github.com/GBert/openwrt-misc/blob/master/mcp2515-banged/pictures/mcp2515_b_perf_04.png)

Sequence
- interrupt
- reading (0x03) CANINTF (0x2c) EFLG
- response buffer 0 is full ( 0x.. 0x.. 0x01 0x..)
- reding RXB0 buffer

![SPI Performance](https://github.com/GBert/openwrt-misc/blob/master/mcp2515-banged/pictures/mcp2515_b_perf_03.png)

The interrupt is cleared after ~60us - close to 47us.
