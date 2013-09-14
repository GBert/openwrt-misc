/*
 * Atheros High Speed UART driver
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/byteorder.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/bitops.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/console.h>

#include <atheros.h>

typedef struct {
	struct tty_driver	*tty;
	char			buf[1024];
	unsigned 		put,
				get;
} ath_hs_uart_softc_t;

ath_hs_uart_softc_t	ath_hs_uart_softc;

void ath_hs_uart_init(void)
{
	extern uint32_t serial_inited;
	u_int32_t data, serial_clk, clk_step, clk_scale;

	/*
	 * Formula to calculate clk_step and clk_scale
	 * temp = (((long)serial_clk)*1310)/131072;
	 * clk_scale = (temp / (baud_rate));
	 * temp = (131072*((long)baud_rate));
	 * clk_step = (temp / (serial_clk)) * (clk_scale + 1);
	 */

	// UART1 Out of Reset
	ath_reg_wr(ATH_RESET,
		ath_reg_rd(ATH_RESET) & ~RST_RESET_UART1_RESET_MASK);

	// Set UART to operate on 100 MHz
	ath_reg_rmw_set(SWITCH_CLOCK_SPARE_ADDRESS,
		SWITCH_CLOCK_SPARE_UART1_CLK_SEL_SET(1));

	serial_clk = 100 * 1000 * 1000;

	//clk_scale = ((serial_clk / 128k) * 1310) / baudrate
	clk_scale = ((serial_clk >> 17) * 1310) / ATH_HS_UART_BAUD;

	//clk_step = ((128k * 115200 * (clk_scale + 1)) / serial_clk)
	// Splitting 128K as 128 * 1024
	clk_step = ((128 * (ATH_HS_UART_BAUD / 100) * (clk_scale + 1)) << 10)
			/ (serial_clk / 100);

	ath_reg_wr(ATH_HS_UART_INT_EN, 0x1);

	// GPIO Settings for HS_UART
	// Enabling UART1_TD as output on GPIO10
	data = ath_reg_rd(ATH_GPIO_OUT_FUNCTION2);
	data = (data & ~GPIO_OUT_FUNCTION2_ENABLE_GPIO_10_MASK) |
		GPIO_OUT_FUNCTION2_ENABLE_GPIO_10_SET(0x4f);
	ath_reg_wr(ATH_GPIO_OUT_FUNCTION2, data);

	data = ath_reg_rd(ATH_GPIO_OUT_FUNCTION3);
	data = (data & ~GPIO_OUT_FUNCTION3_ENABLE_GPIO_12_MASK) |
		GPIO_OUT_FUNCTION3_ENABLE_GPIO_12_SET(0x50);
	ath_reg_wr(ATH_GPIO_OUT_FUNCTION3, data);

	// Enabling UART1_TD, UART1_RTS as outputs on GPIO10,12
	data = ath_reg_rd(ATH_GPIO_OE);
	data = (data & 0xffffebff) | 0xa00;
	ath_reg_wr(ATH_GPIO_OE, data);

	// Enabling UART1_RD,UART1_CTS as inputs on GPIO 9, 11
	ath_reg_wr(ATH_GPIO_IN_ENABLE9, 0xb090000);

	// GPIO_IN_ENABLE9
	// CLOCK Settings
	data = ath_reg_rd(ATH_HS_UART_CLK);
	data = (data & 0xff000000) | clk_step | (clk_scale << 16);
	ath_reg_wr(ATH_HS_UART_CLK, data);

	ath_reg_wr(ATH_HS_UART_CS, 0x2188);

	serial_inited = 1;
	ath_sys_frequency();
	printk("step 0x%x scale 0x%x\n", clk_step, clk_scale);
}


static void
ath_hs_uart_console_write(struct console *co, const char *s, unsigned int count)
{
	extern void UartPut(u8);
	unsigned int i;

	for (i = 0; i < count; i++, s++) {
		if (*s == '\n')
			UartPut('\r');
		UartPut(*s);
	}
}

static int __init ath_hs_uart_console_setup(struct console *co, char *options)
{
	return 0;
}

static int ath_hs_uart_console_early_setup(void)
{
	return update_console_cmdline("uart", 0, "ttyS", 0, NULL);
}

struct tty_driver *ath_hs_uart_console_device(struct console *co, int *index)
{
	*index = 0;
	return ((ath_hs_uart_softc_t *)co->data)->tty;
}


static struct console ath_hs_uart_console = {
	.name		= "ttyS",
	.write		= ath_hs_uart_console_write,
	.device		= ath_hs_uart_console_device,
	.setup		= ath_hs_uart_console_setup,
	.early_setup	= ath_hs_uart_console_early_setup,
	.flags		= CON_PRINTBUFFER | CON_ENABLED | CON_CONSDEV,
	.index		= -1,
	.data		= &ath_hs_uart_softc,
};

static int ath_hs_uart_open(struct tty_struct *tty, struct file *filp)
{
	if (tty->index >= tty->driver->num)
		return -ENODEV;
	return 0;
}

static void ath_hs_uart_close(struct tty_struct *tty, struct file *filp)
{
	return;
}

#define hs_get_char(sc)				\
({						\
	unsigned char c =			\
	sc->buf[sc->get % sizeof(sc->buf)];	\
	sc->get ++;				\
	c;					\
})

