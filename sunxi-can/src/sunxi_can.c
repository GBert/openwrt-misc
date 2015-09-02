/*
 * sunxi_can.c - CAN bus controller driver for Allwinner SUN4I&SUN7I based SoCs
 *
 * Copyright (C) 2013 Peter Chen
 * Copyright (C) 2015 Gerhard Bertelsmann
 *   
 * Parts of this software are based on (derived from) the SJA1000 code by:
 *   Copyright (C) 2014 Oliver Hartkopp <oliver.hartkopp@volkswagen.de>
 *   Copyright (C) 2007 Wolfgang Grandegger <wg@grandegger.com>
 *   Copyright (C) 2002-2007 Volkswagen Group Electronic Research
 *   Copyright (C) 2003 Matthias Brukner, Trajet Gmbh, Rebenring 33,
 *   38106 Braunschweig, GERMANY
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
 *
 *
 * To use the driver CAN must be defined like:
 *  soc@01c00000 {
 *    ...
 *    pio: pinctrl@01c20800 {
 *      ...
 *      can0_pins_a: can0@0 {
 *        allwinner,pins = "PH20","PH21";
 *        allwinner,function = "can";
 *        allwinner,drive = <0>;
 *        allwinner,pull = <0>;
 *      };
 *    };
 *    ...
 *    can0: can@01c2bc00 {
 *      compatible = "allwinner,sunxican";
 *      reg = <0x01c2bc00 0x400>;
 *      reg-io-width = <4>;
 *      interrupts = <0 26 4>;
 *      clocks = <&apb1_gates 4>;
 *      #address-cells = <1>;
 *      #size-cells = <0>;
 *    };
 *    ...
 *  };
 *   
 */

#include <linux/netdevice.h>
#include <linux/can.h>
#include <linux/can/dev.h>
#include <linux/can/error.h>
#include <linux/can/led.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#define DRV_NAME "sunxi_can"
#define DRV_MODULE_VERSION "0.91"

/* Registers address */
#define CAN_BASE0		0x01C2BC00
#define CAN_MSEL_ADDR		0x0000	/* CAN Mode Select Register */
#define CAN_CMD_ADDR            0x0004	/* CAN Command Register */
#define CAN_STA_ADDR		0x0008	/* CAN Status Register */
#define CAN_INT_ADDR		0x000c	/* CAN Interrupt Flag Register */
#define CAN_INTEN_ADDR		0x0010	/* CAN Interrupt Enable Register */
#define CAN_BTIME_ADDR		0x0014	/* CAN Bus Timing 0 Register */
#define CAN_TEWL_ADDR		0x0018	/* CAN Tx Error Warning Limit Register */
#define CAN_ERRC_ADDR		0x001c	/* CAN Error Counter Register */
#define CAN_RMCNT_ADDR		0x0020	/* CAN Receive Message Counter Register */
#define CAN_RBUFSA_ADDR		0x0024	/* CAN Receive Buffer Start Address Register */
#define CAN_BUF0_ADDR		0x0040	/* CAN Tx/Rx Buffer 0 Register */
#define CAN_BUF1_ADDR		0x0044	/* CAN Tx/Rx Buffer 1 Register */
#define CAN_BUF2_ADDR		0x0048	/* CAN Tx/Rx Buffer 2 Register */
#define CAN_BUF3_ADDR		0x004c	/* CAN Tx/Rx Buffer 3 Register */
#define CAN_BUF4_ADDR		0x0050	/* CAN Tx/Rx Buffer 4 Register */
#define CAN_BUF5_ADDR		0x0054	/* CAN Tx/Rx Buffer 5 Register */
#define CAN_BUF6_ADDR		0x0058	/* CAN Tx/Rx Buffer 6 Register */
#define CAN_BUF7_ADDR		0x005c	/* CAN Tx/Rx Buffer 7 Register */
#define CAN_BUF8_ADDR		0x0060	/* CAN Tx/Rx Buffer 8 Register */
#define CAN_BUF9_ADDR		0x0064	/* CAN Tx/Rx Buffer 9 Register */
#define CAN_BUF10_ADDR		0x0068	/* CAN Tx/Rx Buffer 10 Register */
#define CAN_BUF11_ADDR		0x006c	/* CAN Tx/Rx Buffer 11 Register */
#define CAN_BUF12_ADDR		0x0070	/* CAN Tx/Rx Buffer 12 Register */
#define CAN_ACPC_ADDR		0x0040	/* CAN Acceptance Code 0 Register */
#define CAN_ACPM_ADDR		0x0044	/* CAN Acceptance Mask 0 Register */
#define CAN_RBUF_RBACK_START_ADDR	+ 0x0180	/* CAN transmit buffer for read back register */
#define CAN_RBUF_RBACK_END_ADDR		+ 0x01b0	/* CAN transmit buffer for read back register */

