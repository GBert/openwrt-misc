Misc GPIO stuff OpenWRT
=======================
This Git contains miscellaneous stuff for GPIO handling and testing

gpio-test.c
-----------
simple GPIO kernel module

gpio-irq-test.c
---------------
simple GPIO IRQ kernel module testing 0047-GPIO-MIPS-ralink-add-gpio-driver-for-ralink-SoC.patch\_patch

gpio-toggle-test.c
------------------
simple GPIO toggling test (RT3052 & AR933x only)
```
 void __iomem *gpio_addr = NULL;
 void __iomem *gpio_setdataout_addr = NULL;
 void __iomem *gpio_cleardataout_addr = NULL;

 gpio_addr = ioremap(GPIO_START_ADDR, GPIO_SIZE);

 gpio_setdataout_addr   = gpio_addr + GPIO_OFFS_SET;
 gpio_cleardataout_addr = gpio_addr + GPIO_OFFS_CLEAR;
 mask = 1 << gpio;

 for(repeat=0;repeat<NR_REPEAT;repeat++) {
     __raw_writel(mask, gpio_setdataout_addr);
     __raw_writel(mask, gpio_cleardataout_addr);
 }
```

CPU           | toggle speed
--------------|-------------
RT3052@320MHz | 6.67 MHz
AR9331@400MHz | 7.69 MHz
AR9341@533MHz | 7.69 MHz

gpio-irq-latency-test.c
-----------------------
simple GPIO IRQ latency test

728-MIPS-ath79-add-gpio-irq.patch
---------------------------------
adds GPIO IRQ stuff for ATH79 based SoCs like AR933x, AR934x, QCA9533 ...

