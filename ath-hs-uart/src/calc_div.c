
#include <stdint.h>
#include <stdio.h>
 
#define u64 uint64_t
#define u32 uint32_t
 
#define AR933X_UART_MAX_SCALE	0xff
#define AR933X_UART_MAX_STEP	0x3333
 

#define do_div(n,base) ({                                      \
         uint32_t __base = (base);                               \
         uint32_t __rem;                                         \
         __rem = ((uint64_t)(n)) % __base;                       \
         (n) = ((uint64_t)(n)) / __base;                         \
         __rem;                                                  \
}) 
 
static unsigned long ar933x_uart_get_baud(unsigned int clk, unsigned int scale, unsigned int step) {
        u64 t;
        u32 div;
 
        div = (2 << 16) * (scale + 1);
        t = clk;
        t *= step;
        t += (div / 2);
        do_div(t, div);
 
        return t;
}
 
static void ar933x_uart_get_scale_step(unsigned int clk, unsigned int baud, unsigned int *scale, unsigned int *step) {
        unsigned int tscale;
        long min_diff;
 
        *scale = 0;
        *step = 0;
 
        min_diff = baud;
        for (tscale = 0; tscale < AR933X_UART_MAX_SCALE; tscale++) {
                u64 tstep;
                int diff;
 
                tstep = baud * (tscale + 1);
                tstep *= (2 << 16);
                do_div(tstep, clk);
 
                if (tstep > AR933X_UART_MAX_STEP)
                        break;
 
                diff = abs(ar933x_uart_get_baud(clk, tscale, tstep) - baud);
                if (diff < min_diff) {
                        min_diff = diff;
                        *scale = tscale;
                        *step = tstep;
                }
        }
}
 
int main(void) {
	const uint64_t UARTCLOCK = 25000000;
		
	unsigned int baud = 2000000;
 
	unsigned int scale;
	unsigned int step;
 
	ar933x_uart_get_scale_step(UARTCLOCK, baud, &scale, &step);
	
	printf("Uart Clock: %15d\n"
		   "Uart Baud:  %15d\n"
		   "----------------------------\n"
		   "Uart Scale: %15d\n"
		   "Uart Step:  %15d\n", UARTCLOCK, baud, scale, step);
	
	unsigned long baud_real = ar933x_uart_get_baud(UARTCLOCK, scale, step);
 
	printf( "----------------------------\n"
		"Uart Baud:  %15lu\n", baud);
	return 0;
}