/* Controller Register Description */

/* mode select register (r/w)
 * offset:0x0000 default:0x0000_0001
 */
#define SLEEP_MODE		(0x01<<4)	/* This bit can only be written in Reset Mode */
#define WAKE_UP			(0x00<<4)
#define SINGLE_FILTER		(0x01<<3)	/* This bit can only be written in Reset Mode */
#define DUAL_FILTERS		(0x00<<3)
#define LOOPBACK_MODE		BIT(2)
#define LISTEN_ONLY_MODE	BIT(1)
#define RESET_MODE		BIT(0)

/* command register (w)
 * offset:0x0004 default:0x0000_0000
 */
#define BUS_OFF_REQ		BIT(5)
#define SELF_RCV_REQ		BIT(4)
#define CLEAR_OR_FLAG		BIT(3)
#define RELEASE_RBUF		BIT(2)
#define ABORT_REQ		BIT(1)
#define TRANS_REQ		BIT(0)

/* status register (r)
 * offset:0x0008 default:0x0000_003c
 */
#define BIT_ERR			(0x00<<22)
#define FORM_ERR		(0x01<<22)
#define STUFF_ERR		(0x02<<22)
#define OTHER_ERR		(0x03<<22)
#define ERR_DIR			BIT(21)
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
#define BERR_IRQ_EN		BIT(7)
#define ARB_LOST_IRQ_EN		BIT(6)
#define ERR_PASSIVE_IRQ_EN	BIT(5)
#define WAKEUP_IRQ_EN		BIT(4)
#define OR_IRQ_EN		BIT(3)
#define ERR_WRN_IRQ_EN		BIT(2)
#define TX_IRQ_EN		BIT(1)
#define RX_IRQ_EN		BIT(0)

/* error code */
#define ERR_INRCV		(0x1<<5)
#define ERR_INTRANS		(0x0<<5)

/* filter mode */
#define FILTER_CLOSE		0
#define SINGLE_FLTER_MODE	1
#define DUAL_FILTER_MODE	2

#define SUNXI_CAN_MAX_IRQ	20	/* max. number of interrupts handled in ISR */

struct sunxican_priv {
	struct can_priv can;
	struct net_device *dev;
	struct napi_struct napi;

	int open_time;
	struct sk_buff *echo_skb;
	void __iomem *base;
	struct clk *clk;

	void *priv;		/* for board-specific data */

	unsigned long irq_flags;	/* for request_irq() */
	spinlock_t cmdreg_lock;	/* lock for concurrent cmd register writes */

	u16 flags;		/* custom mode flags */
};

