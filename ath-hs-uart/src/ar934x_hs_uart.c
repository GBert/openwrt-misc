/*
 *  Atheros AR934X SoC built-in HS UART driver
 *
 *  Copyright (C) 2011 Gabor Juhos <juhosg@openwrt.org>
 *   errors added by Gerhard Bertelsmann <info@gerhard-bertelsmann.de>
 *
 *  Based on drivers/char/serial.c, by Linus Torvalds, Theodore Ts'o.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/clk.h>

#include <asm/div64.h>

#include "ar934x_hs_uart.h"

#undef CONFIG_SERIAL_AR934X_HS_CONSOLE
#define CONFIG_SERIAL_AR934X_NR_UARTS	1

#define DRIVER_NAME "ar934x-hs-uart"

#define AR934X_HS_UART_MAX_SCALE	0xff
/* #define AR934X_HS_UART_MAX_STEP		0xffff */
/* timing with 0x3333 should be more accurate */
#define AR934X_HS_UART_MAX_STEP		0x3333

#define AR934X_HS_UART_MIN_BAUD		300
#define AR934X_HS_UART_MAX_BAUD		30000000

#define AR934X_HS_DUMMY_STATUS_RD	0x01

#define dprintk(args...) \
    do { \
       printk(KERN_INFO " ar934x-hs-uart : " args); \
    } while (0)


static struct uart_driver ar934x_hs_uart_driver;

struct ar934x_hs_uart_port {
	struct uart_port	port;
	unsigned int		ier;	/* shadow Interrupt Enable Register */
	unsigned int		min_baud;
	unsigned int		max_baud;
	struct clk		*clk;
};

static inline bool ar934x_hs_uart_console_enabled(void)
{
	return config_enabled(CONFIG_SERIAL_AR934X_HS_CONSOLE);
}

static inline unsigned int ar934x_hs_uart_read(struct ar934x_hs_uart_port *up,
					    int offset)
{
	unsigned int value;
	value = readl(up->port.membase + offset);
	// dprintk("%s()  io 0x%08X  ->   0x%08X\n", __func__, (uint32_t) up->port.membase + offset, value);
	return value;
}

static inline void ar934x_hs_uart_write(struct ar934x_hs_uart_port *up,
				     int offset, unsigned int value)
{
	// dprintk("%s() io 0x%08X value 0x%08X\n", __func__, (uint32_t) up->port.membase + offset, value);
	writel(value, up->port.membase + offset);
}

static inline void ar934x_hs_uart_rmw(struct ar934x_hs_uart_port *up,
				  unsigned int offset,
				  unsigned int mask,
				  unsigned int val)
{
	unsigned int t;

	t = ar934x_hs_uart_read(up, offset);
	t &= ~mask;
	t |= val;
	ar934x_hs_uart_write(up, offset, t);
}

static inline void ar934x_hs_uart_rmw_set(struct ar934x_hs_uart_port *up,
				       unsigned int offset,
				       unsigned int val)
{
	ar934x_hs_uart_rmw(up, offset, 0, val);
}

static inline void ar934x_hs_uart_rmw_clear(struct ar934x_hs_uart_port *up,
					 unsigned int offset,
					 unsigned int val)
{
	ar934x_hs_uart_rmw(up, offset, val, 0);
}

static inline void ar934x_hs_uart_start_tx_interrupt(struct ar934x_hs_uart_port *up)
{
	up->ier |= AR934X_HS_UART_INT_TX_EMPTY;
	ar934x_hs_uart_write(up, AR934X_HS_UART_INT_EN_REG, up->ier);
}

static inline void ar934x_hs_uart_stop_tx_interrupt(struct ar934x_hs_uart_port *up)
{
	up->ier &= ~AR934X_HS_UART_INT_TX_EMPTY;
	ar934x_hs_uart_write(up, AR934X_HS_UART_INT_EN_REG, up->ier);
}

