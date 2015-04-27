/*
 * CAN driver for Microchips MCP2515DM-BM board
 *
 * Copyright (C) 2015 Gerhard Bertelsmann (info@gerhard-bertelsmann.de)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.
 *
 * This driver is inspired by the usb8_dev and mcp251x driver
 *
 */

#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/usb.h>

#include <linux/can.h>
#include <linux/can/dev.h>
#include <linux/can/error.h>
/* #include <linux/can/led.h> */

/* driver constants */
#define MAX_RX_URBS			20
#define MAX_TX_URBS			20
#define RX_BUFFER_SIZE			64

/* vendor and product id */
#define MCP2515DM_BM_VENDOR_ID		0x04d8
#define MCP2515DM_BM_PRODUCT_ID		0x0070

/* endpoints */
enum mcp2515dm_bm_endpoint {
	MCP2515DM_BM_ENDP_RX = 1,
	MCP2515DM_BM_ENDP_TX,
};

/* device CAN clock */
#define MCP2515DM_BM_CLOCK		20000000

/* setup flags */
#define MCP2515DM_BM_SILENT			0x01
#define MCP2515DM_BM_LOOPBACK			0x02
#define MCP2515DM_BM_DISABLE_AUTO_RESTRANS	0x04
#define MCP2515DM_BM_STATUS_FRAME		0x08

/* commands */
enum mcp2515dm_bm_cmd {
	MCP2515DM_BM_RESET = 1,
	MCP2515DM_BM_OPEN,
	MCP2515DM_BM_CLOSE,
	MCP2515DM_BM_SET_SPEED,
	MCP2515DM_BM_SET_MASK_FILTER,
	MCP2515DM_BM_GET_STATUS,
	MCP2515DM_BM_GET_STATISTICS,
	MCP2515DM_BM_GET_SERIAL,
	MCP2515DM_BM_GET_SOFTW_VER,
	MCP2515DM_BM_GET_HARDW_VER,
	MCP2515DM_BM_RESET_TIMESTAMP,
	MCP2515DM_BM_GET_SOFTW_HARDW_VER
};

/* command options */
#define MCP2515DM_BM_BAUD_MANUAL	0x09
#define MCP2515DM_BM_CMD_START		0x11
#define MCP2515DM_BM_CMD_END		0x22

#define MCP2515DM_BM_CMD_SUCCESS	0
#define MCP2515DM_BM_CMD_ERROR		255

#define MCP2515DM_BM_CMD_TIMEOUT	1000

/* frames */
#define MCP2515DM_BM_DATA_START		0x55
#define MCP2515DM_BM_DATA_END		0xAA

#define MCP2515DM_BM_TYPE_CAN_FRAME	0
#define MCP2515DM_BM_TYPE_ERROR_FRAME	3

#define MCP2515DM_BM_EXTID		0x01
#define MCP2515DM_BM_RTR		0x02
#define MCP2515DM_BM_ERR_FLAG		0x04

/* status */
#define MCP2515DM_BM_STATUSMSG_OK	0x00  /* Normal condition. */
#define MCP2515DM_BM_STATUSMSG_OVERRUN	0x01  /* Overrun occured when sending */
#define MCP2515DM_BM_STATUSMSG_BUSLIGHT	0x02  /* Error counter has reached 96 */
#define MCP2515DM_BM_STATUSMSG_BUSHEAVY	0x03  /* Error count. has reached 128 */
#define MCP2515DM_BM_STATUSMSG_BUSOFF	0x04  /* Device is in BUSOFF */
#define MCP2515DM_BM_STATUSMSG_STUFF	0x20  /* Stuff Error */
#define MCP2515DM_BM_STATUSMSG_FORM	0x21  /* Form Error */
#define MCP2515DM_BM_STATUSMSG_ACK	0x23  /* Ack Error */
#define MCP2515DM_BM_STATUSMSG_BIT0	0x24  /* Bit1 Error */
#define MCP2515DM_BM_STATUSMSG_BIT1	0x25  /* Bit0 Error */
#define MCP2515DM_BM_STATUSMSG_CRC	0x27  /* CRC Error */

#define MCP2515DM_BM_RP_MASK		0x7F  /* Mask for Receive Error Bit */

/* SPI interface instruction set */
#define INSTRUCTION_WRITE       0x02
#define INSTRUCTION_READ        0x03
#define INSTRUCTION_BIT_MODIFY  0x05
#define INSTRUCTION_LOAD_TXB(n) (0x40 + 2 * (n))
#define INSTRUCTION_READ_RXB(n) (((n) == 0) ? 0x90 : 0x94)
#define INSTRUCTION_RESET       0xC0
#define RTS_TXB0                0x01
#define RTS_TXB1                0x02
#define RTS_TXB2                0x04
#define INSTRUCTION_RTS(n)      (0x80 | ((n) & 0x07))