static const struct can_bittiming_const sunxican_bittiming_const = {
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

static void sunxi_can_write_cmdreg(struct sunxican_priv *priv, u8 val)
{
	unsigned long flags;

	/*
	 * The command register needs some locking and time to settle
	 * the write_reg() operation - especially on SMP systems.
	 */
	spin_lock_irqsave(&priv->cmdreg_lock, flags);
	writel(val, priv->base + CAN_CMD_ADDR);
	readl(priv->base + CAN_STA_ADDR);
	spin_unlock_irqrestore(&priv->cmdreg_lock, flags);
}

static void set_normal_mode(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	u8 status = readl(priv->base + CAN_MSEL_ADDR);
	int i;

	for (i = 0; i < 100; i++) {
		/* check reset bit */
		if ((status & RESET_MODE) == 0) {
			priv->can.state = CAN_STATE_ERROR_ACTIVE;
			/* enable interrupts */
			if (priv->can.ctrlmode & CAN_CTRLMODE_BERR_REPORTING) {
				writel(0xFFFF, priv->base + CAN_INTEN_ADDR);
			} else {
				writel(0xFFFF & ~BERR_IRQ_EN,
				       priv->base + CAN_INTEN_ADDR);
			}

			if (priv->can.ctrlmode & CAN_CTRLMODE_LOOPBACK) {
				/* Put device into loopback mode */
				writel(readl(priv->base + CAN_MSEL_ADDR) |
				       LOOPBACK_MODE,
				       priv->base + CAN_MSEL_ADDR);
			} else if (priv->can.ctrlmode & CAN_CTRLMODE_LISTENONLY) {
				/* Put device into listen-only mode */
				writel(readl(priv->base + CAN_MSEL_ADDR) |
				       LISTEN_ONLY_MODE,
				       priv->base + CAN_MSEL_ADDR);
			}
			return;
		}

		/* set chip to normal mode */
		writel(readl(priv->base + CAN_MSEL_ADDR) & (~RESET_MODE),
		       priv->base + CAN_MSEL_ADDR);
		udelay(10);
		status = readl(priv->base + CAN_MSEL_ADDR);
	}

	netdev_err(dev, "setting controller into normal mode failed!\n");
}

static void set_reset_mode(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	u8 status = readl(priv->base + CAN_MSEL_ADDR);
	int i;

	for (i = 0; i < 100; i++) {
		/* check reset bit */
		if (status & RESET_MODE) {
			priv->can.state = CAN_STATE_STOPPED;
			return;
		}

		writel(readl(priv->base + CAN_MSEL_ADDR) | RESET_MODE, priv->base + CAN_MSEL_ADDR);	/* select reset mode */
		/* select reset mode */
		/* writel(RESET_MODE, priv->base, CAN_MSEL_ADDR); */
		udelay(10);
		status = readl(priv->base + CAN_MSEL_ADDR);
	}

	netdev_err(dev, "setting controller into reset mode failed!\n");
}

static int sunxican_set_bittiming(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	struct can_bittiming *bt = &priv->can.bittiming;
	u32 cfg;

	cfg = ((bt->brp - 1) & 0x3FF) |
	    (((bt->sjw - 1) & 0x3) << 14) |
	    (((bt->prop_seg + bt->phase_seg1 - 1) & 0xf) << 16) |
	    (((bt->phase_seg2 - 1) & 0x7) << 20);
	if (priv->can.ctrlmode & CAN_CTRLMODE_3_SAMPLES)
		cfg |= 0x800000;

	netdev_info(dev, "setting BITTIMING=0x%08x\n", cfg);

	/* CAN_BTIME_ADDR only writable in reset mode */
	set_reset_mode(dev);
	writel(cfg, priv->base + CAN_BTIME_ADDR);
	set_normal_mode(dev);

	return 0;
}

static int sunxican_get_berr_counter(const struct net_device *dev,
				     struct can_berr_counter *bec)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	u32 errors;
	errors = readl(priv->base + CAN_ERRC_ADDR);
	bec->txerr = errors & 0x000F;
	bec->rxerr = (errors  >> 16) & 0x000F;
	return 0;
}

static void sunxi_can_start(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);

	/* leave reset mode */
	if (priv->can.state != CAN_STATE_STOPPED)
		set_reset_mode(dev);

	/* set filters - we accept all */
	writel(0x00000000, priv->base + CAN_ACPC_ADDR);
	writel(0xffffffff, priv->base + CAN_ACPM_ADDR);

	/* Clear error counters and error code capture */
	writel(0x0, priv->base + CAN_ERRC_ADDR);

	/* leave reset mode */
	set_normal_mode(dev);
}