static inline void ar934x_hs_uart_putc(struct ar934x_hs_uart_port *up, int ch)
{
	unsigned int rdata;

	rdata = ch & AR934X_HS_UART_DATA_TX_RX_MASK;
	rdata |= AR934X_HS_UART_DATA_TX_CSR;
	// dprintk("%s() write 0x%08X value 0x%08X\n", __func__, rdata, AR934X_HS_UART_DATA_REG);
	ar934x_hs_uart_write(up, AR934X_HS_UART_DATA_REG, rdata);
}

static unsigned int ar934x_hs_uart_tx_empty(struct uart_port *port)
{
	struct ar934x_hs_uart_port *up = (struct ar934x_hs_uart_port *) port;
	unsigned long flags;
	unsigned int rdata;

	spin_lock_irqsave(&up->port.lock, flags);
	rdata = ar934x_hs_uart_read(up, AR934X_HS_UART_DATA_REG);
	spin_unlock_irqrestore(&up->port.lock, flags);

	return (rdata & AR934X_HS_UART_DATA_TX_CSR) ? 0 : TIOCSER_TEMT;
}

static unsigned int ar934x_hs_uart_get_mctrl(struct uart_port *port)
{
	return TIOCM_CAR;
}

static void ar934x_hs_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
}

static void ar934x_hs_uart_start_tx(struct uart_port *port)
{
	struct ar934x_hs_uart_port *up = (struct ar934x_hs_uart_port *) port;

	ar934x_hs_uart_start_tx_interrupt(up);
}

static void ar934x_hs_uart_stop_tx(struct uart_port *port)
{
	struct ar934x_hs_uart_port *up = (struct ar934x_hs_uart_port *) port;

	ar934x_hs_uart_stop_tx_interrupt(up);
}

static void ar934x_hs_uart_stop_rx(struct uart_port *port)
{
	struct ar934x_hs_uart_port *up = (struct ar934x_hs_uart_port *) port;

	up->ier &= ~AR934X_HS_UART_INT_RX_VALID;
	ar934x_hs_uart_write(up, AR934X_HS_UART_INT_EN_REG, up->ier);
}

static void ar934x_hs_uart_break_ctl(struct uart_port *port, int break_state)
{
	struct ar934x_hs_uart_port *up = (struct ar934x_hs_uart_port *) port;
	unsigned long flags;

	spin_lock_irqsave(&up->port.lock, flags);
	if (break_state == -1)
		ar934x_hs_uart_rmw_set(up, AR934X_HS_UART_CS_REG,
				    AR934X_HS_UART_CS_TX_BREAK);
	else
		ar934x_hs_uart_rmw_clear(up, AR934X_HS_UART_CS_REG,
				      AR934X_HS_UART_CS_TX_BREAK);
	spin_unlock_irqrestore(&up->port.lock, flags);
}

static void ar934x_hs_uart_enable_ms(struct uart_port *port)
{
}

/*
 * baudrate = (clk / (scale + 1)) * (step * (1 / 2^17))
 */
static unsigned long ar934x_hs_uart_get_baud(unsigned int clk,
					  unsigned int scale,
					  unsigned int step)
{
	u64 t;
	u32 div;

	div = (2 << 16) * (scale + 1);
	t = clk;
	t *= step;
	t += (div / 2);
	do_div(t, div);

	return t;
}

static void ar934x_hs_uart_get_scale_step(unsigned int clk,
				       unsigned int baud,
				       unsigned int *scale,
				       unsigned int *step)
{
	unsigned int tscale;
	long min_diff;

	*scale = 0;
	*step = 0;

	min_diff = baud;
	for (tscale = 0; tscale < AR934X_HS_UART_MAX_SCALE; tscale++) {
		u64 tstep;
		int diff;

		tstep = baud * (tscale + 1);
		tstep *= (2 << 16);
		do_div(tstep, clk);

		if (tstep > AR934X_HS_UART_MAX_STEP)
			break;

		diff = abs(ar934x_hs_uart_get_baud(clk, tscale, tstep) - baud);
		if (diff < min_diff) {
			min_diff = diff;
			*scale = tscale;
			*step = tstep;
		
		}
	}
	dprintk("%s() clk:%d scale:0x%04X step:0x%04X baud:%d min_diff:%ld\n", __func__, clk, *scale, *step, baud, min_diff);
}