/* MPC2515 registers */
#define CANSTAT       0x0e
#define CANCTRL       0x0f
#  define CANCTRL_REQOP_MASK        0xe0
#  define CANCTRL_REQOP_CONF        0x80
#  define CANCTRL_REQOP_LISTEN_ONLY 0x60
#  define CANCTRL_REQOP_LOOPBACK    0x40
#  define CANCTRL_REQOP_SLEEP       0x20
#  define CANCTRL_REQOP_NORMAL      0x00
#  define CANCTRL_OSM               0x08
#  define CANCTRL_ABAT              0x10
#define TEC           0x1c
#define REC           0x1d
#define CNF1          0x2a
#  define CNF1_SJW_SHIFT   6
#define CNF2          0x29
#  define CNF2_BTLMODE     0x80
#  define CNF2_SAM         0x40
#  define CNF2_PS1_SHIFT   3
#define CNF3          0x28
#  define CNF3_SOF         0x08
#  define CNF3_WAKFIL      0x04
#  define CNF3_PHSEG2_MASK 0x07
#define CANINTE       0x2b
#  define CANINTE_MERRE 0x80
#  define CANINTE_WAKIE 0x40
#  define CANINTE_ERRIE 0x20
#  define CANINTE_TX2IE 0x10
#  define CANINTE_TX1IE 0x08
#  define CANINTE_TX0IE 0x04
#  define CANINTE_RX1IE 0x02
#  define CANINTE_RX0IE 0x01
#define CANINTF       0x2c
#  define CANINTF_MERRF 0x80
#  define CANINTF_WAKIF 0x40
#  define CANINTF_ERRIF 0x20
#  define CANINTF_TX2IF 0x10
#  define CANINTF_TX1IF 0x08
#  define CANINTF_TX0IF 0x04
#  define CANINTF_RX1IF 0x02
#  define CANINTF_RX0IF 0x01
#  define CANINTF_RX (CANINTF_RX0IF | CANINTF_RX1IF)
#  define CANINTF_TX (CANINTF_TX2IF | CANINTF_TX1IF | CANINTF_TX0IF)
#  define CANINTF_ERR (CANINTF_ERRIF)
#define EFLG          0x2d
#  define EFLG_EWARN    0x01
#  define EFLG_RXWAR    0x02
#  define EFLG_TXWAR    0x04
#  define EFLG_RXEP     0x08
#  define EFLG_TXEP     0x10
#  define EFLG_TXBO     0x20
#  define EFLG_RX0OVR   0x40
#  define EFLG_RX1OVR   0x80
#define TXBCTRL(n)  (((n) * 0x10) + 0x30 + TXBCTRL_OFF)
#  define TXBCTRL_ABTF  0x40
#  define TXBCTRL_MLOA  0x20
#  define TXBCTRL_TXERR 0x10
#  define TXBCTRL_TXREQ 0x08
#define TXBSIDH(n)  (((n) * 0x10) + 0x30 + TXBSIDH_OFF)
#  define SIDH_SHIFT    3
#define TXBSIDL(n)  (((n) * 0x10) + 0x30 + TXBSIDL_OFF)
#  define SIDL_SID_MASK    7
#  define SIDL_SID_SHIFT   5
#  define SIDL_EXIDE_SHIFT 3
#  define SIDL_EID_SHIFT   16
#  define SIDL_EID_MASK    3
#define TXBEID8(n)  (((n) * 0x10) + 0x30 + TXBEID8_OFF)
#define TXBEID0(n)  (((n) * 0x10) + 0x30 + TXBEID0_OFF)
#define TXBDLC(n)   (((n) * 0x10) + 0x30 + TXBDLC_OFF)
#  define DLC_RTR_SHIFT    6
#define TXBCTRL_OFF 0
#define TXBSIDH_OFF 1
#define TXBSIDL_OFF 2
#define TXBEID8_OFF 3
#define TXBEID0_OFF 4
#define TXBDLC_OFF  5
#define TXBDAT_OFF  6
#define RXBCTRL(n)  (((n) * 0x10) + 0x60 + RXBCTRL_OFF)
#  define RXBCTRL_BUKT  0x04
#  define RXBCTRL_RXM0  0x20
#  define RXBCTRL_RXM1  0x40
#define RXBSIDH(n)  (((n) * 0x10) + 0x60 + RXBSIDH_OFF)
#  define RXBSIDH_SHIFT 3
#define RXBSIDL(n)  (((n) * 0x10) + 0x60 + RXBSIDL_OFF)
#  define RXBSIDL_IDE   0x08
#  define RXBSIDL_SRR   0x10
#  define RXBSIDL_EID   3
#  define RXBSIDL_SHIFT 5
#define RXBEID8(n)  (((n) * 0x10) + 0x60 + RXBEID8_OFF)
#define RXBEID0(n)  (((n) * 0x10) + 0x60 + RXBEID0_OFF)
#define RXBDLC(n)   (((n) * 0x10) + 0x60 + RXBDLC_OFF)
#  define RXBDLC_LEN_MASK  0x0f
#  define RXBDLC_RTR       0x40
#define RXBCTRL_OFF 0
#define RXBSIDH_OFF 1
#define RXBSIDL_OFF 2
#define RXBEID8_OFF 3
#define RXBEID0_OFF 4
#define RXBDLC_OFF  5
#define RXBDAT_OFF  6
#define RXFSIDH(n) ((n) * 4)
#define RXFSIDL(n) ((n) * 4 + 1)
#define RXFEID8(n) ((n) * 4 + 2)
#define RXFEID0(n) ((n) * 4 + 3)
#define RXMSIDH(n) ((n) * 4 + 0x20)
#define RXMSIDL(n) ((n) * 4 + 0x21)
#define RXMEID8(n) ((n) * 4 + 0x22)
#define RXMEID0(n) ((n) * 4 + 0x23)

