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
#define DRV_MODULE_VERSION "0.98"

/* Registers address (physical base address 0x01C2BC00) */
#define SUNXI_REG_MSEL_ADDR	0x0000	/* CAN Mode Select */
#define SUNXI_REG_CMD_ADDR	0x0004	/* CAN Command */
#define SUNXI_REG_STA_ADDR	0x0008	/* CAN Status */
#define SUNXI_REG_INT_ADDR	0x000c	/* CAN Interrupt Flag */
#define SUNXI_REG_INTEN_ADDR	0x0010	/* CAN Interrupt Enable */
#define SUNXI_REG_BTIME_ADDR	0x0014	/* CAN Bus Timing 0 */
#define SUNXI_REG_TEWL_ADDR	0x0018	/* CAN Tx Error Warning Limit */
#define SUNXI_REG_ERRC_ADDR	0x001c	/* CAN Error Counter */
#define SUNXI_REG_RMCNT_ADDR	0x0020	/* CAN Receive Message Counter */
#define SUNXI_REG_RBUFSA_ADDR	0x0024	/* CAN Receive Buffer Start Address */
#define SUNXI_REG_BUF0_ADDR	0x0040	/* CAN Tx/Rx Buffer 0 */
#define SUNXI_REG_BUF1_ADDR	0x0044	/* CAN Tx/Rx Buffer 1 */
#define SUNXI_REG_BUF2_ADDR	0x0048	/* CAN Tx/Rx Buffer 2 */
#define SUNXI_REG_BUF3_ADDR	0x004c	/* CAN Tx/Rx Buffer 3 */
#define SUNXI_REG_BUF4_ADDR	0x0050	/* CAN Tx/Rx Buffer 4 */
#define SUNXI_REG_BUF5_ADDR	0x0054	/* CAN Tx/Rx Buffer 5 */
#define SUNXI_REG_BUF6_ADDR	0x0058	/* CAN Tx/Rx Buffer 6 */
#define SUNXI_REG_BUF7_ADDR	0x005c	/* CAN Tx/Rx Buffer 7 */
#define SUNXI_REG_BUF8_ADDR	0x0060	/* CAN Tx/Rx Buffer 8 */
#define SUNXI_REG_BUF9_ADDR	0x0064	/* CAN Tx/Rx Buffer 9 */
#define SUNXI_REG_BUF10_ADDR	0x0068	/* CAN Tx/Rx Buffer 10 */
#define SUNXI_REG_BUF11_ADDR	0x006c	/* CAN Tx/Rx Buffer 11 */
#define SUNXI_REG_BUF12_ADDR	0x0070	/* CAN Tx/Rx Buffer 12 */
#define SUNXI_REG_ACPC_ADDR	0x0040	/* CAN Acceptance Code 0 */
#define SUNXI_REG_ACPM_ADDR	0x0044	/* CAN Acceptance Mask 0 */
#define SUNXI_REG_RBUF_RBACK_START_ADDR	0x0180	/* CAN transmit buffer start */
#define SUNXI_REG_RBUF_RBACK_END_ADDR	0x01b0	/* CAN transmit buffer end */

/* Controller Register Description */

/* mode select register (r/w)
 * offset:0x0000 default:0x0000_0001
 */
#define SUNXI_SLEEP_MODE	(0x01 << 4)	/* write in reset mode */
#define SUNXI_WAKE_UP		(0x00 << 4)
#define SUNXI_SINGLE_FILTER	(0x01 << 3)	/* write in reset mode */
#define SUNXI_DUAL_FILTERS	(0x00 << 3)
#define SUNXI_LOOPBACK_MODE	BIT(2)
#define SUNXI_LISTEN_ONLY_MODE	BIT(1)
#define SUNXI_RESET_MODE	BIT(0)

/* command register (w)
 * offset:0x0004 default:0x0000_0000
 */
#define SUNXI_CMD_BUS_OFF_REQ	BIT(5)
#define SUNXI_CMD_SELF_RCV_REQ	BIT(4)
#define SUNXI_CMD_CLEAR_OR_FLAG	BIT(3)
#define SUNXI_CMD_RELEASE_RBUF	BIT(2)
#define SUNXI_CMD_ABORT_REQ	BIT(1)
#define SUNXI_CMD_TRANS_REQ	BIT(0)