static void ar934x_hs_uart_set_termios(struct uart_port *port,
				    struct ktermios *new,
				    struct ktermios *old)
{
	struct ar934x_hs_uart_port *up = (struct ar934x_hs_uart_port *) port;
	unsigned int cs;
	unsigned long flags;
	unsigned int baud, scale, step;

	/* Only CS8 is supported */
	new->c_cflag &= ~CSIZE;
	new->c_cflag |= CS8;

	/* Only one stop bit is supported */
	new->c_cflag &= ~CSTOPB;

	cs = 0;
	if (new->c_cflag & PARENB) {
		if (!(new->c_cflag & PARODD))
			cs |= AR934X_HS_UART_CS_PARITY_EVEN;
		else
			cs |= AR934X_HS_UART_CS_PARITY_ODD;
	} else {
		cs |= AR934X_HS_UART_CS_PARITY_NONE;
	}

	/* Mark/space parity is not supported */
	new->c_cflag &= ~CMSPAR;

	baud = uart_get_baud_rate(port, new, old, up->min_baud, up->max_baud);
	ar934x_hs_uart_get_scale_step(port->uartclk, baud, &scale, &step);

	/*
	 * Ok, we're now changing the port state. Do it with
	 * interrupts disabled.
	 */
	spin_lock_irqsave(&up->port.lock, flags);

	/* disable the UART */
	ar934x_hs_uart_rmw_clear(up, AR934X_HS_UART_CS_REG,
		      AR934X_HS_UART_CS_IF_MODE_M << AR934X_HS_UART_CS_IF_MODE_S);

	/* Update the per-port timeout. */
	uart_update_timeout(port, new->c_cflag, baud);

	up->port.ignore_status_mask = 0;

	/* ignore all characters if CREAD is not set */
	if ((new->c_cflag & CREAD) == 0)
		up->port.ignore_status_mask |= AR934X_HS_DUMMY_STATUS_RD;

	ar934x_hs_uart_write(up, AR934X_HS_UART_CLOCK_REG,
			  scale << AR934X_HS_UART_CLOCK_SCALE_S | step);

	/* setup configuration register */
	ar934x_hs_uart_rmw(up, AR934X_HS_UART_CS_REG, AR934X_HS_UART_CS_PARITY_M, cs);

	/* enable host interrupt */
	ar934x_hs_uart_rmw_set(up, AR934X_HS_UART_CS_REG,
			    AR934X_HS_UART_CS_HOST_INT_EN);

	/* enable TX ready overide */
	ar934x_hs_uart_rmw_set(up, AR934X_HS_UART_CS_REG,
			    AR934X_HS_UART_CS_TX_READY_ORIDE);

	/* enable RX ready overide */
	ar934x_hs_uart_rmw_set(up, AR934X_HS_UART_CS_REG,
			    AR934X_HS_UART_CS_RX_READY_ORIDE);

	/* reenable the UART */
	ar934x_hs_uart_rmw(up, AR934X_HS_UART_CS_REG,
			AR934X_HS_UART_CS_IF_MODE_M << AR934X_HS_UART_CS_IF_MODE_S,
			AR934X_HS_UART_CS_IF_MODE_DCE << AR934X_HS_UART_CS_IF_MODE_S);

	spin_unlock_irqrestore(&up->port.lock, flags);

	if (tty_termios_baud_rate(new))
		tty_termios_encode_baud_rate(new, baud, baud);
}

static void ar934x_hs_uart_rx_chars(struct ar934x_hs_uart_port *up)
{
	struct tty_port *port = &up->port.state->port;
	int max_count = 256;

	do {
		unsigned int rdata;
		unsigned char ch;

		rdata = ar934x_hs_uart_read(up, AR934X_HS_UART_DATA_REG);
		if ((rdata & AR934X_HS_UART_DATA_RX_CSR) == 0)
			break;

		/* remove the character from the FIFO */
		ar934x_hs_uart_write(up, AR934X_HS_UART_DATA_REG,
				  AR934X_HS_UART_DATA_RX_CSR);

		up->port.icount.rx++;
		ch = rdata & AR934X_HS_UART_DATA_TX_RX_MASK;

		if (uart_handle_sysrq_char(&up->port, ch))
			continue;
		if ((up->port.ignore_status_mask & AR934X_HS_DUMMY_STATUS_RD) == 0)
			// dprintk("%s() send to tty %02X\n", __func__, ch);
			tty_insert_flip_char(port, ch, TTY_NORMAL);
	} while (max_count-- > 0);

	spin_unlock(&up->port.lock);
	tty_flip_buffer_push(port);
	spin_lock(&up->port.lock);
}