#define hs_set_char(sc, c)			\
do {						\
	sc->buf[sc->put % sizeof(sc->buf)] = c;	\
	sc->put ++;				\
} while(0)

static int ath_hs_uart_put_char(struct tty_struct *tty, unsigned char ch)
{
	ath_hs_uart_softc_t	*sc = tty->driver->driver_state;

	hs_set_char(sc, ch);

	return 1;
}

static int ath_hs_uart_chars_in_buffer(struct tty_struct *tty)
{
	ath_hs_uart_softc_t	*sc = tty->driver->driver_state;

	return sc->put - sc->get;
}

static int ath_hs_uart_write_room(struct tty_struct *tty)
{
	ath_hs_uart_softc_t	*sc = tty->driver->driver_state;

	return sizeof(sc->buf) - ath_hs_uart_chars_in_buffer(tty);
}

static void ath_hs_uart_flush_chars(struct tty_struct *tty)
{
	extern void UartPut(u8);
	ath_hs_uart_softc_t	*sc = tty->driver->driver_state;

	while (sc->get < sc->put) {
		UartPut(hs_get_char(sc));
	}
	return;
}

static int
ath_hs_uart_ioctl(struct tty_struct *tty, struct file *filp, unsigned int cmd,
	   unsigned long arg)
{
	switch (cmd) {
	case TCFLSH:
		tty_buffer_flush(tty);
		break;
	case TCGETS:
	case TCSETS:
		// allow the default handler to take care
		return -ENOIOCTLCMD;
	default:
		printk("%s: cmd = 0x%x\n", __func__, cmd);
	}
	return 0;
}

static int ath_hs_uart_break_ctl(struct tty_struct *tty, int break_state)
{
	printk("%s called\n", __func__);
	return 0;
}

static void ath_hs_uart_wait_until_sent(struct tty_struct *tty, int timeout)
{
	printk("%s called\n", __func__);
	return;
}

static int ath_hs_uart_write(struct tty_struct *tty,
			const unsigned char *buf, int count)
{
	int			i;
	ath_hs_uart_softc_t	*sc = tty->driver->driver_state;

	if (count > (i = ath_hs_uart_write_room(tty))) {
		count = i;
	}

	for (i = 0; i < count; i++) {
		hs_set_char(sc, buf[i]);
	}

	ath_hs_uart_flush_chars(tty);
	tty_wakeup(tty);
	return i;
}

static const struct tty_operations ath_hs_uart_ops = {
	.open		= ath_hs_uart_open,
	.close		= ath_hs_uart_close,
	.write		= ath_hs_uart_write,
	.put_char	= ath_hs_uart_put_char,
	.flush_chars	= ath_hs_uart_flush_chars,
	.write_room	= ath_hs_uart_write_room,
	.chars_in_buffer= ath_hs_uart_chars_in_buffer,
	.ioctl		= ath_hs_uart_ioctl,
	.break_ctl	= ath_hs_uart_break_ctl,
	.wait_until_sent= ath_hs_uart_wait_until_sent,
};

static irqreturn_t ath_hs_uart_isr(int irq, void *dev_id)
{
	extern u8 UartGetPoll(void);
	ath_hs_uart_softc_t	*sc = dev_id;

	tty_insert_flip_char(sc->tty->ttys[0], UartGetPoll(), TTY_NORMAL);
	ath_reg_wr(ATH_HS_UART_INT_STATUS, 0xffffffff);
	tty_flip_buffer_push(sc->tty->ttys[0]);
	return IRQ_HANDLED;
}

static int __init ath_hs_uart_user_init(void)
{
	int ret;
	struct tty_driver *tty;

	printk("Serial: Atheros High-Speed UART\n");

	tty = alloc_tty_driver(1);
	tty->owner		= THIS_MODULE;
	tty->driver_name	= "Atheros hs-uart";
	tty->num		= 1;
	tty->name		= "ttyS";
	tty->major		= TTY_MAJOR;
	tty->minor_start	= 64;
	tty->type		= TTY_DRIVER_TYPE_SERIAL;
	tty->subtype		= SERIAL_TYPE_NORMAL;
	tty->init_termios	= tty_std_termios;
	tty->init_termios.c_cflag = B115200 | CS8 | CREAD | HUPCL | CLOCAL;
	tty->init_termios.c_ispeed = tty->init_termios.c_ospeed = ATH_HS_UART_BAUD;
	tty->flags		= TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
	tty->driver_state	= &ath_hs_uart_softc;

	tty_set_operations(tty, &ath_hs_uart_ops);

	ret = tty_register_driver(tty);
	if (ret < 0) {
		put_tty_driver(tty);
		printk("tty_register_driver failed %d\n", ret);
		return ret;
	}

	ath_hs_uart_softc.tty = tty;

	if ((ret = request_irq(ATH_MISC_IRQ_HS_UART, ath_hs_uart_isr,
			IRQF_SHARED, "hs-uart", &ath_hs_uart_softc)) < 0) {
		printk("request irq failed (%d)\n", ret);
		put_tty_driver(tty);
		tty_unregister_driver(tty);
		return ret;
	}

	/* Disable the normal UART interrupt */
	ath_reg_rmw_clear(ATH_MISC_INT_MASK, MIMR_UART);

	return ret;
}

module_init(ath_hs_uart_user_init);

static int __init ath_hs_uart_console_init(void)
{
	register_console(&ath_hs_uart_console);
	return 0;
}
console_initcall(ath_hs_uart_console_init);