/* MCP2515DM-BM specific */
#define PERCENT_0   0x00
#define PERCENT_25  0x01
#define PERCENT_50  0x02
#define PERCENT_75  0x03
#define PERCENT_100 0x04

/* table of devices that work with this driver */
static const struct usb_device_id mcp2515dm_bm_table[] = {
	{USB_DEVICE(MCP2515DM_BM_VENDOR_ID, MCP2515DM_BM_PRODUCT_ID)},
	{}			/* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, mcp2515dm_bm_table);

struct mcp2515dm_bm_tx_urb_context {
	struct mcp2515dm_bm_priv *priv;

	u32 echo_index;
	u8 dlc;
};

/* Structure to hold all of our device specific stuff */
struct mcp2515dm_bm_priv {
	struct can_priv can;	/* must be the first member */

	struct sk_buff *echo_skb[MAX_TX_URBS];

	struct usb_device *udev;
	struct net_device *netdev;

	atomic_t active_tx_urbs;
	struct usb_anchor tx_submitted;
	struct mcp2515dm_bm_tx_urb_context tx_contexts[MAX_TX_URBS];

	struct usb_anchor rx_submitted;

	struct can_berr_counter bec;

	u8 *usb_msg_buffer;

	struct mutex mcp2515dm_bm_cmd_lock;

};

/* MCP2515DM-BM CAN message format */
struct __packed can_msg {
	u8 sidh;
	u8 sidl;
	u8 eid8;
	u8 eid0;
	u8 dlc;
	u8 data[8];
};

/* MCP2515DM-BM USB message format */
struct __packed usb_msg {
	struct can_msg msg[4];
	u8 num_can_msg;
	u8 canintf;
	u8 eflg;
	u8 tx_err_count;
	u8 rx_err_count;
	u8 canstat;
	u8 cantrl;
	u8 status;
	u8 spi_cmd;
	u8 reg;
	u8 data;
	u8 reserved;
};

/* tx frame */
struct __packed mcp2515dm_bm_tx_msg {
	u8 begin;
	u8 flags;		/* RTR and EXT_ID flag */
	__be32 id;		/* upper 3 bits not used */
	u8 dlc;			/* data length code 0-8 bytes */
	u8 data[8];		/* 64-bit data */
	u8 end;
};

/* rx frame */
struct __packed mcp2515dm_bm_rx_msg {
	u8 begin;
	u8 type;		/* frame type */
	u8 flags;		/* RTR and EXT_ID flag */
	__be32 id;		/* upper 3 bits not used */
	u8 dlc;			/* data length code 0-8 bytes */
	u8 data[8];		/* 64-bit data */
	__be32 timestamp;	/* 32-bit timestamp */
	u8 end;
};

/* command frame */
struct __packed mcp2515dm_bm_cmd_msg {
	u8 begin;
	u8 channel;		/* unkown - always 0 */
	u8 command;		/* command to execute */
	u8 opt1;		/* optional parameter / return value */
	u8 opt2;		/* optional parameter 2 */
	u8 data[10];		/* optional parameter and data */
	u8 end;
};

static int mcp2515dm_bm_send_cmd_msg(struct mcp2515dm_bm_priv *priv, u8 *msg, int size)
{
	int actual_length;

	return usb_interrupt_msg(priv->udev,
			    usb_sndintpipe(priv->udev, MCP2515DM_BM_ENDP_TX),
			    msg, size, &actual_length, MCP2515DM_BM_CMD_TIMEOUT);
}

static int mcp2515dm_bm_wait_cmd_msg(struct mcp2515dm_bm_priv *priv, u8 *msg, int size,
				int *actual_length)
{
	return usb_interrupt_msg(priv->udev,
			    usb_rcvintpipe(priv->udev, MCP2515DM_BM_ENDP_RX),
			    msg, size, actual_length, MCP2515DM_BM_CMD_TIMEOUT);
}

/* Send command to device and receive result.
 * Command was successful when opt1 = 0.
 */
static int mcp2515dm_bm_send_cmd(struct mcp2515dm_bm_priv *priv,
				 struct mcp2515dm_bm_cmd_msg *out,
				 struct mcp2515dm_bm_cmd_msg *in)
{
	int err;
	int num_bytes_read;
	struct net_device *netdev;

	netdev = priv->netdev;

	out->begin = MCP2515DM_BM_CMD_START;
	out->end = MCP2515DM_BM_CMD_END;

	mutex_lock(&priv->mcp2515dm_bm_cmd_lock);

	memcpy(priv->usb_msg_buffer, out, sizeof(struct usb_msg));

	err = mcp2515dm_bm_send_cmd_msg(priv, priv->usb_msg_buffer,
					sizeof(struct usb_msg));
	if (err < 0) {
		netdev_err(netdev, "sending command message failed\n");
		goto failed;
	}

	err = mcp2515dm_bm_wait_cmd_msg(priv, priv->usb_msg_buffer,
					sizeof(struct usb_msg),
					&num_bytes_read);
	if (err < 0) {
		netdev_err(netdev, "no command message answer\n");
		goto failed;
	}

	memcpy(in, priv->usb_msg_buffer, sizeof(struct usb_msg));

	if (in->begin != MCP2515DM_BM_CMD_START
	    || in->end != MCP2515DM_BM_CMD_END || num_bytes_read != 16
	    || in->opt1 != 0)
		err = -EPROTO;

failed:
	mutex_unlock(&priv->mcp2515dm_bm_cmd_lock);
	return err;
}

/* Send open command to device */
static int mcp2515dm_bm_cmd_open(struct mcp2515dm_bm_priv *priv)
{
	struct can_bittiming *bt = &priv->can.bittiming;
	struct mcp2515dm_bm_cmd_msg outmsg;
	struct mcp2515dm_bm_cmd_msg inmsg;
	u32 ctrlmode = priv->can.ctrlmode;
	u32 flags = MCP2515DM_BM_STATUS_FRAME;
	__be32 beflags;
	__be16 bebrp;

	memset(&outmsg, 0, sizeof(outmsg));
	outmsg.command = MCP2515DM_BM_OPEN;
	outmsg.opt1 = MCP2515DM_BM_BAUD_MANUAL;
	outmsg.data[0] = bt->prop_seg + bt->phase_seg1;
	outmsg.data[1] = bt->phase_seg2;
	outmsg.data[2] = bt->sjw;

	/* BRP */
	bebrp = cpu_to_be16((u16) bt->brp);
	memcpy(&outmsg.data[3], &bebrp, sizeof(bebrp));

	/* flags */
	if (ctrlmode & CAN_CTRLMODE_LOOPBACK)
		flags |= MCP2515DM_BM_LOOPBACK;
	if (ctrlmode & CAN_CTRLMODE_LISTENONLY)
		flags |= MCP2515DM_BM_SILENT;
	if (ctrlmode & CAN_CTRLMODE_ONE_SHOT)
		flags |= MCP2515DM_BM_DISABLE_AUTO_RESTRANS;

	beflags = cpu_to_be32(flags);
	memcpy(&outmsg.data[5], &beflags, sizeof(beflags));

	return mcp2515dm_bm_send_cmd(priv, &outmsg, &inmsg);
}

/* Send close command to device */
static int mcp2515dm_bm_cmd_close(struct mcp2515dm_bm_priv *priv)
{
	struct mcp2515dm_bm_cmd_msg inmsg;
	struct mcp2515dm_bm_cmd_msg outmsg = {
		.channel = 0,
		.command = MCP2515DM_BM_CLOSE,
		.opt1 = 0,
		.opt2 = 0
	};

	return mcp2515dm_bm_send_cmd(priv, &outmsg, &inmsg);
}

/* Get firmware and hardware version */
static int mcp2515dm_bm_cmd_version(struct mcp2515dm_bm_priv *priv, u32 * res)
{
	struct mcp2515dm_bm_cmd_msg inmsg;
	struct mcp2515dm_bm_cmd_msg outmsg = {
		.channel = 0,
		.command = MCP2515DM_BM_GET_SOFTW_HARDW_VER,
		.opt1 = 0,
		.opt2 = 0
	};

	int err = mcp2515dm_bm_send_cmd(priv, &outmsg, &inmsg);
	if (err)
		return err;

	*res = be32_to_cpup((__be32 *) inmsg.data);

	return err;
}

/* Set network device mode
 *
 * Maybe we should leave this function empty, because the device
 * set mode variable with open command.
 */
static int mcp2515dm_bm_set_mode(struct net_device *netdev, enum can_mode mode)
{
	struct mcp2515dm_bm_priv *priv = netdev_priv(netdev);
	int err = 0;

	switch (mode) {
	case CAN_MODE_START:
		err = mcp2515dm_bm_cmd_open(priv);
		if (err)
			netdev_warn(netdev, "couldn't start device");
		break;

	default:
		return -EOPNOTSUPP;
	}

	return err;
}

/* Read error/status frames */
static void mcp2515dm_bm_rx_err_msg(struct mcp2515dm_bm_priv *priv,
				struct mcp2515dm_bm_rx_msg *msg)
{
	struct can_frame *cf;
	struct sk_buff *skb;
	struct net_device_stats *stats = &priv->netdev->stats;

	/* TODO */
	/* Error message:
	 * byte 0: Status
	 * byte 1: bit   7: Receive Passive
	 * byte 1: bit 0-6: Receive Error Counter
	 * byte 2: Transmit Error Counter
	 * byte 3: Always 0 (maybe reserved for future use)
	 */

	u8 state = msg->data[0];
	u8 rxerr = msg->data[1] & MCP2515DM_BM_RP_MASK;
	u8 txerr = msg->data[2];
	int rx_errors = 0;
	int tx_errors = 0;

	skb = alloc_can_err_skb(priv->netdev, &cf);
	if (!skb)
		return;

	switch (state) {
	case MCP2515DM_BM_STATUSMSG_OK:
		priv->can.state = CAN_STATE_ERROR_ACTIVE;
		cf->can_id |= CAN_ERR_PROT;
		cf->data[2] = CAN_ERR_PROT_ACTIVE;
		break;
	case MCP2515DM_BM_STATUSMSG_BUSOFF:
		priv->can.state = CAN_STATE_BUS_OFF;
		cf->can_id |= CAN_ERR_BUSOFF;
		priv->can.can_stats.bus_off++;
		can_bus_off(priv->netdev);
		break;
	case MCP2515DM_BM_STATUSMSG_OVERRUN:
	case MCP2515DM_BM_STATUSMSG_BUSLIGHT:
	case MCP2515DM_BM_STATUSMSG_BUSHEAVY:
		cf->can_id |= CAN_ERR_CRTL;
		break;
	default:
		priv->can.state = CAN_STATE_ERROR_WARNING;
		cf->can_id |= CAN_ERR_PROT | CAN_ERR_BUSERROR;
		priv->can.can_stats.bus_error++;
		break;
	}

	switch (state) {
	case MCP2515DM_BM_STATUSMSG_OK:
	case MCP2515DM_BM_STATUSMSG_BUSOFF:
		break;
	case MCP2515DM_BM_STATUSMSG_ACK:
		cf->can_id |= CAN_ERR_ACK;
		tx_errors = 1;
		break;
	case MCP2515DM_BM_STATUSMSG_CRC:
		cf->data[2] |= CAN_ERR_PROT_UNSPEC;
		cf->data[3] |= CAN_ERR_PROT_LOC_CRC_SEQ |
		    CAN_ERR_PROT_LOC_CRC_DEL;
		rx_errors = 1;
		break;
	case MCP2515DM_BM_STATUSMSG_BIT0:
		cf->data[2] |= CAN_ERR_PROT_BIT0;
		tx_errors = 1;
		break;
	case MCP2515DM_BM_STATUSMSG_BIT1:
		cf->data[2] |= CAN_ERR_PROT_BIT1;
		tx_errors = 1;
		break;
	case MCP2515DM_BM_STATUSMSG_FORM:
		cf->data[2] |= CAN_ERR_PROT_FORM;
		rx_errors = 1;
		break;
	case MCP2515DM_BM_STATUSMSG_STUFF:
		cf->data[2] |= CAN_ERR_PROT_STUFF;
		rx_errors = 1;
		break;
	case MCP2515DM_BM_STATUSMSG_OVERRUN:
		cf->data[1] = CAN_ERR_CRTL_RX_OVERFLOW;
		stats->rx_over_errors++;
		rx_errors = 1;
		break;
	case MCP2515DM_BM_STATUSMSG_BUSLIGHT:
		priv->can.state = CAN_STATE_ERROR_WARNING;
		cf->data[1] = (txerr > rxerr) ?
		    CAN_ERR_CRTL_TX_WARNING : CAN_ERR_CRTL_RX_WARNING;
		priv->can.can_stats.error_warning++;
		break;
	case MCP2515DM_BM_STATUSMSG_BUSHEAVY:
		priv->can.state = CAN_STATE_ERROR_PASSIVE;
		cf->data[1] = (txerr > rxerr) ?
		    CAN_ERR_CRTL_TX_PASSIVE : CAN_ERR_CRTL_RX_PASSIVE;
		priv->can.can_stats.error_passive++;
		break;
	default:
		netdev_warn(priv->netdev,
			    "Unknown status/error message (%d)\n", state);
		break;
	}

	if (tx_errors) {
		cf->data[2] |= CAN_ERR_PROT_TX;
		stats->tx_errors++;
	}

	if (rx_errors)
		stats->rx_errors++;

	cf->data[6] = txerr;
	cf->data[7] = rxerr;

	priv->bec.txerr = txerr;
	priv->bec.rxerr = rxerr;

	netif_rx(skb);

	stats->rx_packets++;
	stats->rx_bytes += cf->can_dlc;
}

/* Read data and status frames */
static void mcp2515dm_bm_rx_can_msg(struct mcp2515dm_bm_priv *priv,
				    struct mcp2515dm_bm_rx_msg *msg)
{
	struct can_frame *cf;
	struct sk_buff *skb;
	struct net_device_stats *stats = &priv->netdev->stats;

	if (msg->type == MCP2515DM_BM_TYPE_ERROR_FRAME &&
	    msg->flags == MCP2515DM_BM_ERR_FLAG) {
		mcp2515dm_bm_rx_err_msg(priv, msg);
	} else if (msg->type == MCP2515DM_BM_TYPE_CAN_FRAME) {
		skb = alloc_can_skb(priv->netdev, &cf);
		if (!skb)
			return;

		cf->can_id = be32_to_cpu(msg->id);
		cf->can_dlc = get_can_dlc(msg->dlc & 0xF);

		if (msg->flags & MCP2515DM_BM_EXTID)
			cf->can_id |= CAN_EFF_FLAG;

		if (msg->flags & MCP2515DM_BM_RTR)
			cf->can_id |= CAN_RTR_FLAG;
		else
			memcpy(cf->data, msg->data, cf->can_dlc);

		netif_rx(skb);

		stats->rx_packets++;
		stats->rx_bytes += cf->can_dlc;

		/* can_led_event(priv->netdev, CAN_LED_EVENT_RX); */
	} else {
		netdev_warn(priv->netdev, "frame type %d unknown", msg->type);
	}

}

/* Callback for reading data from device
 *
 * Check urb status, call read function and resubmit urb read operation.
 */
static void mcp2515dm_bm_read_int_callback(struct urb *urb)
{
	struct mcp2515dm_bm_priv *priv = urb->context;
	struct net_device *netdev;
	int retval;
	int pos = 0;

	netdev = priv->netdev;

	if (!netif_device_present(netdev))
		return;

	switch (urb->status) {
	case 0:		/* success */
		break;

	case -ENOENT:
	case -ESHUTDOWN:
		return;

	default:
		netdev_info(netdev, "Rx URB aborted (%d)\n", urb->status);
		goto resubmit_urb;
	}

	while (pos < urb->actual_length) {
		struct mcp2515dm_bm_rx_msg *msg;

		if (pos + sizeof(struct mcp2515dm_bm_rx_msg) > urb->actual_length) {
			netdev_err(priv->netdev, "format error\n");
			break;
		}

		msg = (struct mcp2515dm_bm_rx_msg *)(urb->transfer_buffer + pos);
		mcp2515dm_bm_rx_can_msg(priv, msg);

		pos += sizeof(struct mcp2515dm_bm_rx_msg);
	}

resubmit_urb:
	usb_fill_int_urb(urb, priv->udev,
			  usb_rcvintpipe(priv->udev, MCP2515DM_BM_ENDP_RX),
			  urb->transfer_buffer, RX_BUFFER_SIZE,
			  mcp2515dm_bm_read_int_callback, priv, 0);

	retval = usb_submit_urb(urb, GFP_ATOMIC);

	if (retval == -ENODEV)
		netif_device_detach(netdev);
	else if (retval)
		netdev_err(netdev,
			   "failed resubmitting read int urb: %d\n", retval);
}

/* Callback handler for write operations
 *
 * Free allocated buffers, check transmit status and
 * calculate statistic.
 */
static void mcp2515dm_bm_write_int_callback(struct urb *urb)
{
	struct mcp2515dm_bm_tx_urb_context *context = urb->context;
	struct mcp2515dm_bm_priv *priv;
	struct net_device *netdev;

	BUG_ON(!context);

	priv = context->priv;
	netdev = priv->netdev;

	/* free up our allocated buffer */
	usb_free_coherent(urb->dev, urb->transfer_buffer_length,
			  urb->transfer_buffer, urb->transfer_dma);

	atomic_dec(&priv->active_tx_urbs);

	if (!netif_device_present(netdev))
		return;

	if (urb->status)
		netdev_info(netdev, "Tx URB aborted (%d)\n", urb->status);

	netdev->stats.tx_packets++;
	netdev->stats.tx_bytes += context->dlc;

	can_get_echo_skb(netdev, context->echo_index);

	/* can_led_event(netdev, CAN_LED_EVENT_TX); */

	/* Release context */
	context->echo_index = MAX_TX_URBS;

	netif_wake_queue(netdev);
}

/* Send data to device */
static netdev_tx_t mcp2515dm_bm_start_xmit(struct sk_buff *skb,
					   struct net_device *netdev)
{
	struct mcp2515dm_bm_priv *priv = netdev_priv(netdev);
	struct net_device_stats *stats = &netdev->stats;
	struct can_frame *cf = (struct can_frame *)skb->data;
	struct mcp2515dm_bm_tx_msg *msg;
	struct urb *urb;
	struct mcp2515dm_bm_tx_urb_context *context = NULL;
	u8 *buf;
	int i, err;
	size_t size = sizeof(struct mcp2515dm_bm_tx_msg);

	if (can_dropped_invalid_skb(netdev, skb))
		return NETDEV_TX_OK;

	/* create a URB, and a buffer for it, and copy the data to the URB */
	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb) {
		netdev_err(netdev, "No memory left for URBs\n");
		goto nomem;
	}

	buf = usb_alloc_coherent(priv->udev, size, GFP_ATOMIC,
				 &urb->transfer_dma);
	if (!buf) {
		netdev_err(netdev, "No memory left for USB buffer\n");
		goto nomembuf;
	}

	memset(buf, 0, size);

	msg = (struct mcp2515dm_bm_tx_msg *)buf;
	msg->begin = MCP2515DM_BM_DATA_START;
	msg->flags = 0x00;

	if (cf->can_id & CAN_RTR_FLAG)
		msg->flags |= MCP2515DM_BM_RTR;

	if (cf->can_id & CAN_EFF_FLAG)
		msg->flags |= MCP2515DM_BM_EXTID;

	msg->id = cpu_to_be32(cf->can_id & CAN_ERR_MASK);
	msg->dlc = cf->can_dlc;
	memcpy(msg->data, cf->data, cf->can_dlc);
	msg->end = MCP2515DM_BM_DATA_END;

	for (i = 0; i < MAX_TX_URBS; i++) {
		if (priv->tx_contexts[i].echo_index == MAX_TX_URBS) {
			context = &priv->tx_contexts[i];
			break;
		}
	}

	/* May never happen! When this happens we'd more URBs in flight as
	 * allowed (MAX_TX_URBS).
	 */
	if (!context)
		goto nofreecontext;

	context->priv = priv;
	context->echo_index = i;
	context->dlc = cf->can_dlc;

	usb_fill_int_urb(urb, priv->udev,
			  usb_sndintpipe(priv->udev, MCP2515DM_BM_ENDP_TX),
			  buf, size, mcp2515dm_bm_write_int_callback, context, 0);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	usb_anchor_urb(urb, &priv->tx_submitted);

	can_put_echo_skb(skb, netdev, context->echo_index);

	atomic_inc(&priv->active_tx_urbs);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (unlikely(err))
		goto failed;
	else if (atomic_read(&priv->active_tx_urbs) >= MAX_TX_URBS)
		/* Slow down tx path */
		netif_stop_queue(netdev);

	/* Release our reference to this URB, the USB core will eventually free
	 * it entirely.
	 */
	usb_free_urb(urb);

	return NETDEV_TX_OK;

nofreecontext:
	usb_free_coherent(priv->udev, size, buf, urb->transfer_dma);
	usb_free_urb(urb);

	netdev_warn(netdev, "couldn't find free context");

	return NETDEV_TX_BUSY;

failed:
	can_free_echo_skb(netdev, context->echo_index);

	usb_unanchor_urb(urb);
	usb_free_coherent(priv->udev, size, buf, urb->transfer_dma);

	atomic_dec(&priv->active_tx_urbs);

	if (err == -ENODEV)
		netif_device_detach(netdev);
	else
		netdev_warn(netdev, "failed tx_urb %d\n", err);

nomembuf:
	usb_free_urb(urb);

nomem:
	dev_kfree_skb(skb);
	stats->tx_dropped++;

	return NETDEV_TX_OK;
}