static int sunxican_set_mode(struct net_device *dev, enum can_mode mode)
{
	struct sunxican_priv *priv = netdev_priv(dev);

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

/*
 * transmit a CAN message
 * message layout in the sk_buff should be like this:
 * xx xx xx xx         ff         ll 00 11 22 33 44 55 66 77
 * [ can_id ] [flags] [len] [can data (up to 8 bytes]
 */
static int sunxican_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	struct can_frame *cf = (struct can_frame *)skb->data;
	u8 dlc;
	canid_t id;
	u32 temp = 0;
	int i;

	/* wait buffer ready */
	while (!(readl(priv->base + CAN_STA_ADDR) & TBUF_RDY))
		usleep_range(10,100);

	if (can_dropped_invalid_skb(dev, skb))
		return NETDEV_TX_OK;

	netif_stop_queue(dev);

	dlc = cf->can_dlc;
	id = cf->can_id;

	temp = ((id >> 30) << 6) | dlc;
	writel(temp, priv->base + CAN_BUF0_ADDR);
	if (id & CAN_EFF_FLAG) {	/* extended frame */
		writel((id >> 21) & 0xFF, priv->base + CAN_BUF1_ADDR);	/* id28~21 */
		writel((id >> 13) & 0xFF, priv->base + CAN_BUF2_ADDR);	/* id20~13 */
		writel((id >> 5)  & 0xFF, priv->base + CAN_BUF3_ADDR);	/* id12~5  */
		writel((id & 0x1F) << 3,  priv->base + CAN_BUF4_ADDR);	/* id4~0   */

		for (i = 0; i < dlc; i++) {
			writel(cf->data[i],
			       priv->base + (CAN_BUF5_ADDR + i * 4));
		}
	} else {		/* standard frame */
		writel((id >> 3) & 0xFF, priv->base + CAN_BUF1_ADDR);	/* id28~21 */
		writel((id & 0x7) << 5, priv->base + CAN_BUF2_ADDR);	/* id20~13 */

		for (i = 0; i < dlc; i++) {
			writel(cf->data[i], priv->base + CAN_BUF3_ADDR + i * 4);
		}
	}

	can_put_echo_skb(skb, dev, 0);
	sunxi_can_write_cmdreg(priv, TRANS_REQ);

	return NETDEV_TX_OK;
}

static void sunxi_can_rx(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	u8 fi;
	canid_t id;
	int i;

	/* create zero'ed CAN frame buffer */
	skb = alloc_can_skb(dev, &cf);
	if (skb == NULL)
		return;

	fi = readl(priv->base + CAN_BUF0_ADDR);
	cf->can_dlc = get_can_dlc(fi & 0x0F);
	if (fi >> 7) {
		/* extended frame format (EFF) */
		id = (readl(priv->base + CAN_BUF1_ADDR) << 21) |	/* id28~21 */
		     (readl(priv->base + CAN_BUF2_ADDR) << 13) |	/* id20~13 */
		     (readl(priv->base + CAN_BUF3_ADDR) << 5)  |	/* id12~5  */
		    ((readl(priv->base + CAN_BUF4_ADDR) >> 3)  & 0x1f);	/* id4~0   */
		id |= CAN_EFF_FLAG;

		/* remote transmission request ? */
		if ((fi >> 6) & 0x1)
			id |= CAN_RTR_FLAG;
		else {
			for (i = 0; i < cf->can_dlc; i++)
				cf->data[i] =
				    readl(priv->base + CAN_BUF5_ADDR + i * 4);
		}
	} else {
		/* standard frame format (SFF) */
		id = (readl(priv->base + CAN_BUF1_ADDR) << 3) |	/* id28~21 */
		    ((readl(priv->base + CAN_BUF2_ADDR) >> 5) & 0x7);	/* id20~18 */

		/* remote transmission request ? */
		if ((fi >> 6) & 0x1)
			id |= CAN_RTR_FLAG;
		else {
			for (i = 0; i < cf->can_dlc; i++)
				cf->data[i] =
				    readl(priv->base + CAN_BUF3_ADDR + i * 4);
		}
	}

	cf->can_id = id;

	/* release receive buffer */
	sunxi_can_write_cmdreg(priv, RELEASE_RBUF);

	netif_rx(skb);

	stats->rx_packets++;
	stats->rx_bytes += cf->can_dlc;

	can_led_event(dev, CAN_LED_EVENT_RX);
}