/* status register (r)
 * offset:0x0008 default:0x0000_003c
 */
#define SUNXI_STA_BIT_ERR	(0x00 << 22)
#define SUNXI_STA_FORM_ERR	(0x01 << 22)
#define SUNXI_STA_STUFF_ERR	(0x02 << 22)
#define SUNXI_STA_OTHER_ERR	(0x03 << 22)
#define SUNXI_STA_MASK_ERR	(0x03 << 22)
#define SUNXI_STA_ERR_DIR	BIT(21)
#define SUNXI_STA_ERR_SEG_CODE	(0x1f << 16)
#define SUNXI_STA_START		(0x03 << 16)
#define SUNXI_STA_ID28_21	(0x02 << 16)
#define SUNXI_STA_ID20_18	(0x06 << 16)
#define SUNXI_STA_SRTR		(0x04 << 16)
#define SUNXI_STA_IDE		(0x05 << 16)
#define SUNXI_STA_ID17_13	(0x07 << 16)
#define SUNXI_STA_ID12_5	(0x0f << 16)
#define SUNXI_STA_ID4_0		(0x0e << 16)
#define SUNXI_STA_RTR		(0x0c << 16)
#define SUNXI_STA_RB1		(0x0d << 16)
#define SUNXI_STA_RB0		(0x09 << 16)
#define SUNXI_STA_DLEN		(0x0b << 16)
#define SUNXI_STA_DATA_FIELD	(0x0a << 16)
#define SUNXI_STA_CRC_SEQUENCE	(0x08 << 16)
#define SUNXI_STA_CRC_DELIMITER	(0x18 << 16)
#define SUNXI_STA_ACK		(0x19 << 16)
#define SUNXI_STA_ACK_DELIMITER	(0x1b << 16)
#define SUNXI_STA_END		(0x1a << 16)
#define SUNXI_STA_INTERMISSION	(0x12 << 16)
#define SUNXI_STA_ACTIVE_ERROR	(0x11 << 16)
#define SUNXI_STA_PASSIVE_ERROR	(0x16 << 16)
#define SUNXI_STA_TOLERATE_DOMINANT_BITS	(0x13 << 16)
#define SUNXI_STA_ERROR_DELIMITER	(0x17 << 16)
#define SUNXI_STA_OVERLOAD	(0x1c << 16)
#define SUNXI_STA_BUS_OFF	BIT(7)
#define SUNXI_STA_ERR_STA	BIT(6)
#define SUNXI_STA_TRANS_BUSY	BIT(5)
#define SUNXI_STA_RCV_BUSY	BIT(4)
#define SUNXI_STA_TRANS_OVER	BIT(3)
#define SUNXI_STA_TBUF_RDY	BIT(2)
#define SUNXI_STA_DATA_ORUN	BIT(1)
#define SUNXI_STA_RBUF_RDY	BIT(0)

/* interrupt register (r)
 * offset:0x000c default:0x0000_0000
 */
#define SUNXI_INT_BUS_ERR	BIT(7)
#define SUNXI_INT_ARB_LOST	BIT(6)
#define SUNXI_INT_ERR_PASSIVE	BIT(5)
#define SUNXI_INT_WAKEUP	BIT(4)
#define SUNXI_INT_DATA_OR	BIT(3)
#define SUNXI_INT_ERR_WRN	BIT(2)
#define SUNXI_INT_TBUF_VLD	BIT(1)
#define SUNXI_INT_RBUF_VLD	BIT(0)

/* interrupt enable register (r/w)
 * offset:0x0010 default:0x0000_0000
 */
#define SUNXI_INTEN_BERR	BIT(7)
#define SUNXI_INTEN_ARB_LOST	BIT(6)
#define SUNXI_INTEN_ERR_PASSIVE	BIT(5)
#define SUNXI_INTEN_WAKEUP	BIT(4)
#define SUNXI_INTEN_OR		BIT(3)
#define SUNXI_INTEN_ERR_WRN	BIT(2)
#define SUNXI_INTEN_TX		BIT(1)
#define SUNXI_INTEN_RX		BIT(0)

