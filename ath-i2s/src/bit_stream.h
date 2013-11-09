#ifndef BIT_STREAM_H
#define BIT_STREAM_H
#include <linux/ioctl.h>
 
typedef struct
{
    int frequency;
} bit_serial_arg_t;
 
#define BIT_STREAM_GET_FREQ _IOR('q', 1, bit_stream_arg_t *)
#define BIT_STREAM_SET_FREQ _IOW('q', 2, bit_stream_arg_t *)
 
#endif
