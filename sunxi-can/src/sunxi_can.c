/*
 * sunxi_can.c - CAN bus controller driver for Allwinner SUN4I&UN7I based SoCs
 *
 * Copyright (c) 2015 Gerhard Bertelsmann
 *
 * Parts of this software are based on (derived from) the SJA1000 code by:
 *   Copyright (C) 2007 Wolfgang Grandegger <wg@grandegger.com>
 *   Copyright (c) 2002-2007 Volkswagen Group Electronic Research
 *   Copyright (c) 2003 Matthias Brukner, Trajet Gmbh, Rebenring 33,
 *   38106 Braunschweig, GERMANY
 *   Copyright (c) 2014 Oliver Hartkopp <oliver.hartkopp@volkswagen.de>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the version 2 of the GNU General Public License
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/netdevice.h>
#include <linux/can.h>
#include <linux/can/dev.h>
#include <linux/can/error.h>
/* #include <linux/can/led.h> */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#define DRV_NAME "sunxi_can"

/* Registers address */
#define CAN_BASE0		0x01C2BC00
#define CAN_MSEL_ADDR		0x0000  /* CAN Mode Select Register */
#define CAN_CMD_ADDR            0x0004  /* CAN Command Register */
#define CAN_STA_ADDR		0x0008  /* CAN Status Register */
#define CAN_INT_ADDR		0x000c  /* CAN Interrupt Flag Register */
#define CAN_INTEN_ADDR		0x0010  /* CAN Interrupt Enable Register */
#define CAN_BTIME_ADDR		0x0014  /* CAN Bus Timing 0 Register */
#define CAN_TEWL_ADDR		0x0018  /* CAN Tx Error Warning Limit Register */
#define CAN_ERRC_ADDR		0x001c  /* CAN Error Counter Register */
#define CAN_RMCNT_ADDR		0x0020  /* CAN Receive Message Counter Register */
#define CAN_RBUFSA_ADDR		0x0024  /* CAN Receive Buffer Start Address Register */
#define CAN_BUF0_ADDR		0x0040  /* CAN Tx/Rx Buffer 0 Register */
#define CAN_BUF1_ADDR		0x0044  /* CAN Tx/Rx Buffer 1 Register */
#define CAN_BUF2_ADDR		0x0048  /* CAN Tx/Rx Buffer 2 Register */
#define CAN_BUF3_ADDR		0x004c  /* CAN Tx/Rx Buffer 3 Register */
#define CAN_BUF4_ADDR		0x0050  /* CAN Tx/Rx Buffer 4 Register */
#define CAN_BUF5_ADDR		0x0054  /* CAN Tx/Rx Buffer 5 Register */
#define CAN_BUF6_ADDR		0x0058  /* CAN Tx/Rx Buffer 6 Register */
#define CAN_BUF7_ADDR		0x005c  /* CAN Tx/Rx Buffer 7 Register */
#define CAN_BUF8_ADDR		0x0060  /* CAN Tx/Rx Buffer 8 Register */
#define CAN_BUF9_ADDR		0x0064  /* CAN Tx/Rx Buffer 9 Register */
#define CAN_BUF10_ADDR		0x0068  /* CAN Tx/Rx Buffer 10 Register */
#define CAN_BUF11_ADDR		0x006c  /* CAN Tx/Rx Buffer 11 Register */
#define CAN_BUF12_ADDR		0x0070  /* CAN Tx/Rx Buffer 12 Register */
#define CAN_ACPC_ADDR		0x0040  /* CAN Acceptance Code 0 Register */
#define CAN_ACPM_ADDR		0x0044  /* CAN Acceptance Mask 0 Register */
#define CAN_RBUF_RBACK_START_ADDR	+ 0x0180	/* CAN transmit buffer for read back register */
#define CAN_RBUF_RBACK_END_ADDR		+ 0x01b0	/* CAN transmit buffer for read back register */

/* Controller Register Description */

/* mode select register (r/w)
 * offset:0x0000 default:0x0000_0001
 */
#define SLEEP_MODE		(1<<4)	/* This bit can only be written in Reset Mode */
#define WAKE_UP			(0<<4)
#define SINGLE_FILTER		(1<<3)	/* This bit can only be written in Reset Mode */
#define DUAL_FILTERS		(0<<3)
#define LOOPBACK_MODE		BIT(2)
#define LISTEN_ONLY_MODE	BIT(1)
#define RESET_MODE		BIT(0)

/* command register (w)
 * offset:0x0004 default:0x0000_0000
 */
