/*
* sun7i_can.h - CAN bus controller driver for sun7i
*
* Copyright (c) 2013 Peter Chen
*
* Copyright (c) 2013 Inmotion Co., Ltd.
* All right reserved.
*
*/

#ifndef SUN7I_CAN_H
#define SUN7I_CAN_H

#include <linux/irqreturn.h>
#include <linux/can/dev.h>

#define SUN7I_CAN_ECHO_SKB_MAX        1 /* the SUN7I CAN has one TX buffer object */

/* Registers' address */
/* #define CAN_BASE0		0xF1C2BC00 */
static void __iomem *CAN_BASE0 = (void *)0x01C2BC00;
#define CAN_MSEL_ADDR		(CAN_BASE0 + 0x0000)         //Can Mode Select Register
#define CAN_CMD_ADDR		(CAN_BASE0 + 0x0004)         //Can Command Register
#define CAN_STA_ADDR		(CAN_BASE0 + 0x0008)         //Can Status Register
#define CAN_INT_ADDR		(CAN_BASE0 + 0x000c)         //Can Interrupt Flag Register
#define CAN_INTEN_ADDR		(CAN_BASE0 + 0x0010)         //Can Interrupt Enable Register
#define CAN_BTIME_ADDR		(CAN_BASE0 + 0x0014)         //Can Bus Timing 0 Register
#define CAN_TEWL_ADDR		(CAN_BASE0 + 0x0018)         //Can Tx Error Warning Limit Register
#define CAN_ERRC_ADDR		(CAN_BASE0 + 0x001c)         //Can Error Counter Register
#define CAN_RMCNT_ADDR		(CAN_BASE0 + 0x0020)         //Can Receive Message Counter Register
#define CAN_RBUFSA_ADDR		(CAN_BASE0 + 0x0024)         //Can Receive Buffer Start Address Register
#define CAN_BUF0_ADDR		(CAN_BASE0 + 0x0040)         //Can Tx/Rx Buffer 0 Register
#define CAN_BUF1_ADDR		(CAN_BASE0 + 0x0044)         //Can Tx/Rx Buffer 1 Register
#define CAN_BUF2_ADDR		(CAN_BASE0 + 0x0048)         //Can Tx/Rx Buffer 2 Register
#define CAN_BUF3_ADDR		(CAN_BASE0 + 0x004c)         //Can Tx/Rx Buffer 3 Register
#define CAN_BUF4_ADDR		(CAN_BASE0 + 0x0050)         //Can Tx/Rx Buffer 4 Register
#define CAN_BUF5_ADDR		(CAN_BASE0 + 0x0054)         //Can Tx/Rx Buffer 5 Register
#define CAN_BUF6_ADDR		(CAN_BASE0 + 0x0058)         //Can Tx/Rx Buffer 6 Register
#define CAN_BUF7_ADDR		(CAN_BASE0 + 0x005c)         //Can Tx/Rx Buffer 7 Register
#define CAN_BUF8_ADDR		(CAN_BASE0 + 0x0060)         //Can Tx/Rx Buffer 8 Register
#define CAN_BUF9_ADDR		(CAN_BASE0 + 0x0064)         //Can Tx/Rx Buffer 9 Register
#define CAN_BUF10_ADDR		(CAN_BASE0 + 0x0068)         //Can Tx/Rx Buffer 10 Register
#define CAN_BUF11_ADDR		(CAN_BASE0 + 0x006c)         //Can Tx/Rx Buffer 11 Register
#define CAN_BUF12_ADDR		(CAN_BASE0 + 0x0070)         //Can Tx/Rx Buffer 12 Register
#define CAN_ACPC_ADDR		(CAN_BASE0 + 0x0040)         //Can Acceptance Code 0 Register
#define CAN_ACPM_ADDR		(CAN_BASE0 + 0x0044)         //Can Acceptance Mask 0 Register
#define CAN_RBUF_RBACK_START_ADDR	(CAN_BASE0 + 0x0180)        //CAN transmit buffer for read back register
#define CAN_RBUF_RBACK_END_ADDR		(CAN_BASE0 + 0x01b0)        //CAN transmit buffer for read back register

/* Controller Register Description */

/* mode select register (r/w)
* offset:0x0000 default:0x0000_0001 */
#define SLEEP_MODE		(1<<4)		//This bit can only be written in Reset Mode
#define WAKE_UP			(0<<4)		//This bit can only be written in Reset Mode
#define SINGLE_FILTER		(1<<3)
#define DUAL_FILTERS		(0<<3)
#define LOOPBACK_MODE		(1<<2)
#define LISTEN_ONLY_MODE	(1<<1)
#define RESET_MODE		(1<<0)
/* command register (w)
* offset:0x0004 default:0x0000_0000 */
#define BUS_OFF_REQ		(1<<5)
#define SELF_RCV_REQ		(1<<4)
#define CLEAR_DOVERRUN		(1<<3)
#define RELEASE_RBUF		(1<<2)
#define ABORT_REQ		(1<<1)
#define TRANS_REQ		(1<<0)
/* status register (r)
* offset:0x0008 default:0x0000_003c */
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
#define BUS_OFF			(1<<7)
#define ERR_STA			(1<<6)
#define TRANS_BUSY		(1<<5)
#define RCV_BUSY		(1<<4)
#define TRANS_OVER		(1<<3)
#define TBUF_RDY		(1<<2)
#define DATA_ORUN		(1<<1)
#define RBUF_RDY		(1<<0)
/* interrupt register (r)
* offset:0x000c default:0x0000_0000 */
#define BUS_ERR			(1<<7)        //write 1 to clear
#define ARB_LOST		(1<<6)        //write 1 to clear
#define ERR_PASSIVE		(1<<5)        //write 1 to clear
#define WAKEUP			(1<<4)        //write 1 to clear
#define DATA_ORUNI		(1<<3)        //write 1 to clear
#define ERR_WRN			(1<<2)        //write 1 to clear
#define TBUF_VLD		(1<<1)        //write 1 to clear
#define RBUF_VLD		(1<<0)        //write 1 to clear
/* interrupt enable register (r/w)
* offset:0x0010 default:0x0000_0000 */
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
/* timing register (r/w)
* offset:0x0014 default:0x0000_0000 */
//...

/* output control */
#define NOR_OMODE		(2)
#define CLK_OMODE		(3)
/* arbitration lost flag*/

/* error code */
#define ERR_INRCV		(1<<5)
#define ERR_INTRANS		(0<<5)

/* filter mode */
#define FILTER_CLOSE		0
#define SINGLE_FLTER_MODE	1
#define DUAL_FILTER_MODE	2

/*
* Flags for sun7icanpriv.flags
*/
#define SUN7I_CAN_CUSTOM_IRQ_HANDLER 0x1

#define SW_INT_IRQNO_CAN	26
#define CLK_MOD_CAN "can"
#define SUN7I_CAN_MAX_IRQ	20        /* max. number of interrupts handled in ISR */

/*
* sun7i_can private data structure
*/
struct sun7i_can_priv {
        struct can_priv can;        /* must be the first member */
        int open_time;
        struct sk_buff *echo_skb;

        void *priv;                /* for board-specific data */
        struct net_device *dev;

        unsigned long irq_flags; /* for request_irq() */
        spinlock_t cmdreg_lock; /* lock for concurrent cmd register writes */

        u16 flags;                /* custom mode flags */
};

#endif