static int sunxi_can_err(struct net_device *dev, u8 isrc, u8 status)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	enum can_state state = priv->can.state;
	u32 ecc, alc;

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
		sunxi_can_write_cmdreg(priv, CLEAR_OR_FLAG);	/* clear bit */
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

		ecc = readl(priv->base + CAN_STA_ADDR);

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
		alc = readl(priv->base + CAN_STA_ADDR);
		priv->can.can_stats.arbitration_lost++;
		stats->tx_errors++;
		cf->can_id |= CAN_ERR_LOSTARB;
		cf->data[0] = (alc & 0x1f) >> 8;
	}

	if (state != priv->can.state && (state == CAN_STATE_ERROR_WARNING ||
					 state == CAN_STATE_ERROR_PASSIVE)) {
		u8 rxerr =
		    (readl(priv->base + CAN_ERRC_ADDR) >> 16) & 0xFF;
		u8 txerr = readl(priv->base + CAN_ERRC_ADDR) & 0xFF;
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
	struct sunxican_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	u8 isrc, status;
	int n = 0;

	/* Shared interrupts and IRQ off? */
	if ((readl(priv->base + CAN_INT_ADDR) & 0xF) == 0x0)
		return IRQ_NONE;

	while ((isrc = readl(priv->base + CAN_INT_ADDR))
	       && (n < SUNXI_CAN_MAX_IRQ)) {
		n++;
		status = readl(priv->base + CAN_STA_ADDR);

		if (isrc & WAKEUP)
			netdev_warn(dev, "wakeup interrupt\n");

		if (isrc & TBUF_VLD) {
			/* transmission complete interrupt */
			stats->tx_bytes +=
			    readl(priv->base + CAN_RBUF_RBACK_START_ADDR) & 0xf;
			stats->tx_packets++;
			can_get_echo_skb(dev, 0);
			netif_wake_queue(dev);
			can_led_event(dev, CAN_LED_EVENT_TX);
		}
		if (isrc & RBUF_VLD) {
			/* receive interrupt */
			while (status & RBUF_RDY) {	/* RX buffer is not empty */
				sunxi_can_rx(dev);
				status = readl(priv->base + CAN_STA_ADDR);
			}
		}
		if (isrc &
		    (DATA_ORUNI | ERR_WRN | BUS_ERR | ERR_PASSIVE | ARB_LOST)) {
			/* error interrupt */
			if (sunxi_can_err(dev, isrc, status))
				break;
		}
		/* clear the interrupt */
		writel(isrc, priv->base + CAN_INT_ADDR);
		udelay(10);
	}

	if (n >= SUNXI_CAN_MAX_IRQ)
		netdev_dbg(dev, "%d messages handled in ISR", n);

	return (n) ? IRQ_HANDLED : IRQ_NONE;
}

EXPORT_SYMBOL_GPL(sunxi_can_interrupt);

static int sunxican_open(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	int err;

	/* set chip into reset mode */
	set_reset_mode(dev);

	/* common open */
	err = open_candev(dev);
	if (err)
		return err;

	/* register interrupt handler */
	if (request_irq
	    (dev->irq, sunxi_can_interrupt, priv->irq_flags, dev->name,
	     (void *)dev)) {
		netdev_err(dev, "request_irq err: %d\n", err);
		return -EAGAIN;
	}

	sunxican_set_bittiming(dev);
	sunxi_can_start(dev);

	can_led_event(dev, CAN_LED_EVENT_OPEN);

	netif_start_queue(dev);

	return 0;
}

static int sunxican_close(struct net_device *dev)
{
	netif_stop_queue(dev);
	set_reset_mode(dev);

	free_irq(dev->irq, (void *)dev);

	close_candev(dev);

	can_led_event(dev, CAN_LED_EVENT_STOP);

	return 0;
}

void free_sunxicandev(struct net_device *dev)
{
	free_candev(dev);
}

EXPORT_SYMBOL_GPL(free_sunxicandev);

static const struct net_device_ops sunxican_netdev_ops = {
	.ndo_open = sunxican_open,
	.ndo_stop = sunxican_close,
	.ndo_start_xmit = sunxican_start_xmit,
};

static const struct of_device_id sunxican_of_match[] = {
	{.compatible = "allwinner,sunxican"},
	{},
}

MODULE_DEVICE_TABLE(of, sunxican_of_match);

static const struct platform_device_id sunxican_id_table[] = {
	{.name = "sunxican"},
	{},
};

MODULE_DEVICE_TABLE(platform, sunxican_id_table);

static int sunxican_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct sunxican_priv *priv = netdev_priv(dev);
	struct resource *res;

	set_reset_mode(dev);
	unregister_netdev(dev);
	iounmap(priv->base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(res->start, resource_size(res));

	free_candev(dev);

	return 0;
}

