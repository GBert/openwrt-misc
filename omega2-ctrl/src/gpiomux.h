#ifndef _GPIOMUX_H_
#define _GPIOMUX_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "omega2_ctrl_types.h"

int gpiomux_set(char *group, char *name);
int gpiomux_get(void);
int gpiomux_mmap_open(void);
void gpiomux_mmap_close(void);

static struct gpiomux {
    char *name;
    char *func[4];
    unsigned int shift;
    unsigned int mask;
} omega2GpioMux[] = {
    {
	.name = "i2c",.func = { "i2c", "gpio", NULL, NULL},.shift = 20,.mask = 0x3,}, {
	.name = "uart0",.func = { "uart", "gpio", NULL, NULL},.shift = 8,.mask = 0x3,}, {
	.name = "uart1",.func = { "uart", "gpio", NULL, NULL},.shift = 24,.mask = 0x3,}, {
	.name = "uart2",.func = { "uart", "gpio", "pwm", NULL},.shift = 26,.mask = 0x3,}, {
	.name = "pwm0",.func = { "pwm", "gpio", NULL, NULL},.shift = 28,.mask = 0x3,}, {
	.name = "pwm1",.func = { "pwm", "gpio", NULL, NULL},.shift = 30,.mask = 0x3,}, {
	.name = "refclk",.func = { "refclk", "gpio", NULL, NULL},.shift = 18,.mask = 0x1,}, {
	.name = "spi_s",.func = { "spi_s", "gpio", NULL, NULL},.shift = 2,.mask = 0x3,}, {
	.name = "spi_cs1",.func = { "spi_cs1", "gpio", NULL, "refclk"},.shift = 4,.mask = 0x3,}, {
	.name = "i2s",.func = { "i2s", "gpio", "pcm", NULL},.shift = 6,.mask = 0x3,}, {
	.name = "ephy",.func = { "ephy", "gpio", NULL, NULL},.shift = 34,.mask = 0x3,}, {
	.name = "wled",.func = { "wled", "gpio", NULL, NULL},.shift = 32,.mask = 0x3,}
};

#endif /* _GPIOMUX_H_ */