/* error code */
#define SUNXI_ERR_INRCV		(0x1 << 5)
#define SUNXI_ERR_INTRANS	(0x0 << 5)

/* filter mode */
#define SINXI_FILTER_CLOSE	0
#define SUNXI_SINGLE_FLTER_MODE	1
#define SUNXI_DUAL_FILTER_MODE	2

/* message buffer flags */
#define SUNXI_MSG_EFF_FLAG	BIT(7)
#define SUNXI_MSG_RTR_FLAG	BIT(6)

/* max. number of interrupts handled in ISR */
#define SUNXI_CAN_MAX_IRQ	20
#define SUNXI_MODE_MAX_RETRIES	100
struct sunxican_priv {
	struct can_priv can;
	void __iomem *base;
	struct clk *clk;
	spinlock_t cmdreg_lock;	/* lock for concurrent cmd register writes */
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

	spin_lock_irqsave(&priv->cmdreg_lock, flags);
	writel(val, priv->base + SUNXI_REG_CMD_ADDR);
	spin_unlock_irqrestore(&priv->cmdreg_lock, flags);
}

static int set_normal_mode(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	int retry = SUNXI_MODE_MAX_RETRIES;
	u32 mod_reg_val = 0;

	while (retry-- &&
	       (readl(priv->base + SUNXI_REG_MSEL_ADDR) & SUNXI_RESET_MODE))
		writel(readl(priv->base + SUNXI_REG_MSEL_ADDR) &
		       (~SUNXI_RESET_MODE), priv->base + SUNXI_REG_MSEL_ADDR);

	if (readl(priv->base + SUNXI_REG_MSEL_ADDR) & SUNXI_RESET_MODE) {
		netdev_err(dev,
			   "setting controller into normal mode failed!\n");
		return -ETIMEDOUT;
	}

	priv->can.state = CAN_STATE_ERROR_ACTIVE;
	/* enable interrupts */
	if (priv->can.ctrlmode & CAN_CTRLMODE_BERR_REPORTING)
		writel(0xFFFF, priv->base + SUNXI_REG_INTEN_ADDR);
	else
		writel(0xFFFF & ~SUNXI_INTEN_BERR,
		       priv->base + SUNXI_REG_INTEN_ADDR);

	mod_reg_val = readl(priv->base + SUNXI_REG_MSEL_ADDR);

	if (priv->can.ctrlmode & CAN_CTRLMODE_PRESUME_ACK)
		mod_reg_val |= SUNXI_LOOPBACK_MODE;
	else if (priv->can.ctrlmode & CAN_CTRLMODE_LISTENONLY)
		mod_reg_val |= SUNXI_LISTEN_ONLY_MODE;

	writel(mod_reg_val, priv->base + SUNXI_REG_MSEL_ADDR);
	return 0;
}

static int set_reset_mode(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	int retry = SUNXI_MODE_MAX_RETRIES;

	while (retry-- &&
	       !(readl(priv->base + SUNXI_REG_MSEL_ADDR) & SUNXI_RESET_MODE))
		writel(readl(priv->base + SUNXI_REG_MSEL_ADDR) |
		       SUNXI_RESET_MODE, priv->base + SUNXI_REG_MSEL_ADDR);

	if (!(readl(priv->base + SUNXI_REG_MSEL_ADDR) & SUNXI_RESET_MODE)) {
		netdev_err(dev, "setting controller into reset mode failed!\n");
		return -ETIMEDOUT;
	}

	priv->can.state = CAN_STATE_STOPPED;

	return 0;
}

static int sunxican_set_bittiming(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	struct can_bittiming *bt = &priv->can.bittiming;
	int err;
	u32 cfg;

	cfg = ((bt->brp - 1) & 0x3FF) |
	    (((bt->sjw - 1) & 0x3) << 14) |
	    (((bt->prop_seg + bt->phase_seg1 - 1) & 0xf) << 16) |
	    (((bt->phase_seg2 - 1) & 0x7) << 20);
	if (priv->can.ctrlmode & CAN_CTRLMODE_3_SAMPLES)
		cfg |= 0x800000;

	netdev_info(dev, "setting BITTIMING=0x%08x\n", cfg);

	/* CAN_BTIME_ADDR only writable in reset mode */
	err = set_reset_mode(dev);
	if (err)
		return err;
	writel(cfg, priv->base + SUNXI_REG_BTIME_ADDR);

	return set_normal_mode(dev);
}

