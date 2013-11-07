/*
 *  Bit serialization device driver header
 *
 *  Copyright (C) 2013 Gerhard Bertelsmann
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#ifndef BIT_SERIALIZATION_H
#define BIT_SERIALIZATION_H

#define BIT_SERIALIZATION_DEVICE_NAME     ("bit-serial")

struct bit_serialization_platform_data
{
    void *data;
    int  (*open) (void *data);
    int  (*release) (void *data);
};

#endif /* BIT_SERIALIZATION_H */