static void ar934x_hs_uart_tx_chars(struct ar934x_hs_uart_port *up)
{
	struct circ_buf *xmit = &up->port.state->xmit;
	int count;

	// dprintk("%s() check tx_stopped\n", __func__);
	if (uart_tx_stopped(&up->port))
		return;
	// dprintk("%s() tx running\n", __func__);

	count = up->port.fifosize;
	do {
		unsigned int rdata;

		rdata = ar934x_hs_uart_read(up, AR934X_HS_UART_DATA_REG);
		if ((rdata & AR934X_HS_UART_DATA_TX_CSR) == 0)
			break;

		if (up->port.x_char) {
			ar934x_hs_uart_putc(up, up->port.x_char);
			up->port.icount.tx++;
			up->port.x_char = 0;
			continue;
		}

		if (uart_circ_empty(xmit))
			break;

		ar934x_hs_uart_putc(up, xmit->buf[xmit->tail]);

		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		up->port.icount.tx++;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&up->port);

	if (!uart_circ_empty(xmit))
		ar934x_hs_uart_start_tx_interrupt(up);
}

static irqreturn_t ar934x_hs_uart_interrupt(int irq, void *dev_id)
{
	struct ar934x_hs_uart_port *up = dev_id;
	unsigned int status;

	// dprintk("  HS Uart int\n");

	status = ar934x_hs_uart_read(up, AR934X_HS_UART_CS_REG);
	if ((status & AR934X_HS_UART_CS_HOST_INT) == 0)
		return IRQ_NONE;

	spin_lock(&up->port.lock);

	status = ar934x_hs_uart_read(up, AR934X_HS_UART_INT_REG);
	status &= ar934x_hs_uart_read(up, AR934X_HS_UART_INT_EN_REG);

	if (status & AR934X_HS_UART_INT_RX_VALID) {
		ar934x_hs_uart_write(up, AR934X_HS_UART_INT_REG,
				  AR934X_HS_UART_INT_RX_VALID);
		ar934x_hs_uart_rx_chars(up);
	}

	if (status & AR934X_HS_UART_INT_TX_EMPTY) {
		ar934x_hs_uart_write(up, AR934X_HS_UART_INT_REG,
				  AR934X_HS_UART_INT_TX_EMPTY);
		ar934x_hs_uart_stop_tx_interrupt(up);
		ar934x_hs_uart_tx_chars(up);
	}

	spin_unlock(&up->port.lock);

	return IRQ_HANDLED;
}

static int ar934x_hs_uart_startup(struct uart_port *port)
{
	struct ar934x_hs_uart_port *up = (struct ar934x_hs_uart_port *) port;
	unsigned long flags;
	int ret;

	ret = request_irq(up->port.irq, ar934x_hs_uart_interrupt,
			  up->port.irqflags, dev_name(up->port.dev), up);
	if (ret)
		return ret;

	spin_lock_irqsave(&up->port.lock, flags);

	/* Enable HOST interrupts */
	ar934x_hs_uart_rmw_set(up, AR934X_HS_UART_CS_REG,
			    AR934X_HS_UART_CS_HOST_INT_EN);

	/* enable TX ready overide */
	ar934x_hs_uart_rmw_set(up, AR934X_HS_UART_CS_REG,
			    AR934X_HS_UART_CS_TX_READY_ORIDE);

	/* Enable RX interrupts */
	up->ier = AR934X_HS_UART_INT_RX_VALID;
	ar934x_hs_uart_write(up, AR934X_HS_UART_INT_EN_REG, up->ier);

	spin_unlock_irqrestore(&up->port.lock, flags);

	return 0;
}