static int sunxican_get_berr_counter(const struct net_device *dev,
				     struct can_berr_counter *bec)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	u32 errors;

	errors = readl(priv->base + SUNXI_REG_ERRC_ADDR);
	bec->txerr = errors & 0x000F;
	bec->rxerr = (errors >> 16) & 0x000F;
	return 0;
}

static int sunxi_can_start(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	int err;
	u32 temp_irqen;

	/* turn on clocking for CAN peripheral block */
	err = clk_prepare_enable(priv->clk);
	if (err) {
		netdev_err(dev, "could not enable clocking (apb1_can)\n");
		goto exit;
	}

	/* leave reset mode */
	if (priv->can.state != CAN_STATE_STOPPED)
		set_reset_mode(dev);

	/* set filters - we accept all */
	writel(0x00000000, priv->base + SUNXI_REG_ACPC_ADDR);
	writel(0xFFFFFFFF, priv->base + SUNXI_REG_ACPM_ADDR);

	/* Clear error counters and error code capture */
	writel(0x0, priv->base + SUNXI_REG_ERRC_ADDR);

	/* enable CAN specific interrupts */
	temp_irqen = SUNXI_INTEN_BERR | SUNXI_INTEN_ERR_PASSIVE |
		     SUNXI_INTEN_OR | SUNXI_INTEN_RX;
	writel(readl(priv->base + SUNXI_REG_INTEN_ADDR) | temp_irqen,
	       priv->base + SUNXI_REG_INTEN_ADDR);

	err = sunxican_set_bittiming(dev);
	if (err)
		return err;

	/* leave reset mode */
	set_normal_mode(dev);
	return 0;

exit:
	return err;
}

static int sunxican_set_mode(struct net_device *dev, enum can_mode mode)
{
	int err;

	switch (mode) {
	case CAN_MODE_START:
		err = sunxi_can_start(dev);
		if (err) {
			netdev_err(dev, "starting CAN controller failed!\n");
			return err;
		}
		if (netif_queue_stopped(dev))
			netif_wake_queue(dev);
		break;

	default:
		return -EOPNOTSUPP;
	}
	return 0;
}

/* transmit a CAN message
 * message layout in the sk_buff should be like this:
 * xx xx xx xx         ff         ll 00 11 22 33 44 55 66 77
 * [ can_id ] [flags] [len] [can data (up to 8 bytes]
 */
static int sunxican_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	struct can_frame *cf = (struct can_frame *)skb->data;
	u8 dlc;
	u32 dreg, msg_flag_n;
	canid_t id;
	int i;

	if (can_dropped_invalid_skb(dev, skb))
		return NETDEV_TX_OK;

	netif_stop_queue(dev);

	id = cf->can_id;
	dlc = cf->can_dlc;
	msg_flag_n = dlc;

	if (id & CAN_RTR_FLAG)
		msg_flag_n |= SUNXI_MSG_RTR_FLAG;

	if (id & CAN_EFF_FLAG) {
		msg_flag_n |= SUNXI_MSG_EFF_FLAG;
		dreg = SUNXI_REG_BUF5_ADDR;
		writel((id >> 21) & 0xFF, priv->base + SUNXI_REG_BUF1_ADDR);
		writel((id >> 13) & 0xFF, priv->base + SUNXI_REG_BUF2_ADDR);
		writel((id >> 5)  & 0xFF, priv->base + SUNXI_REG_BUF3_ADDR);
		writel((id << 3)  & 0xF8, priv->base + SUNXI_REG_BUF4_ADDR);
	} else {
		dreg = SUNXI_REG_BUF3_ADDR;
		writel((id >> 3) & 0xFF, priv->base + SUNXI_REG_BUF1_ADDR);
		writel((id << 5) & 0xE0, priv->base + SUNXI_REG_BUF2_ADDR);
	}

	for (i = 0; i < dlc; i++)
		writel(cf->data[i], priv->base + (dreg + i * 4));

	writel(msg_flag_n, priv->base + SUNXI_REG_BUF0_ADDR);

	can_put_echo_skb(skb, dev, 0);

	if (priv->can.ctrlmode & CAN_CTRLMODE_LOOPBACK)
		sunxi_can_write_cmdreg(priv, SUNXI_CMD_SELF_RCV_REQ);
	else
		sunxi_can_write_cmdreg(priv, SUNXI_CMD_TRANS_REQ);

	return NETDEV_TX_OK;
}

