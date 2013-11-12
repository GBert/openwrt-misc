/*
 *  Atheros AR934X HS UART defines
 *
 *  Copyright (C) 2013 Gerhard Bertelsmann <info@gerhard-bertelsmann.de>
 *  losely copied from ar934x_uart.h from Gabor Juhos
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#ifndef __AR934X_HS_UART_H
#define __AR934X_HS_UART_H

struct ar934x_uart_platform_data {
	unsigned        uartclk;
};

#define AR71XX_APB_BASE				0x18000000
#define AR934X_UART1_BASE        		(AR71XX_APB_BASE + 0x00500000)
#define AR934X_UART1_SIZE        		0x14
#define AR934X_HS_UART_BASE        		AR934X_UART1_BASE

#define AR934X_HS_UART_REGS_SIZE		20
#define AR934X_HS_UART_FIFO_SIZE		4

#define AR934X_HS_UART_DATA_REG			0x00
#define AR934X_HS_UART_CS_REG			0x04
#define AR934X_HS_UART_CLOCK_REG		0x08
#define AR934X_HS_UART_INT_REG			0x0c
#define AR934X_HS_UART_INT_EN_REG		0x10

#define AR934X_HS_UART_DATA_TX_RX_MASK		0xff
#define AR934X_HS_UART_DATA_RX_CSR		BIT(8)
#define AR934X_HS_UART_DATA_TX_CSR		BIT(9)

#define AR934X_HS_UART_CS_PARITY_S		0
#define AR934X_HS_UART_CS_PARITY_M		0x3
#define	  AR934X_HS_UART_CS_PARITY_NONE		0
#define	  AR934X_HS_UART_CS_PARITY_ODD		1
#define	  AR934X_HS_UART_CS_PARITY_EVEN		2
#define AR934X_HS_UART_CS_IF_MODE_S		2
#define AR934X_HS_UART_CS_IF_MODE_M		0x3
#define	  AR934X_HS_UART_CS_IF_MODE_NONE	0
#define	  AR934X_HS_UART_CS_IF_MODE_DTE		1
#define	  AR934X_HS_UART_CS_IF_MODE_DCE		2
#define AR934X_HS_UART_CS_FLOW_CTRL_S		4
#define AR934X_HS_UART_CS_FLOW_CTRL_M		0x3
/* #define AR934X_HS_UART_CS_DMA_EN		BIT(6) */
#define AR934X_HS_UART_CS_TX_READY_ORIDE	BIT(7)
#define AR934X_HS_UART_CS_RX_READY_ORIDE	BIT(8)
#define AR934X_HS_UART_CS_TX_READY		BIT(9)
#define AR934X_HS_UART_CS_RX_BREAK		BIT(10)
#define AR934X_HS_UART_CS_TX_BREAK		BIT(11)
#define AR934X_HS_UART_CS_HOST_INT		BIT(12)
#define AR934X_HS_UART_CS_HOST_INT_EN		BIT(13)
#define AR934X_HS_UART_CS_TX_BUSY		BIT(14)
#define AR934X_HS_UART_CS_RX_BUSY		BIT(15)

#define AR934X_HS_UART_CLOCK_STEP_M		0xffff
#define AR934X_HS_UART_CLOCK_SCALE_M		0xfff
#define AR934X_HS_UART_CLOCK_SCALE_S		16
#define AR934X_HS_UART_CLOCK_STEP_M		0xffff

#define AR934X_HS_UART_INT_RX_VALID		BIT(0)
#define AR934X_HS_UART_INT_TX_READY		BIT(1)
#define AR934X_HS_UART_INT_RX_FRAMING_ERR	BIT(2)
#define AR934X_HS_UART_INT_RX_OFLOW_ERR		BIT(3)
#define AR934X_HS_UART_INT_TX_OFLOW_ERR		BIT(4)
#define AR934X_HS_UART_INT_RX_PARITY_ERR	BIT(5)
#define AR934X_HS_UART_INT_RX_BREAK_ON		BIT(6)
#define AR934X_HS_UART_INT_RX_BREAK_OFF		BIT(7)
#define AR934X_HS_UART_INT_RX_FULL		BIT(8)
#define AR934X_HS_UART_INT_TX_EMPTY		BIT(9)
#define AR934X_HS_UART_INT_ALLINTS		0x3ff

#endif /* __AR934X_HS_UART_H */