static int mcp2515dm_bm_get_berr_counter(const struct net_device *netdev,
					 struct can_berr_counter *bec)
{
	struct mcp2515dm_bm_priv *priv = netdev_priv(netdev);

	bec->txerr = priv->bec.txerr;
	bec->rxerr = priv->bec.rxerr;

	return 0;
}

/* Start USB device */
static int mcp2515dm_bm_start(struct mcp2515dm_bm_priv *priv)
{
	struct net_device *netdev = priv->netdev;
	int err, i;

	for (i = 0; i < MAX_RX_URBS; i++) {
		struct urb *urb = NULL;
		u8 *buf;

		/* create a URB, and a buffer for it */
		urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!urb) {
			netdev_err(netdev, "No memory left for URBs\n");
			err = -ENOMEM;
			break;
		}

		buf = usb_alloc_coherent(priv->udev, RX_BUFFER_SIZE, GFP_KERNEL,
					 &urb->transfer_dma);
		if (!buf) {
			netdev_err(netdev, "No memory left for USB buffer\n");
			usb_free_urb(urb);
			err = -ENOMEM;
			break;
		}

		usb_fill_int_urb(urb, priv->udev,
				  usb_rcvintpipe(priv->udev,
						  MCP2515DM_BM_ENDP_RX),
				  buf, RX_BUFFER_SIZE,
				  mcp2515dm_bm_read_int_callback, priv, 0);
		urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
		usb_anchor_urb(urb, &priv->rx_submitted);

		err = usb_submit_urb(urb, GFP_KERNEL);
		if (err) {
			usb_unanchor_urb(urb);
			usb_free_coherent(priv->udev, RX_BUFFER_SIZE, buf,
					  urb->transfer_dma);
			usb_free_urb(urb);
			break;
		}

		/* Drop reference, USB core will take care of freeing it */
		usb_free_urb(urb);
	}

	/* Did we submit any URBs */
	if (i == 0) {
		netdev_warn(netdev, "couldn't setup read URBs\n");
		return err;
	}

	/* Warn if we've couldn't transmit all the URBs */
	if (i < MAX_RX_URBS)
		netdev_warn(netdev, "rx performance may be slow\n");

	err = mcp2515dm_bm_cmd_open(priv);
	if (err)
		goto failed;

	priv->can.state = CAN_STATE_ERROR_ACTIVE;

	return 0;