#define BUS_OFF_REQ		BIT(5)
#define SELF_RCV_REQ		BIT(4)
#define CLEAR_DOVERRUN		BIT(3)
#define RELEASE_RBUF		BIT(2)
#define ABORT_REQ		BIT(1)
#define TRANS_REQ		BIT(0)


/* status register (r)
 * offset:0x0008 default:0x0000_003c
 */
#define BIT_ERR			(0<<22)
#define FORM_ERR		(1<<22)
#define STUFF_ERR		(2<<22)
#define OTHER_ERR		(3<<22)
#define ERR_DIR			(1<<21)
#define ERR_SEG_CODE		(0x1f<<16)
#define START			(0x03<<16)
#define ID28_21			(0x02<<16)
#define ID20_18			(0x06<<16)
#define SRTR			(0x04<<16)
#define IDE			(0x05<<16)
#define ID17_13			(0x07<<16)
#define ID12_5			(0x0f<<16)
#define ID4_0			(0x0e<<16)
#define RTR			(0x0c<<16)
#define RB1			(0x0d<<16)
#define RB0			(0x09<<16)
#define DLEN			(0x0b<<16)
#define DATA_FIELD		(0x0a<<16)
#define CRC_SEQUENCE		(0x08<<16)
#define CRC_DELIMITER		(0x18<<16)
#define ACK			(0x19<<16)
#define ACK_DELIMITER		(0x1b<<16)
#define END			(0x1a<<16)
#define INTERMISSION		(0x12<<16)
#define ACTIVE_ERROR		(0x11<<16)
#define PASSIVE_ERROR		(0x16<<16)
#define TOLERATE_DOMINANT_BITS	(0x13<<16)
#define ERROR_DELIMITER		(0x17<<16)
#define OVERLOAD		(0x1c<<16)
#define BUS_OFF			BIT(7)
#define ERR_STA			BIT(6)
#define TRANS_BUSY		BIT(5)
#define RCV_BUSY		BIT(4)
#define TRANS_OVER		BIT(3)
#define TBUF_RDY		BIT(2)
#define DATA_ORUN		BIT(1)
#define RBUF_RDY		BIT(0)

/* interrupt register (r)
 * offset:0x000c default:0x0000_0000
 */
#define BUS_ERR			BIT(7)
#define ARB_LOST		BIT(6)
#define ERR_PASSIVE		BIT(5)
#define WAKEUP			BIT(4)
#define DATA_ORUNI		BIT(3)
#define ERR_WRN			BIT(2)
#define TBUF_VLD		BIT(1)
#define RBUF_VLD		BIT(0)

/* interrupt enable register (r/w)
 * offset:0x0010 default:0x0000_0000
 */
#define BERR_IRQ_EN		(1<<7)
#define BERR_IRQ_DIS		(0<<7)
#define ARB_LOST_IRQ_EN		(1<<6)
#define ARB_LOST_IRQ_DIS	(0<<6)
#define ERR_PASSIVE_IRQ_EN	(1<<5)
#define ERR_PASSIVE_IRQ_DIS	(0<<5)
#define WAKEUP_IRQ_EN		(1<<4)
#define WAKEUP_IRQ_DIS		(0<<4)
#define OR_IRQ_EN		(1<<3)
#define OR_IRQ_DIS		(0<<3)
#define ERR_WRN_IRQ_EN		(1<<2)
#define ERR_WRN_IRQ_DIS		(0<<2)
#define TX_IRQ_EN		(1<<1)
#define TX_IRQ_DIS		(0<<1)
#define RX_IRQ_EN		(1<<0)
#define RX_IRQ_DIS		(0<<0)

/* output control */
#define NOR_OMODE		(2)
#define CLK_OMODE		(3)

/* error code */
#define ERR_INRCV		(1<<5)
#define ERR_INTRANS		(0<<5)

/* filter mode */
#define FILTER_CLOSE		0
#define SINGLE_FLTER_MODE	1
#define DUAL_FILTER_MODE	2

#define SUNXI_CAN_CUSTOM_IRQ_HANDLER 0x1

#define SW_INT_IRQNO_CAN	26
#define CLK_MOD_CANi		"can"
#define SUNXI_CAN_MAX_IRQ	20	/* max. number of interrupts handled in ISR */


struct sunxi-can_devtype_data {
        u32 features;   /* hardware controller features */
};

struct sunxi_can_priv {
        struct can_priv can;
        struct net_device *dev;
	struct napi_struct napi;