static void sunxi_can_rx(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	u8 fi;
	u32 dreg;
	canid_t id;
	int i;

	/* create zero'ed CAN frame buffer */
	skb = alloc_can_skb(dev, &cf);
	if (!skb)
		return;

	fi = readl(priv->base + SUNXI_REG_BUF0_ADDR);
	cf->can_dlc = get_can_dlc(fi & 0x0F);
	if (fi & SUNXI_MSG_EFF_FLAG) {
		dreg = SUNXI_REG_BUF5_ADDR;
		id = (readl(priv->base + SUNXI_REG_BUF1_ADDR) << 21) |
		     (readl(priv->base + SUNXI_REG_BUF2_ADDR) << 13) |
		     (readl(priv->base + SUNXI_REG_BUF3_ADDR) << 5)  |
		    ((readl(priv->base + SUNXI_REG_BUF4_ADDR) >> 3)  & 0x1f);
		id |= CAN_EFF_FLAG;
	} else {
		dreg = SUNXI_REG_BUF3_ADDR;
		id = (readl(priv->base + SUNXI_REG_BUF1_ADDR) << 3) |
		    ((readl(priv->base + SUNXI_REG_BUF2_ADDR) >> 5) & 0x7);
	}

	/* remote frame ? */
	if (fi & SUNXI_MSG_RTR_FLAG)
		id |= CAN_RTR_FLAG;
	else
		for (i = 0; i < cf->can_dlc; i++)
			cf->data[i] = readl(priv->base + dreg + i * 4);

	cf->can_id = id;

	sunxi_can_write_cmdreg(priv, SUNXI_CMD_RELEASE_RBUF);

	stats->rx_packets++;
	stats->rx_bytes += cf->can_dlc;
	netif_rx(skb);

	can_led_event(dev, CAN_LED_EVENT_RX);
}