failed:
	if (err == -ENODEV)
		netif_device_detach(priv->netdev);

	netdev_warn(netdev, "couldn't submit control: %d\n", err);

	return err;
}

/* Open USB device */
static int mcp2515dm_bm_open(struct net_device *netdev)
{
	struct mcp2515dm_bm_priv *priv = netdev_priv(netdev);
	int err;

	/* common open */
	err = open_candev(netdev);
	if (err)
		return err;

	/* can_led_event(netdev, CAN_LED_EVENT_OPEN); */

	/* finally start device */
	err = mcp2515dm_bm_start(priv);
	if (err) {
		if (err == -ENODEV)
			netif_device_detach(priv->netdev);

		netdev_warn(netdev, "couldn't start device: %d\n", err);

		close_candev(netdev);

		return err;
	}

	netif_start_queue(netdev);

	return 0;
}

static void unlink_all_urbs(struct mcp2515dm_bm_priv *priv)
{
	int i;

	usb_kill_anchored_urbs(&priv->rx_submitted);

	usb_kill_anchored_urbs(&priv->tx_submitted);
	atomic_set(&priv->active_tx_urbs, 0);

	for (i = 0; i < MAX_TX_URBS; i++)
		priv->tx_contexts[i].echo_index = MAX_TX_URBS;
}