        int open_time;
        struct sk_buff *echo_skb;
        void __iomem *reg_base;
	struct clk *clk;

        void *priv;                     /* for board-specific data */

        unsigned long irq_flags;        /* for request_irq() */
        spinlock_t cmdreg_lock;         /* lock for concurrent cmd register writes */

        u16 flags;                      /* custom mode flags */
};

static struct sunxi-can_devtype_data sun7i_devtype_data;

static struct can_bittiming_const sunxi_can_bittiming_const = {
	.name = DRV_NAME,
	.tseg1_min = 1,
	.tseg1_max = 16,
	.tseg2_min = 1,
	.tseg2_max = 8,
	.sjw_max = 4,
	.brp_min = 1,
	.brp_max = 64,
	.brp_inc = 1,
};

static void sunxi_can_write_cmdreg(struct sunxi_can_priv *priv, u8 val)
{
	unsigned long flags;

	/*
	 * The command register needs some locking and time to settle
	 * the write_reg() operation - especially on SMP systems.
	 */
	spin_lock_irqsave(&priv->cmdreg_lock, flags);
	writel(val, priv->reg_base + CAN_CMD_ADDR);
	readl(priv->reg_base + CAN_STA_ADDR);
	spin_unlock_irqrestore(&priv->cmdreg_lock, flags);
}

static int sunxi_can_is_absent(struct sunxi_can_priv *priv)
{
	return ((readl(priv->reg_base + CAN_MSEL_ADDR) & 0xFF) == 0xFF);
}

static int sunxi_can_probe(struct net_device *dev)
{
	int err, irq = 0;
	/* struct net_device *dev; */

	struct sunxi_can_priv *priv = netdev_priv(dev);


	if (sunxi_can_is_absent(priv)) {
		printk(KERN_INFO "%s: probing @0x%lX failed\n",
		       DRV_NAME, dev->base_addr);
		return 0;
	}
	return -1;
}

static void set_reset_mode(struct net_device *dev)
{
	struct sunxi_can_priv *priv = netdev_priv(dev);
	uint32_t status = readl(priv->reg_base + CAN_MSEL_ADDR);
	int i;

	for (i = 0; i < 100; i++) {
		/* check reset bit */
		if (status & RESET_MODE) {
			priv->can.state = CAN_STATE_STOPPED;
			return;
		}

		writel(readl(priv->reg_base + CAN_MSEL_ADDR) | RESET_MODE, priv->reg_base + CAN_MSEL_ADDR);	/* select reset mode */
		/* writel(RESET_MODE, CAN_MSEL_ADDR); */ /* select reset mode */
		udelay(10);
		status = readl(priv->reg_base + CAN_MSEL_ADDR);
	}

	netdev_err(dev, "setting SUNXI_CAN into reset mode failed!\n");
}

static void set_normal_mode(struct net_device *dev)
{
	struct sunxi_can_priv *priv = netdev_priv(dev);
	unsigned char status = readl(priv->reg_base + CAN_MSEL_ADDR);
	int i;

	for (i = 0; i < 100; i++) {
		/* check reset bit */
		if ((status & RESET_MODE) == 0) {
			priv->can.state = CAN_STATE_ERROR_ACTIVE;
			/* enable interrupts */
			if (priv->can.ctrlmode & CAN_CTRLMODE_BERR_REPORTING) {
				writel(0xFFFF, priv->reg_base + CAN_INTEN_ADDR);
			} else {
				writel(0xFFFF & ~BERR_IRQ_EN, priv->reg_base + CAN_INTEN_ADDR);
			}

			if (priv->can.ctrlmode & CAN_CTRLMODE_LOOPBACK) {
				/* Put device into loopback mode */
				writel(readl(priv->reg_base + CAN_MSEL_ADDR) | LOOPBACK_MODE,
				       priv->reg_base + CAN_MSEL_ADDR);
			} else if (priv->can.ctrlmode & CAN_CTRLMODE_LISTENONLY) {
				/* Put device into listen-only mode */
				writel(readl( priv->reg_base + CAN_MSEL_ADDR) | LISTEN_ONLY_MODE,
				       priv->reg_base + CAN_MSEL_ADDR);
			}
			return;
		}

		/* set chip to normal mode */
		writel(readl(priv->reg_base + CAN_MSEL_ADDR) & (~RESET_MODE), priv->reg_base + CAN_MSEL_ADDR);
		udelay(10);
		status = readl(priv->reg_base + CAN_MSEL_ADDR);
	}

	netdev_err(dev, "setting SUNXI_CAN into normal mode failed!\n");
}

