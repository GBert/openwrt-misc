#ifndef _OMEGA2_CTRL_TYPES_H_
#define _OMEGA2_CTRL_TYPES_H_

#define MMAP_PATH			"/dev/mem"

////////////////
// REFCLK
////////////////
typedef enum e_Omega2Refclk {
    O2_REFCLK_XTAL = 0,
    O2_REFCLK_12,
    O2_REFCLK_25,
    O2_REFCLK_40,
    O2_REFCLK_48,
    _O2_NUM_REFCLK
} eOmega2Refclk;

enum {
    CLK_XTAL = 0,
    CLK_12,
    CLK_25,
    CLK_40,
    CLK_48,
    __CLK_MAX,
};

////////////////
// GPIO MUX
////////////////

typedef enum e_Omega2GpioMux {
    O2_GPIO_MUX_I2C = 0,
    O2_GPIO_MUX_UART0,
    O2_GPIO_MUX_UART1,
    O2_GPIO_MUX_UART2,
    O2_GPIO_MUX_PWM0,
    O2_GPIO_MUX_PWM1,
    O2_GPIO_MUX_REFCLK,
    O2_GPIO_MUX_SPI_S,
    O2_GPIO_MUX_SPI_CS1,
    O2_GPIO_MUX_I2S,
    O2_GPIO_MUX_EPHY,
    O2_GPIO_MUX_WLED,
    _O2_NUM_GPIO_MUX
} eOmega2GpioMux;

#endif // _OMEGA2_CTRL_TYPES_H_