static int sunxi_can_err(struct net_device *dev, u8 isrc, u8 status)
{
	struct sunxican_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	enum can_state state = priv->can.state;
	enum can_state rx_state, tx_state;
	unsigned int rxerr, txerr, errc;
	u32 ecc, alc;

	skb = alloc_can_err_skb(dev, &cf);
	if (!skb)
		return -ENOMEM;

	errc = readl(priv->base + SUNXI_REG_ERRC_ADDR);
	rxerr = (errc >> 16) & 0xFF;
	txerr = errc & 0xFF;

	cf->data[6] = txerr;
	cf->data[7] = rxerr;

	if (isrc & SUNXI_INT_DATA_OR) {
		/* data overrun interrupt */
		netdev_dbg(dev, "data overrun interrupt\n");
		cf->can_id |= CAN_ERR_CRTL;
		cf->data[1] = CAN_ERR_CRTL_RX_OVERFLOW;
		stats->rx_over_errors++;
		stats->rx_errors++;
		/* clear bit */
		sunxi_can_write_cmdreg(priv, SUNXI_CMD_CLEAR_OR_FLAG);
	}
	if (isrc & SUNXI_INT_ERR_WRN) {
		/* error warning interrupt */
		netdev_dbg(dev, "error warning interrupt\n");

		if (status & SUNXI_STA_BUS_OFF)
			state = CAN_STATE_BUS_OFF;
		else if (status & SUNXI_STA_ERR_STA)
			state = CAN_STATE_ERROR_WARNING;
		else
			state = CAN_STATE_ERROR_ACTIVE;
	}
	if (isrc & SUNXI_INT_BUS_ERR) {
		/* bus error interrupt */
		netdev_dbg(dev, "bus error interrupt\n");
		priv->can.can_stats.bus_error++;
		stats->rx_errors++;

		ecc = readl(priv->base + SUNXI_REG_STA_ADDR);

		cf->can_id |= CAN_ERR_PROT | CAN_ERR_BUSERROR;

		switch (ecc & SUNXI_STA_MASK_ERR) {
		case SUNXI_STA_BIT_ERR:
			cf->data[2] |= CAN_ERR_PROT_BIT;
			break;
		case SUNXI_STA_FORM_ERR:
			cf->data[2] |= CAN_ERR_PROT_FORM;
			break;
		case SUNXI_STA_STUFF_ERR:
			cf->data[2] |= CAN_ERR_PROT_STUFF;
			break;
		default:
			cf->data[2] |= CAN_ERR_PROT_UNSPEC;
			cf->data[3] = (ecc & SUNXI_STA_ERR_SEG_CODE) >> 16;
			break;
		}
		/* error occurred during transmission? */
		if ((ecc & SUNXI_STA_ERR_DIR) == 0)
			cf->data[2] |= CAN_ERR_PROT_TX;
	}
	if (isrc & SUNXI_INT_ERR_PASSIVE) {
		/* error passive interrupt */
		netdev_dbg(dev, "error passive interrupt\n");
		if (state == CAN_STATE_ERROR_PASSIVE)
			state = CAN_STATE_ERROR_WARNING;
		else
			state = CAN_STATE_ERROR_PASSIVE;
	}
	if (isrc & SUNXI_INT_ARB_LOST) {
		/* arbitration lost interrupt */
		netdev_dbg(dev, "arbitration lost interrupt\n");
		alc = readl(priv->base + SUNXI_REG_STA_ADDR);
		priv->can.can_stats.arbitration_lost++;
		stats->tx_errors++;
		cf->can_id |= CAN_ERR_LOSTARB;
		cf->data[0] = (alc & 0x1f) >> 8;
	}

	if (state != priv->can.state) {
		tx_state = txerr >= rxerr ? state : 0;
		rx_state = txerr <= rxerr ? state : 0;

		can_change_state(dev, cf, tx_state, rx_state);
		if (state == CAN_STATE_BUS_OFF)
			can_bus_off(dev);
	}

	stats->rx_packets++;
	stats->rx_bytes += cf->can_dlc;
	netif_rx(skb);

	return 0;
}

static irqreturn_t sunxi_can_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct sunxican_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	u8 isrc, status;
	int n = 0;

	while ((isrc = readl(priv->base + SUNXI_REG_INT_ADDR)) &&
	       (n < SUNXI_CAN_MAX_IRQ)) {
		n++;
		status = readl(priv->base + SUNXI_REG_STA_ADDR);

		if (isrc & SUNXI_INT_WAKEUP)
			netdev_warn(dev, "wakeup interrupt\n");

		if (isrc & SUNXI_INT_TBUF_VLD) {
			/* transmission complete interrupt */
			stats->tx_bytes +=
			    readl(priv->base +
				  SUNXI_REG_RBUF_RBACK_START_ADDR) & 0xf;
			stats->tx_packets++;
			can_get_echo_skb(dev, 0);
			netif_wake_queue(dev);
			can_led_event(dev, CAN_LED_EVENT_TX);
		}
		if (isrc & SUNXI_INT_RBUF_VLD) {
			/* receive interrupt */
			while (status & SUNXI_STA_RBUF_RDY) {
				/* RX buffer is not empty */
				sunxi_can_rx(dev);
				status = readl(priv->base + SUNXI_REG_STA_ADDR);
			}
		}
		if (isrc &
		    (SUNXI_INT_DATA_OR | SUNXI_INT_ERR_WRN | SUNXI_INT_BUS_ERR |
		     SUNXI_INT_ERR_PASSIVE | SUNXI_INT_ARB_LOST)) {
			/* error interrupt */
			if (sunxi_can_err(dev, isrc, status))
				break;
		}
		/* clear the interrupt */
		writel(isrc, priv->base + SUNXI_REG_INT_ADDR);
	}
	if (n >= SUNXI_CAN_MAX_IRQ)
		netdev_dbg(dev, "%d messages handled in ISR", n);

	return (n) ? IRQ_HANDLED : IRQ_NONE;
}