/* Close USB device */
static int mcp2515dm_bm_close(struct net_device *netdev)
{
	struct mcp2515dm_bm_priv *priv = netdev_priv(netdev);
	int err = 0;

	/* Send CLOSE command to CAN controller */
	err = mcp2515dm_bm_cmd_close(priv);
	if (err)
		netdev_warn(netdev, "couldn't stop device");

	priv->can.state = CAN_STATE_STOPPED;

	netif_stop_queue(netdev);

	/* Stop polling */
	unlink_all_urbs(priv);

	close_candev(netdev);

	/* can_led_event(netdev, CAN_LED_EVENT_STOP); */

	return err;
}

static const struct net_device_ops mcp2515dm_bm_netdev_ops = {
	.ndo_open = mcp2515dm_bm_open,
	.ndo_stop = mcp2515dm_bm_close,
	.ndo_start_xmit = mcp2515dm_bm_start_xmit,
	/* .ndo_change_mtu = can_change_mtu, */
};

static const struct can_bittiming_const mcp2515dm_bm_bittiming_const = {
	.name = "mcp2515dm_bm",
	.tseg1_min = 3,
	.tseg1_max = 16,
	.tseg2_min = 2,
	.tseg2_max = 8,
	.sjw_max = 4,
	.brp_min = 1,
	.brp_max = 64,
	.brp_inc = 1,
};

