#ifndef _REFCLK_H_
#define _REFCLK_H_

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

int refclk_set(unsigned int rate);
int refclk_get(void);

int refclk_mmap_open(void);
void refclk_mmap_close(void);

#endif // _REFCLK_H_
