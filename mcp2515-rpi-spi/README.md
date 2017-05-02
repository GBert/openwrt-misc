# Combined MCP2515 RPi SPI driver

this code is an example how you should **not** write Linux kernel modules.
The CAN interface device driver is tightly interlocked with the SPI code.
The interclock avoids task switching which speed up the whole driver.
Drawback: driver blocks CPU during reading the MCP2515.

