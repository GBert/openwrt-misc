CAN IO check
============

```
/* PH20 select Connector3 Pin
* bit 18:16 100 CAN_TX 16
* bit 19 -
* PH21 select
* bit 22:20 100 CAN_RX 18
* */

#define PIO_BASE 0x01c20800
#define PI_CFG0_OFFSET 0x120
#define PI_CFG1_OFFSET 0x124
#define PI_CFG2_OFFSET 0x128
#define PI_CFG3_OFFSET 0x12c
#define PI_DATA_OFFSET 0x130

GPIO_BASE=(unsigned long)ioremap(PIO_BASE,0x400);
PI_CFG = (unsigned long *)(GPIO_BASE + PI_CFG2_OFFSET);
PI_DATA= (unsigned long *)(GPIO_BASE + PI_DATA_OFFSET);
writel(0x00000000,PI_CFG); //set port i as input

void __iomem *gpio_base = ioremap(PIO_BASE, 0x400);
void __iomem * port_h_config = gpio_base + PH_CFG2_OFFSET;
/* 0xf1c20800 + 0x128 = 0xf1c20928 */

tmp = readl(port_h_config);
tmp &= 0xFF00FFFF;
writel(tmp | (4 << 16) | (4 << 20), port_h_config);
              0x40000  |  0x400000 = 0x440000

io -4 0xf1c20928  # -> 0x..44....

/* ############################################################ */
/* clocking */
writel(readl(0xF1C20000 + 0x6C) | (1 << 4), 0xF1C20000 + 0x6C);

io -4 0xf1c2006c  # -> (1<<4) -> 0x10

```



http://sourceforge.net/p/can4linux/code/HEAD/tree/trunk/can4linux/bananapi.c

https://www.kernel.org/doc/Documentation/devicetree/bindings/net/can/sja1000.txt