static void ar934x_hs_uart_shutdown(struct uart_port *port)
{
	struct ar934x_hs_uart_port *up = (struct ar934x_hs_uart_port *) port;

	/* Disable all interrupts */
	up->ier = 0;
	ar934x_hs_uart_write(up, AR934X_HS_UART_INT_EN_REG, up->ier);

	/* Disable break condition */
	ar934x_hs_uart_rmw_clear(up, AR934X_HS_UART_CS_REG,
			      AR934X_HS_UART_CS_TX_BREAK);

	free_irq(up->port.irq, up);
}

static const char *ar934x_hs_uart_type(struct uart_port *port)
{
	return (port->type == PORT_AR933X) ? "AR934X UART" : NULL;
}

static void ar934x_hs_uart_release_port(struct uart_port *port)
{
	// dprintk("%s() : do\n", __func__);
	/* Nothing to release ... */
}

static int ar934x_hs_uart_request_port(struct uart_port *port)
{
	// dprintk("%s() : do\n", __func__);
	/* UARTs always present */
	return 0;
}

static void ar934x_hs_uart_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE)
		port->type = PORT_AR933X;
}

static int ar934x_hs_uart_verify_port(struct uart_port *port,
				   struct serial_struct *ser)
{
	struct ar934x_hs_uart_port *up = (struct ar934x_hs_uart_port *) port;

	if (ser->type != PORT_UNKNOWN &&
	    ser->type != PORT_AR933X)
		return -EINVAL;

	if (ser->irq < 0 || ser->irq >= NR_IRQS)
		return -EINVAL;

	if (ser->baud_base < up->min_baud ||
	    ser->baud_base > up->max_baud)
		return -EINVAL;

	return 0;
}

static struct uart_ops ar934x_hs_uart_ops = {
	.tx_empty	= ar934x_hs_uart_tx_empty,
	.set_mctrl	= ar934x_hs_uart_set_mctrl,
	.get_mctrl	= ar934x_hs_uart_get_mctrl,
	.stop_tx	= ar934x_hs_uart_stop_tx,
	.start_tx	= ar934x_hs_uart_start_tx,
	.stop_rx	= ar934x_hs_uart_stop_rx,
	.enable_ms	= ar934x_hs_uart_enable_ms,
	.break_ctl	= ar934x_hs_uart_break_ctl,
	.startup	= ar934x_hs_uart_startup,
	.shutdown	= ar934x_hs_uart_shutdown,
	.set_termios	= ar934x_hs_uart_set_termios,
	.type		= ar934x_hs_uart_type,
	.release_port	= ar934x_hs_uart_release_port,
	.request_port	= ar934x_hs_uart_request_port,
	.config_port	= ar934x_hs_uart_config_port,
	.verify_port	= ar934x_hs_uart_verify_port,
};

static struct ar934x_hs_uart_port *
ar934x_console_ports[CONFIG_SERIAL_AR933X_NR_UARTS];

static void ar934x_hs_uart_wait_xmitr(struct ar934x_hs_uart_port *up)
{
	unsigned int status;
	unsigned int timeout = 60000;

	/* Wait up to 60ms for the character(s) to be sent. */
	do {
		status = ar934x_hs_uart_read(up, AR934X_HS_UART_DATA_REG);
		if (--timeout == 0)
			break;
		udelay(1);
	} while ((status & AR934X_HS_UART_DATA_TX_CSR) == 0);
}

static void ar934x_hs_uart_console_putchar(struct uart_port *port, int ch)
{
	struct ar934x_hs_uart_port *up = (struct ar934x_hs_uart_port *) port;

	ar934x_hs_uart_wait_xmitr(up);
	ar934x_hs_uart_putc(up, ch);
}