static int sunxican_open(struct net_device *dev)
{
	int err;

	/* common open */
	err = open_candev(dev);
	if (err)
		return err;

	/* register interrupt handler */
	err = request_irq(dev->irq, sunxi_can_interrupt, IRQF_SHARED,
			  dev->name, dev);
	if (err) {
		netdev_err(dev, "request_irq err: %d\n", err);
		close_candev(dev);
		return -EAGAIN;
	}

	sunxi_can_start(dev);

	can_led_event(dev, CAN_LED_EVENT_OPEN);

	netif_start_queue(dev);

	return 0;
}

static int sunxican_close(struct net_device *dev)
{
	struct sunxican_priv *priv = netdev_priv(dev);

	netif_stop_queue(dev);
	set_reset_mode(dev);
	clk_disable_unprepare(priv->clk);

	free_irq(dev->irq, dev);
	close_candev(dev);
	can_led_event(dev, CAN_LED_EVENT_STOP);

	return 0;
}

static const struct net_device_ops sunxican_netdev_ops = {
	.ndo_open = sunxican_open,
	.ndo_stop = sunxican_close,
	.ndo_start_xmit = sunxican_start_xmit,
};

static const struct of_device_id sunxican_of_match[] = {
	{.compatible = "allwinner,sunxican"},
	{},
};

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
	struct net_device *dev;
	struct sunxican_priv *priv;

	clk = clk_get(&pdev->dev, "apb1_can");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "no clock defined\n");
		err = -ENODEV;
		goto exit;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "could not get a valid irq\n");
		err = -ENODEV;
		goto exit;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
		dev_err(&pdev->dev, "could not get io memory resource\n");
		err = -EBUSY;
		goto exit;
	}

	addr = ioremap_nocache(res->start, resource_size(res));
	if (!addr) {
		dev_err(&pdev->dev, "could not map io memory\n");
		err = -ENOMEM;
		goto exit_release;
	}

	dev = alloc_candev(sizeof(struct sunxican_priv), 1);
	if (!dev) {
		dev_err(&pdev->dev,
			"could not allocate memory for CAN device\n");
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
				       CAN_CTRLMODE_LISTENONLY |
				       CAN_CTRLMODE_LOOPBACK |
				       CAN_CTRLMODE_3_SAMPLES;
	priv->base = addr;
	priv->clk = clk;
	spin_lock_init(&priv->cmdreg_lock);

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
exit:
	return err;
}

static int __maybe_unused sunxi_can_suspend(struct device *device)
{
	struct net_device *dev = dev_get_drvdata(device);
	struct sunxican_priv *priv = netdev_priv(dev);
	u32 mode;
	int err;

	if (netif_running(dev)) {
		netif_stop_queue(dev);
		netif_device_detach(dev);
	}

	err = set_reset_mode(dev);
	if (err)
		return err;

	mode = readl(priv->base + SUNXI_REG_MSEL_ADDR) | SUNXI_SLEEP_MODE;
	writel(mode, priv->base + SUNXI_REG_MSEL_ADDR);

	priv->can.state = CAN_STATE_SLEEPING;

	clk_disable(priv->clk);
	return 0;
}

static int __maybe_unused sunxi_can_resume(struct device *device)
{
	struct net_device *dev = dev_get_drvdata(device);
	struct sunxican_priv *priv = netdev_priv(dev);
	u32 mode;
	int err;

	err = clk_enable(priv->clk);
	if (err) {
	netdev_err(dev, "clk_enable() failed, error %d\n", err);
		return err;
	}

	err = set_reset_mode(dev);
	if (err)
		return err;

	mode = readl(priv->base + SUNXI_REG_MSEL_ADDR) & ~(SUNXI_SLEEP_MODE);
	writel(mode, priv->base + SUNXI_REG_MSEL_ADDR);
	err = set_normal_mode(dev);
	if (err)
		return err;

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
		.pm = &sunxi_can_pm_ops,
		.of_match_table = sunxican_of_match,
	},
	.probe = sunxican_probe,
	.remove = sunxican_remove,
	.id_table = sunxican_id_table,
};

module_platform_driver(sunxi_can_driver);

MODULE_AUTHOR("Peter Chen <xingkongcp@gmail.com>");
MODULE_AUTHOR("Gerhard Bertelsmann <info@gerhard-bertelsmann.de>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRV_NAME "CAN driver for Allwinner SoCs (A10/A20)");
MODULE_VERSION(DRV_MODULE_VERSION);