static void sunxi_can_start(struct net_device *dev)
{
	struct sunxi_can_priv *priv = netdev_priv(dev);

	/* leave reset mode */
	if (priv->can.state != CAN_STATE_STOPPED)
		set_reset_mode(dev);

	/* Clear error counters and error code capture */
	writel(0x0,  priv->reg_base + CAN_ERRC_ADDR);

	/* leave reset mode */
	set_normal_mode(dev);
}

static int sunxi_can_set_mode(struct net_device *dev, enum can_mode mode)
{
	struct sunxi_can_priv *priv = netdev_priv(dev);

	if (!priv->open_time)
		return -EINVAL;

	switch (mode) {
	case CAN_MODE_START:
		sunxi_can_start(dev);
		if (netif_queue_stopped(dev))
			netif_wake_queue(dev);
		break;

	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static int sunxi_can_set_bittiming(struct net_device *dev)
{
	struct sunxi_can_priv *priv = netdev_priv(dev);
	struct can_bittiming *bt = &priv->can.bittiming;
	u32 cfg;

	cfg = ((bt->brp - 1) & 0x3FF) |
	      (((bt->sjw - 1) & 0x3) << 14) |
	      (((bt->prop_seg + bt->phase_seg1 - 1) & 0xf) << 16) |
	      (((bt->phase_seg2 - 1) & 0x7) << 20);
	if (priv->can.ctrlmode & CAN_CTRLMODE_3_SAMPLES)
		cfg |= 0x800000;

	netdev_info(dev, "setting BITTIMING=0x%08x\n", cfg);

	set_reset_mode(dev);	/* CAN_BTIME_ADDR only writable in reset mode */
	writel(cfg,  priv->reg_base + CAN_BTIME_ADDR);
	set_normal_mode(dev);

	return 0;
}

static int sunxi_can_get_berr_counter(const struct net_device *dev,
				      struct can_berr_counter *bec)
{
	struct sunxi_can_priv *priv = netdev_priv(dev);
	bec->txerr = readl(priv->reg_base + CAN_ERRC_ADDR) & 0x000F;
	bec->rxerr = (readl(priv->reg_base + CAN_ERRC_ADDR) & 0x0F00) >> 16;

	return 0;
}

/*
* initialize sunxi_can:
* - reset chip
* - set output mode
* - set baudrate
* - enable interrupts
* - start operating mode
*/
static void chipset_init(struct net_device *dev)
{
	u32 temp_irqen;

	/* config pins
	 * PH20-TX, PH21-RX :4 */

	/* void __iomem *gpio_base = ioremap(PIO_BASE, 0x400);
	   void __iomem * port_h_config = gpio_base + PH_CFG2_OFFSET;

	   msleep(100);
	   uint32_t tmp = readl(port_h_config);
	   msleep(100);
	   tmp &= 0xFF00FFFF;
	   writel(tmp | (4 << 16) | (4 << 20), port_h_config);
	   msleep(100);
	 */

	/* 
	   if (gpio_request("can_para", "can_tx") == 0 || gpio_request("can_para", "can_rx") == 0 ) {
	   printk(KERN_INFO "can request gpio fail!\n");
	   }
	 */

	/* enable clock */
	/*  writel(readl((void *)0xF1C20000 + 0x6C) | (1 << 4), (void *)0xF1C20000 + 0x6C); */

	/* set can controller in reset mode */
	set_reset_mode(dev);

	/* enable interrupt */
	temp_irqen = BERR_IRQ_EN | ERR_PASSIVE_IRQ_EN | OR_IRQ_EN | RX_IRQ_EN;
	/* TODO */
	/* writel(readl(  priv->reg_base + CAN_INTEN_ADDR) | temp_irqen,  priv->reg_base + CAN_INTEN_ADDR); */

	/* return to transfer mode */
	set_normal_mode(dev);
}

/*
 * transmit a CAN message
 * message layout in the sk_buff should be like this:
 * xx xx xx xx         ff         ll 00 11 22 33 44 55 66 77
 * [ can_id ] [flags] [len] [can data (up to 8 bytes]
 */
static int sunxi_can_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct sunxi_can_priv *priv = netdev_priv(dev);
	struct can_frame *cf = (struct can_frame *)skb->data;
	uint8_t dlc;
	canid_t id;
	uint32_t temp = 0;
	uint8_t i;

	/* wait buff ready */
	while (!(readl(priv->reg_base + CAN_STA_ADDR) & TBUF_RDY)) ;

	set_reset_mode(dev);

	writel(0xffffffff,  priv->reg_base + CAN_ACPM_ADDR);

	/* enter transfer mode */
	set_normal_mode(dev);

	if (can_dropped_invalid_skb(dev, skb))
		return NETDEV_TX_OK;

	netif_stop_queue(dev);

	dlc = cf->can_dlc;
	id = cf->can_id;

	temp = ((id >> 30) << 6) | dlc;
	writel(temp,  priv->reg_base + CAN_BUF0_ADDR);
	if (id & CAN_EFF_FLAG) {	/* extended frame */
		writel(0xFF & (id >> 21), priv->reg_base + CAN_BUF1_ADDR);	/* id28~21 */
		writel(0xFF & (id >> 13), priv->reg_base + CAN_BUF2_ADDR);	/* id20~13 */
		writel(0xFF & (id >> 5),  priv->reg_base + CAN_BUF3_ADDR);	/* id12~5  */
		writel((id & 0x1F) << 3,  priv->reg_base + CAN_BUF4_ADDR);	/* id4~0   */

		for (i = 0; i < dlc; i++) {
			writel(cf->data[i], priv->reg_base + (CAN_BUF5_ADDR + i * 4));
		}
	} else {			/* standard frame */
		writel(0xFF & (id >> 3),  priv->reg_base + CAN_BUF1_ADDR);	/* id28~21 */
		writel((id & 0x7) << 5,  priv->reg_base + CAN_BUF2_ADDR);	/* id20~13 */

		for (i = 0; i < dlc; i++) {
			writel(cf->data[i],  priv->reg_base + CAN_BUF3_ADDR + i * 4);
		}
	}

	can_put_echo_skb(skb, dev, 0);

	while (!(readl(priv->reg_base + CAN_STA_ADDR) & TBUF_RDY)) ;
	sunxi_can_write_cmdreg(priv, TRANS_REQ);

	return NETDEV_TX_OK;
}

static void sunxi_can_rx(struct net_device *dev)
{
	struct sunxi_can_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	uint8_t fi;
	canid_t id;
	int i;

	/* create zero'ed CAN frame buffer */
	skb = alloc_can_skb(dev, &cf);
	if (skb == NULL)
		return;

	fi = readl(priv->reg_base + CAN_BUF0_ADDR);
	cf->can_dlc = get_can_dlc(fi & 0x0F);
	if (fi >> 7) {
		/* extended frame format (EFF) */
		id = (readl(priv->reg_base + CAN_BUF1_ADDR) << 21) |		/* id28~21 */
		     (readl(priv->reg_base + CAN_BUF2_ADDR) << 13) |		/* id20~13 */
		     (readl(priv->reg_base + CAN_BUF3_ADDR) << 5)  | 		/* id12~5  */
		     ((readl(priv->reg_base + CAN_BUF4_ADDR) >> 3) & 0x1f);	/* id4~0   */
		id |= CAN_EFF_FLAG;

		if ((fi >> 6) & 0x1) {	/* remote transmission request */
			id |= CAN_RTR_FLAG;
		} else {
			for (i = 0; i < cf->can_dlc; i++)
				cf->data[i] = readl(priv->reg_base + CAN_BUF5_ADDR + i * 4);
		}
	} else {
		/* standard frame format (SFF) */
		id = (readl(priv->reg_base + CAN_BUF1_ADDR) << 3)		/* id28~21 */
		    | ((readl(priv->reg_base + CAN_BUF2_ADDR) >> 5) & 0x7);	/* id20~18 */

		if ((fi >> 6) & 0x1) {	/* remote transmission request */
			id |= CAN_RTR_FLAG;
		} else {
			for (i = 0; i < cf->can_dlc; i++)
				cf->data[i] = readl(priv->reg_base + CAN_BUF3_ADDR + i * 4);
		}
	}

	cf->can_id = id;

	/* release receive buffer */
	sunxi_can_write_cmdreg(priv, RELEASE_RBUF);

	netif_rx(skb);

	stats->rx_packets++;
	stats->rx_bytes += cf->can_dlc;
}

static int sunxi_can_err(struct net_device *dev, uint8_t isrc, uint8_t status)
{
	struct sunxi_can_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	enum can_state state = priv->can.state;
	uint32_t ecc, alc;

	skb = alloc_can_err_skb(dev, &cf);
	if (skb == NULL)
		return -ENOMEM;

	if (isrc & DATA_ORUNI) {
		/* data overrun interrupt */
		netdev_dbg(dev, "data overrun interrupt\n");
		cf->can_id |= CAN_ERR_CRTL;
		cf->data[1] = CAN_ERR_CRTL_RX_OVERFLOW;
		stats->rx_over_errors++;
		stats->rx_errors++;
		sunxi_can_write_cmdreg(priv, CLEAR_DOVERRUN);	/* clear bit */
	}

	if (isrc & ERR_WRN) {
		/* error warning interrupt */
		netdev_dbg(dev, "error warning interrupt\n");

		if (status & BUS_OFF) {
			state = CAN_STATE_BUS_OFF;
			cf->can_id |= CAN_ERR_BUSOFF;
			can_bus_off(dev);
		} else if (status & ERR_STA) {
			state = CAN_STATE_ERROR_WARNING;
		} else
			state = CAN_STATE_ERROR_ACTIVE;
	}
	if (isrc & BUS_ERR) {
		/* bus error interrupt */
		priv->can.can_stats.bus_error++;
		stats->rx_errors++;

		ecc = readl(priv->reg_base + CAN_STA_ADDR);

		cf->can_id |= CAN_ERR_PROT | CAN_ERR_BUSERROR;

		if (ecc & BIT_ERR)
			cf->data[2] |= CAN_ERR_PROT_BIT;
		else if (ecc & FORM_ERR)
			cf->data[2] |= CAN_ERR_PROT_FORM;
		else if (ecc & STUFF_ERR)
			cf->data[2] |= CAN_ERR_PROT_STUFF;
		else {
			cf->data[2] |= CAN_ERR_PROT_UNSPEC;
			cf->data[3] = (ecc & ERR_SEG_CODE) >> 16;
		}
		/* Error occurred during transmission? */
		if ((ecc & ERR_DIR) == 0)
			cf->data[2] |= CAN_ERR_PROT_TX;
	}
	if (isrc & ERR_PASSIVE) {
		/* error passive interrupt */
		netdev_dbg(dev, "error passive interrupt\n");
		if (status & ERR_STA)
			state = CAN_STATE_ERROR_PASSIVE;
		else
			state = CAN_STATE_ERROR_ACTIVE;
	}
	if (isrc & ARB_LOST) {
		/* arbitration lost interrupt */
		netdev_dbg(dev, "arbitration lost interrupt\n");
		alc = readl(priv->reg_base + CAN_STA_ADDR);
		priv->can.can_stats.arbitration_lost++;
		stats->tx_errors++;
		cf->can_id |= CAN_ERR_LOSTARB;
		cf->data[0] = (alc & 0x1f) >> 8;
	}

	if (state != priv->can.state && (state == CAN_STATE_ERROR_WARNING ||
					 state == CAN_STATE_ERROR_PASSIVE)) {
		uint8_t rxerr = (readl(priv->reg_base + CAN_ERRC_ADDR) >> 16) & 0xFF;
		uint8_t txerr = readl(priv->reg_base + CAN_ERRC_ADDR) & 0xFF;
		cf->can_id |= CAN_ERR_CRTL;
		if (state == CAN_STATE_ERROR_WARNING) {
			priv->can.can_stats.error_warning++;
			cf->data[1] = (txerr > rxerr) ?
			    CAN_ERR_CRTL_TX_WARNING : CAN_ERR_CRTL_RX_WARNING;
		} else {
			priv->can.can_stats.error_passive++;
			cf->data[1] = (txerr > rxerr) ?
			    CAN_ERR_CRTL_TX_PASSIVE : CAN_ERR_CRTL_RX_PASSIVE;
		}
		cf->data[6] = txerr;
		cf->data[7] = rxerr;
	}

	priv->can.state = state;

	netif_rx(skb);

	stats->rx_packets++;
	stats->rx_bytes += cf->can_dlc;

	return 0;
}

irqreturn_t sunxi_can_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct sunxi_can_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	uint8_t isrc, status;
	int n = 0;

	printk(KERN_INFO "sunxican: capture a interrupt\n");

	/* Shared interrupts and IRQ off? */
	if ((readl(priv->reg_base + CAN_INT_ADDR) & 0xF) == 0x0)
		return IRQ_NONE;

	while ((isrc = readl(priv->reg_base + CAN_INT_ADDR)) && (n < SUNXI_CAN_MAX_IRQ)) {
		n++;
		status = readl(priv->reg_base + CAN_STA_ADDR);
		/* check for absent controller due to hw unplug */
		if (sunxi_can_is_absent(priv))
			return IRQ_NONE;

		if (isrc & WAKEUP)
			netdev_warn(dev, "wakeup interrupt\n");

		if (isrc & TBUF_VLD) {
			/* transmission complete interrupt */
			stats->tx_bytes +=
			    readl(priv->reg_base + CAN_RBUF_RBACK_START_ADDR) & 0xf;
			stats->tx_packets++;
			can_get_echo_skb(dev, 0);
			netif_wake_queue(dev);
		}
		if (isrc & RBUF_VLD) {
			/* receive interrupt */
			while (status & RBUF_RDY) {	/* RX buffer is not empty */
				sunxi_can_rx(dev);
				status = readl(priv->reg_base + CAN_STA_ADDR);
				/* check for absent controller */
				if (sunxi_can_is_absent(priv))
					return IRQ_NONE;
			}
		}
		if (isrc &
		    (DATA_ORUNI | ERR_WRN | BUS_ERR | ERR_PASSIVE | ARB_LOST)) {
			/* error interrupt */
			if (sunxi_can_err(dev, isrc, status))
				break;
		}
		/* clear the interrupt */
		writel(isrc, priv->reg_base + CAN_INT_ADDR);
		udelay(10);
	}

	if (n >= SUNXI_CAN_MAX_IRQ)
		netdev_dbg(dev, "%d messages handled in ISR", n);

	return (n) ? IRQ_HANDLED : IRQ_NONE;
}