static void ar934x_hs_uart_console_write(struct console *co, const char *s,
				      unsigned int count)
{
	struct ar934x_hs_uart_port *up = ar934x_console_ports[co->index];
	unsigned long flags;
	unsigned int int_en;
	int locked = 1;

	local_irq_save(flags);

	if (up->port.sysrq)
		locked = 0;
	else if (oops_in_progress)
		locked = spin_trylock(&up->port.lock);
	else
		spin_lock(&up->port.lock);

	/*
	 * First save the IER then disable the interrupts
	 */
	int_en = ar934x_hs_uart_read(up, AR934X_HS_UART_INT_EN_REG);
	ar934x_hs_uart_write(up, AR934X_HS_UART_INT_EN_REG, 0);

	uart_console_write(&up->port, s, count, ar934x_hs_uart_console_putchar);

	/*
	 * Finally, wait for transmitter to become empty
	 * and restore the IER
	 */
	ar934x_hs_uart_wait_xmitr(up);
	ar934x_hs_uart_write(up, AR934X_HS_UART_INT_EN_REG, int_en);

	ar934x_hs_uart_write(up, AR934X_HS_UART_INT_REG, AR934X_HS_UART_INT_ALLINTS);

	if (locked)
		spin_unlock(&up->port.lock);

	local_irq_restore(flags);
}

static int ar934x_hs_uart_console_setup(struct console *co, char *options)
{
	struct ar934x_hs_uart_port *up;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (co->index < 0 || co->index >= CONFIG_SERIAL_AR933X_NR_UARTS)
		return -EINVAL;

	up = ar934x_console_ports[co->index];
	if (!up)
		return -ENODEV;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	return uart_set_options(&up->port, co, baud, parity, bits, flow);
}

static struct console ar934x_hs_uart_console = {
	.name		= "ttyATH",
	.write		= ar934x_hs_uart_console_write,
	.device		= uart_console_device,
	.setup		= ar934x_hs_uart_console_setup,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.data		= &ar934x_hs_uart_driver,
};

static void ar934x_hs_uart_add_console_port(struct ar934x_hs_uart_port *up)
{
	if (!ar934x_hs_uart_console_enabled())
		return;
	
	dprintk("%s() is console\n", __func__ );
	ar934x_console_ports[up->port.line] = up;
}

static struct uart_driver ar934x_hs_uart_driver = {
	.owner		= THIS_MODULE,
	.driver_name	= DRIVER_NAME,
	.dev_name	= "ttyATH",
	.nr		= CONFIG_SERIAL_AR933X_NR_UARTS,
	.cons		= NULL, /* filled in runtime */
};

static int ar934x_hs_uart_probe(struct platform_device *pdev)
{
	struct ar934x_hs_uart_port *up;
	struct uart_port *port;
	struct resource *mem_res;
	struct resource *irq_res;
	struct device_node *np;
	unsigned int baud;
	int id;
	int ret;

	np = pdev->dev.of_node;
	if (config_enabled(CONFIG_OF) && np) {
		id = of_alias_get_id(np, "serial");
		if (id < 0) {
			dev_err(&pdev->dev, "unable to get alias id, err=%d\n",
				id);
			return id;
		}
	} else {
		id = pdev->id;
		if (id == -1)
			id = 0;
	}

	if (id > CONFIG_SERIAL_AR933X_NR_UARTS)
		return -EINVAL;

	irq_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!irq_res) {
		dev_err(&pdev->dev, "no IRQ resource\n");
		return -EINVAL;
	}

	up = devm_kzalloc(&pdev->dev, sizeof(struct ar934x_hs_uart_port),
			  GFP_KERNEL);
	if (!up)
		return -ENOMEM;

	up->clk = devm_clk_get(&pdev->dev, "uart");
	if (IS_ERR(up->clk)) {
		dev_err(&pdev->dev, "unable to get UART clock\n");
		return PTR_ERR(up->clk);
	}

	port = &up->port;

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	port->membase = devm_ioremap_resource(&pdev->dev, mem_res);
	if (IS_ERR(port->membase))
		return PTR_ERR(port->membase);

	ret = clk_prepare_enable(up->clk);
	if (ret)
		return ret;

	port->uartclk = clk_get_rate(up->clk);
	if (!port->uartclk) {
		ret = -EINVAL;
		goto err_disable_clk;
	}

	port->mapbase = mem_res->start;
	port->line = id;
	port->irq = irq_res->start;
	port->dev = &pdev->dev;
	port->type = PORT_AR933X;
	port->iotype = UPIO_MEM32;

	port->regshift = 2;
	port->fifosize = AR934X_HS_UART_FIFO_SIZE;
	port->ops = &ar934x_hs_uart_ops;

	baud = ar934x_hs_uart_get_baud(port->uartclk, AR934X_HS_UART_MAX_SCALE, 1);
	up->min_baud = max_t(unsigned int, baud, AR934X_HS_UART_MIN_BAUD);

	baud = ar934x_hs_uart_get_baud(port->uartclk, 0, AR934X_HS_UART_MAX_STEP);
	up->max_baud = min_t(unsigned int, baud, AR934X_HS_UART_MAX_BAUD);

	ar934x_hs_uart_add_console_port(up);

	ret = uart_add_one_port(&ar934x_hs_uart_driver, &up->port);
	if (ret)
		goto err_disable_clk;

	platform_set_drvdata(pdev, up);
	return 0;

