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
simple GPIO toggling test (AR933x only)

<pre><code>
  void \_\_iomem \*set\_reg = ath79\_gpio\_base + AR71XX\_GPIO\_REG\_SET;
  void \_\_iomem \*clear\_reg = ath79\_gpio\_base + AR71XX\_GPIO\_REG\_CLEAR;
  uint32\_t mask, repeat;
  for(repeat=0;repeat\<10000000;repeat++)
  {
    \_\_raw\_writel(mask, set\_reg);
    \_\_raw\_writel(mask, clear\_reg);
}
</pre></code>
AR9331@400MHz -> 7.69 MHz


0047-GPIO-MIPS-ralink-add-gpio-driver-for-ralink-SoC.patch\_patch
-----------------------------------------------------------------
adds Ralink pinctrl glue code to free GPIO. See: https://dev.openwrt.org/ticket/14309


728-MIPS-ath79-add-gpio-irq.patch
---------------------------------
adds GPIO IRQ stuff for ATH79 based SoCs like AR933x, AR934x, QCA9533 ...