EXPORT_SYMBOL_GPL(sunxi_can_interrupt);

static int sunxi_can_open(struct net_device *dev)
{
	struct sunxi_can_priv *priv = netdev_priv(dev);
	int err;

	/* set chip into reset mode */
	set_reset_mode(dev);

	/* common open */
	err = open_candev(dev);
	if (err)
		return err;

	/* register interrupt handler, if not done by the device driver */
	if (!(priv->flags & SUNXI_CAN_CUSTOM_IRQ_HANDLER)) {
		err =
		    request_irq(dev->irq, sunxi_can_interrupt, priv->irq_flags,
				dev->name, (void *)dev);
		if (err) {
			close_candev(dev);
			printk(KERN_INFO "request_irq err:%d\n", err);
			return -EAGAIN;
		}
	}

	/* init and start chi */
	sunxi_can_start(dev);
	priv->open_time = jiffies;

	netif_start_queue(dev);

	return 0;
}

static int sunxi_can_close(struct net_device *dev)
{
	struct sunxi_can_priv *priv = netdev_priv(dev);

	netif_stop_queue(dev);
	set_reset_mode(dev);

	if (!(priv->flags & SUNXI_CAN_CUSTOM_IRQ_HANDLER))
		free_irq(dev->irq, (void *)dev);

	close_candev(dev);

	priv->open_time = 0;

	return 0;
}