err_disable_clk:
	clk_disable_unprepare(up->clk);
	return ret;
}

static int ar934x_hs_uart_remove(struct platform_device *pdev)
{
	struct ar934x_hs_uart_port *up;

	up = platform_get_drvdata(pdev);

	if (up) {
		uart_remove_one_port(&ar934x_hs_uart_driver, &up->port);
		clk_disable_unprepare(up->clk);
	}

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ar934x_hs_uart_of_ids[] = {
	{ .compatible = "qca,ar9340-hs-uart" },
	{},
};
MODULE_DEVICE_TABLE(of, ar934x_hs_uart_of_ids);
#endif

static struct platform_driver ar934x_hs_uart_platform_driver = {
	.probe		= ar934x_hs_uart_probe,
	.remove		= ar934x_hs_uart_remove,
	.driver		= {
		.name		= DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = of_match_ptr(ar934x_hs_uart_of_ids),
	},
};

static struct resource ar934x_hs_uart_resources[] = {
        {
                .start  = AR934X_UART1_BASE,
                .end    = AR934X_UART1_BASE + AR934X_UART1_SIZE - 1,
                .flags  = IORESOURCE_MEM,
        },
        {
                .start  = ATH79_MISC_IRQ(6),
                .end    = ATH79_MISC_IRQ(6),
                .flags  = IORESOURCE_IRQ,
        },
};

static struct ar934x_uart_platform_data ar934x_uart_data;
static struct platform_device ar934x_hs_uart_device = {
        .name           = DRIVER_NAME,
        .id             = 0,
        .resource       = ar934x_hs_uart_resources,
        .num_resources  = ARRAY_SIZE(ar934x_hs_uart_resources),
	.dev = {
		.platform_data  = &ar934x_uart_data,
	},
};

static int __init ar934x_hs_uart_init(void)
{
	int ret;
	/* unsigned long uart_clk_rate; */
	// dprintk("%s() : init ...\n", __func__);

	if (ar934x_hs_uart_console_enabled()) {
		dprintk("%s() : is console\n", __func__);
		ar934x_hs_uart_driver.cons = &ar934x_hs_uart_console;
	};

	ret = uart_register_driver(&ar934x_hs_uart_driver);
	if (ret)
		goto err_out;

	ret = platform_driver_register(&ar934x_hs_uart_platform_driver);
	if (ret)
		goto err_unregister_uart_driver;

	platform_device_register(&ar934x_hs_uart_device);

	return 0;

err_unregister_uart_driver:
	uart_unregister_driver(&ar934x_hs_uart_driver);
err_out:
	dprintk("%s() : exit with error\n", __func__);
	return ret;
}

static void __exit ar934x_hs_uart_exit(void)
{
	platform_device_unregister(&ar934x_hs_uart_device);
	platform_driver_unregister(&ar934x_hs_uart_platform_driver);
	uart_unregister_driver(&ar934x_hs_uart_driver);
}

module_init(ar934x_hs_uart_init);
module_exit(ar934x_hs_uart_exit);

MODULE_DESCRIPTION("Atheros AR934X HS UART driver");
MODULE_AUTHOR("Gabor Juhos <juhosg@openwrt.org, errors designed by Gerhard Bertelsmann <info@gerhard-bertelsmann.de>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRIVER_NAME);