/* Probe USB device
 *
 * Check device and firmware.
 * Set supported modes and bittiming constants.
 * Allocate some memory.
 */
static int mcp2515dm_bm_probe(struct usb_interface *intf,
			      const struct usb_device_id *id)
{
	struct net_device *netdev;
	struct mcp2515dm_bm_priv *priv;
	int i, err = -ENOMEM;
	u32 version;
	char buf[18];
	struct usb_device *usbdev = interface_to_usbdev(intf);

	/* product id looks strange, better we also check iProduct string */
	if (usb_string(usbdev, usbdev->descriptor.iProduct, buf,
		       sizeof(buf)) > 0 && strcmp(buf, "MCP2515 BUS MONITOR")) {
		dev_info(&usbdev->dev, "ignoring: not an MCP2515DM_BM board\n");
		return -ENODEV;
	}

	netdev = alloc_candev(sizeof(struct mcp2515dm_bm_priv), MAX_TX_URBS);
	if (!netdev) {
		dev_err(&intf->dev, "Couldn't alloc candev\n");
		return -ENOMEM;
	}

	priv = netdev_priv(netdev);

	priv->udev = usbdev;
	priv->netdev = netdev;

	priv->can.state = CAN_STATE_STOPPED;
	priv->can.clock.freq = MCP2515DM_BM_CLOCK;
	priv->can.bittiming_const = &mcp2515dm_bm_bittiming_const;
	priv->can.do_set_mode = mcp2515dm_bm_set_mode;
	priv->can.do_get_berr_counter = mcp2515dm_bm_get_berr_counter;
	priv->can.ctrlmode_supported = CAN_CTRLMODE_LOOPBACK |
	    CAN_CTRLMODE_LISTENONLY | CAN_CTRLMODE_ONE_SHOT;

	netdev->netdev_ops = &mcp2515dm_bm_netdev_ops;

	netdev->flags |= IFF_ECHO;	/* we support local echo */

	init_usb_anchor(&priv->rx_submitted);

	init_usb_anchor(&priv->tx_submitted);
	atomic_set(&priv->active_tx_urbs, 0);

	for (i = 0; i < MAX_TX_URBS; i++)
		priv->tx_contexts[i].echo_index = MAX_TX_URBS;

	/* TODO */
	/* priv->cmd_msg_buffer = kzalloc(sizeof(struct mcp2515dm_bm_cmd_msg),
				       GFP_KERNEL); */
	priv->usb_msg_buffer = kzalloc(sizeof(struct usb_msg), GFP_KERNEL);

	if (!priv->usb_msg_buffer)
		goto cleanup_candev;

	usb_set_intfdata(intf, priv);

	SET_NETDEV_DEV(netdev, &intf->dev);

	mutex_init(&priv->mcp2515dm_bm_cmd_lock);

	err = register_candev(netdev);
	if (err) {
		netdev_err(netdev, "couldn't register CAN device: %d\n", err);
		goto cleanup_cmd_msg_buffer;
	}

	err = mcp2515dm_bm_cmd_version(priv, &version);
	if (err) {
		netdev_err(netdev, "can't get firmware version\n");
		goto cleanup_unregister_candev;
	} else {
		netdev_info(netdev,
			    "firmware: %d.%d, hardware: %d.%d\n",
			    (version >> 24) & 0xff, (version >> 16) & 0xff,
			    (version >> 8) & 0xff, version & 0xff);
	}

	/* devm_can_led_init(netdev); */

	return 0;

cleanup_unregister_candev:
	unregister_netdev(priv->netdev);

cleanup_cmd_msg_buffer:
	kfree(priv->usb_msg_buffer);

cleanup_candev:
	free_candev(netdev);

	return err;

}

/* Called by the usb core when driver is unloaded or device is removed */
static void mcp2515dm_bm_disconnect(struct usb_interface *intf)
{
	struct mcp2515dm_bm_priv *priv = usb_get_intfdata(intf);

	usb_set_intfdata(intf, NULL);

	if (priv) {
		netdev_info(priv->netdev, "device disconnected\n");

		unregister_netdev(priv->netdev);
		free_candev(priv->netdev);

		unlink_all_urbs(priv);
	}

}

static struct usb_driver mcp2515dm_bm_driver = {
	.name =		"mcp2515dm_bm",
	.probe =	mcp2515dm_bm_probe,
	.disconnect =	mcp2515dm_bm_disconnect,
	.id_table =	mcp2515dm_bm_table,
};

module_usb_driver(mcp2515dm_bm_driver);

MODULE_AUTHOR("Gerhard Bertelsmann <info@gerhard-bertelsmann.de>");
MODULE_DESCRIPTION
    ("CAN driver for Microchips MCP2515 CAN Bus Monitor Demo Board");
MODULE_LICENSE("GPL v2");