struct net_device *alloc_sunxicandev(int sizeof_priv)
{
	struct net_device *dev;
	struct sunxi_can_priv *priv;

	dev = alloc_candev(sizeof(struct sunxi_can_priv) + sizeof_priv,
			   SUNXI_CAN_ECHO_SKB_MAX);
	if (!dev)
		return NULL;

	priv = netdev_priv(dev);

	priv->dev = dev;
	priv->can.bittiming_const = &sunxi_can_bittiming_const;
	priv->can.do_set_bittiming = sunxi_can_set_bittiming;
	priv->can.do_set_mode = sunxi_can_set_mode;
	priv->can.do_get_berr_counter = sunxi_can_get_berr_counter;
	priv->can.ctrlmode_supported = CAN_CTRLMODE_LOOPBACK |
	    CAN_CTRLMODE_LISTENONLY |
	    CAN_CTRLMODE_3_SAMPLES | CAN_CTRLMODE_BERR_REPORTING;

	spin_lock_init(&priv->cmdreg_lock);

	if (sizeof_priv)
		priv->priv = (void *)priv + sizeof(struct sunxi_can_priv);

	return dev;
}

EXPORT_SYMBOL_GPL(alloc_sunxicandev);

void free_sunxicandev(struct net_device *dev)
{
	free_candev(dev);
}

