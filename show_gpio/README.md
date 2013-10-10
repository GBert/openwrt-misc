show\_gpio
==========

little tool to show PIN assigment on TL-WR841N(D):
<pre><code>
root@OpenWrt:~# show-gpio 
   JP2 pin 9 GPIO 00 I GPIO
   JP2 pin 3 GPIO 01 I GPIO
   JP2 pin 5 GPIO 02 O GPIO
   JP2 pin 7 GPIO 03 I GPIO
             GPIO 04 O GPIO
             GPIO 05 O SPI_CS_0
             GPIO 06 O SPI_CLK
             GPIO 07 O SPI_MOSI
             GPIO 08 I SPI_MISO
   JP1 pin 2 GPIO 09 I UART0\_SIN
   JP1 pin 1 GPIO 10 O UART0\_SOUT
             GPIO 11 O GPIO
       LED 1 GPIO 12 O GPIO
       LED 2 GPIO 13 O GPIO
       LED 3 GPIO 14 O GPIO
       LED 4 GPIO 15 O GPIO
      Switch GPIO 16 I GPIO
      Button GPIO 17 I GPIO
       LED 5 GPIO 18 O GPIO
       LED 6 GPIO 19 O GPIO
       LED 7 GPIO 20 O GPIO
       LED 8 GPIO 21 O GPIO
             GPIO 22 O GPIO
</pre></code>