static int sunxican_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct clk *clk;
	void __iomem *addr;
	int err, irq;
	u32 temp_irqen;
	struct net_device *dev;
	struct sunxican_priv *priv;

	clk = clk_get(&pdev->dev, "apb1_can");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "no clock defined\n");
		err = -ENODEV;
		goto exit;
	}

	/* turn on clocking for CAN peripheral block */
	err = clk_prepare_enable(clk);
	if (err) {
		dev_err(&pdev->dev, "could not enable clocking (apb1_can)\n");
		goto exit;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);

	if (!res || irq <= 0) {
		dev_err(&pdev->dev, "could not get a valid irq\n");
		err = -ENODEV;
		goto exit_put;
	}

	if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
		dev_err(&pdev->dev, "could not get io memory resource\n");
		err = -EBUSY;
		goto exit_put;
	}

	addr = ioremap_nocache(res->start, resource_size(res));
	if (!addr) {
		dev_err(&pdev->dev, "could not map io memory\n");
		err = -ENOMEM;
		goto exit_release;
	}

	dev = alloc_candev(sizeof(struct sunxican_priv), 1);

	if (!dev) {
		dev_err(&pdev->dev, "could not allocate memory for CAN device\n");
		err = -ENOMEM;
		goto exit_iounmap;
	}

	dev->netdev_ops = &sunxican_netdev_ops;
	dev->irq = irq;
	dev->flags |= IFF_ECHO;

	priv = netdev_priv(dev);
	priv->can.clock.freq = clk_get_rate(clk);
	priv->can.bittiming_const = &sunxican_bittiming_const;
	priv->can.do_set_mode = sunxican_set_mode;
	priv->can.do_get_berr_counter = sunxican_get_berr_counter;
	priv->can.ctrlmode_supported = CAN_CTRLMODE_BERR_REPORTING |
	    CAN_CTRLMODE_LISTENONLY | CAN_CTRLMODE_LOOPBACK |
	    CAN_CTRLMODE_3_SAMPLES;
	priv->dev = dev;
	priv->base = addr;
	priv->clk = clk;
	spin_lock_init(&priv->cmdreg_lock);

	/* enable CAN specific interrupts */
	set_reset_mode(dev);
	temp_irqen = BERR_IRQ_EN | ERR_PASSIVE_IRQ_EN | OR_IRQ_EN | RX_IRQ_EN;
	writel(readl(priv->base + CAN_INTEN_ADDR) | temp_irqen,
	       priv->base + CAN_INTEN_ADDR);

	platform_set_drvdata(pdev, dev);
	SET_NETDEV_DEV(dev, &pdev->dev);

	err = register_candev(dev);
	if (err) {
		dev_err(&pdev->dev, "registering %s failed (err=%d)\n",
			DRV_NAME, err);
		goto exit_free;
	}

	devm_can_led_init(dev);

	dev_info(&pdev->dev, "device registered (base=%p, irq=%d)\n",
		 priv->base, dev->irq);

	return 0;

exit_free:
	free_candev(dev);
exit_iounmap:
	iounmap(addr);
exit_release:
	release_mem_region(res->start, resource_size(res));
exit_put:
	clk_put(clk);
exit:
	return err;
}

static int __maybe_unused sunxi_can_suspend(struct device *device)
{
	struct net_device *dev = dev_get_drvdata(device);
	struct sunxican_priv *priv = netdev_priv(dev);

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
	struct sunxican_priv *priv = netdev_priv(dev);

	priv->can.state = CAN_STATE_ERROR_ACTIVE;
	if (netif_running(dev)) {
		netif_device_attach(dev);
		netif_start_queue(dev);
	}
	return 0;
}

static SIMPLE_DEV_PM_OPS(sunxi_can_pm_ops, sunxi_can_suspend, sunxi_can_resume);

static struct platform_driver sunxi_can_driver = {
	.driver = {
		   .name = DRV_NAME,
		   .owner = THIS_MODULE,
		   .pm = &sunxi_can_pm_ops,
		   .of_match_table = sunxican_of_match,
		   },
	.probe = sunxican_probe,
	.remove = sunxican_remove,
	.id_table = sunxican_id_table,
};

module_platform_driver(sunxi_can_driver);

MODULE_AUTHOR("Peter Chen <xingkongcp@gmail.com>, "
	      "Gerhard Bertelsmann <info@gerhard-bertelsmann.de>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION(DRV_NAME "CAN driver for Allwinner SoCs (A10/A20)");
MODULE_VERSION(DRV_MODULE_VERSION);