EXPORT_SYMBOL_GPL(free_sunxicandev);

static const struct net_device_ops sunxican_netdev_ops = {
	.ndo_open = sunxi_can_open,
	.ndo_stop = sunxi_can_close,
	.ndo_start_xmit = sunxi_can_start_xmit,
};

int register_sunxicandev(struct net_device *dev)
{
	if (!sunxi_can_probe(dev))
		return -ENODEV;

	dev->flags |= IFF_ECHO;	/* support local echo */
	dev->netdev_ops = &sunxican_netdev_ops;

	set_reset_mode(dev);

	return register_candev(dev);
}

void unregister_sunxicandev(struct net_device *dev)
{
	set_reset_mode(dev);
	unregister_candev(dev);
}

static const struct of_device_id sunxican_of_match[] = {
        { .compatible = "allwinner,sunxi-can", .data = &sun7i_devtype_data, },
	{ /* sentinel */ },
}

MODULE_DEVICE_TABLE(of, sunxican_of_match);

static const struct platform_device_id sunxican_id_table[] = {
        { .name = "sunxi-can", .driver_data = (kernel_ulong_t)&sun7i_devtype_data, },
        { /* sentinel */ },
};
MODULE_DEVICE_TABLE(platform, sunxican_id_table);

static __init int sunxi_can_init(void)
{
	struct sunxi_can_priv *priv;
	int err = 0;
	void __iomem *addr;
	struct clk *clk;

	/* 
	   int ret = 0;
	   int used = 0;
	 */

/*	clk = clk_get(&pdev->dev, "apb1_can");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "no clock defined\n");
		err = -ENODEV;
		goto exit_clk;
	}

        err = clk_prepare_enable(clk);
        if (err)
                goto exit_clk;
*/

	sunxican_dev = alloc_sunxicandev(0);
	if (!sunxican_dev) {
		printk(KERN_INFO "alloc sunxicandev fail\n");
	}
/*
	
		ret = script_parser_fetch("can_para", "can_used", &used, sizeof (used));
		if ( ret || used == 0) {
			printk(KERN_INFO "[sunxi-can] Cannot setup CANBus driver, maybe not configured in script.bin?");
			goto exit_free;
		}
*/


	priv = netdev_priv(sunxican_dev);
	printk(KERN_INFO "%s: mapping CAN io ...\n", DRV_NAME);
	addr = ioremap_nocache(CAN_BASE0,0x400);
	printk(KERN_INFO "%s: mapping CAN io done\n", DRV_NAME);
	priv->reg_base = addr;

	/* sunxican_dev->irq = SW_INT_IRQNO_CAN; */
	sunxican_dev->irq = 32 + SW_INT_IRQNO_CAN;
	priv->irq_flags = 0;
	priv->can.clock.freq = 24000000;
	chipset_init(sunxican_dev);
	err = register_sunxicandev(sunxican_dev);
	if (err) {
		dev_err(&sunxican_dev->dev, "registering %s failed (err=%d)\n",
			DRV_NAME, err);
		goto exit_free;
	}

	dev_info(&sunxican_dev->dev,
		 "%s device registered (reg_base=0x%08x, irq=%d)\n", DRV_NAME,
		 CAN_BASE0, sunxican_dev->irq);

	printk(KERN_INFO "%s CAN netdevice driver\n", DRV_NAME);

	return 0;

exit_free:
	free_sunxicandev(sunxican_dev);
exit_clk:
	return err;
}

