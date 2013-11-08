#ifndef BIT_SERIAL_H
#define BIT_SERIAL_H
#include <linux/ioctl.h>
 
typedef struct
{
    int frequency;
} bit_serial_arg_t;
 
#define BIT_SERIAL_GET_FREQ _IOR('q', 1, bit_serial_arg_t *)
#define BIT_SERIAL_SET_FREQ _IOW('q', 2, bit_serial_arg_t *)
 
#endif
