#c2tool (by Guntermann-Drunck)

**c2tool** is a linux userspace application meant to enable in-system
programming on low pin-count Silicon Labs devices using the Silicon Labs
2-Wire Interface (C2).

## Existing kernel driver

There is a linux kernel driver (drivers/misc/c2port) for this, **but**

* hardware support is quite limited
* sysfs interface is not very intuitive
* `local_irq_disable()` affects real time performance
* GPIOs can be easily accessed from userspace

##Challenge

The C2 Clock signal is shared with an active-low reset signal on the
target device. Any clock pulse taking longer than 20us is interpreted as a reset
signal. The kernel driver is driving the clock line with `local_irq_disable()`
pulled to make sure the timing is valid. But this degrades system performance.

**c2tool** depends on hardware support for generating clock pulses that are in
the range from 20ns to 5000ns. In our reference application there is an FPGA but
a discrete monoflop should also do.

## Install

Dependencies:

* libbfd
* libiberty

A simple "make" should be sufficient. More Makefile magic is welcome :)

##Prerequisites

Three gpios exported in `/sys/class/gpio` have to be supplied to **c2tool**:

* c2d: Read/write access to the C2 Data pin
* c2ck: Write access to the  C2 Clock pin
* c2stb: A falling edge triggers a defined pulse on C2 Clock pin

All gpios must be configured as outputs and set to value 1.

## Usage

```
Usage:  c2tool [options] <gpio c2d> <gpio c2ck> <gpio c2ckstb> command
        --version       show version (0.01)
Commands:
        info
                Show information about connected device.

        dump [offset] [len]
                Dump flash memory of connected device.

        flash [target <bfdname>] [adjust-start <incr>] <file>
                Write file to flash memory of connected device.

        erase
                Erase flash memory of connected device.

        verify [target <bfdname>] [adjust-start <incr>] <file>
                Verify file to flash memory of connected device.

        reset
                Reset connected device.
```

## C2 Documentation
* [C2 Interface Specification](http://www.silabs.com/Support%20Documents/TechnicalDocs/C2spec.pdf)
* [Silabs Application Note 127](http://www.silabs.com/Support%20Documents/TechnicalDocs/an127.pdf): Fash programming via the C2 interface
* [AN127 software](http://www.silabs.com/Support%20Documents/Software/AN127SW.zip)