static int __maybe_unused sunxi_can_suspend(struct device *device)
{
	struct net_device *dev = dev_get_drvdata(device);
	struct sunxi_can_priv *priv = netdev_priv(dev);
	int err;

	err = sunxi_chip_disable(priv);
	if (err)
		return err;

	if (netif_running(dev)) {
		netif_stop_queue(dev);
		netif_device_detach(dev);
	}
	priv->can.state = CAN_STATE_SLEEPING;

	return 0;
}

static int __maybe_unused sunxi_can_resume(struct device *device)
{
	struct net_device *dev = dev_get_drvdata(device);
	struct sunxi_can_priv *priv = netdev_priv(dev);

	priv->can.state = CAN_STATE_ERROR_ACTIVE;
	if (netif_running(dev)) {
		netif_device_attach(dev);
		netif_start_queue(dev);
	}
	return sunxi_can_chip_enable(priv);
}

static SIMPLE_DEV_PM_OPS( sunxi_can_pm_ops,  sunxi_can_suspend,  sunxi_can_resume);

static struct platform_driver sunxi_can_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.pm = &sunxi_can_pm_ops,
		.of_match_table = sunxican_of_match,
	},
	.probe = sunxi_can_probe,
	.remove = sunxi_can_remove,
	.id_table = sunxican_id_table,
};

module_platform_driver(sunxi_can_driver);

MODULE_AUTHOR("Gerhard Bertelsmann <info@gerhard-bertelsmann.de>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION(DRV_NAME "CAN netdevice driver");
