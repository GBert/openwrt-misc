#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/mman.h>

#define CAN_BASE_ADDR	0x01C2BC00
#define CAN_MAP_LEN	0x200

static int can_mem = -1;
static int *can_map = NULL;

void print_byte_rows(int *ptr, int rows) {
    int i, j;
    int *my_ptr_row, *my_ptr;

    my_ptr_row = ptr;

    for (i = 0; i < rows; i++) {
	my_ptr = my_ptr_row;
	for (j = 0; j < 8; j++) {
	    printf(" %02x", *my_ptr++ & 0xff);
	}
	my_ptr = my_ptr_row;
	for (j = 0; j < 8; j++) {
	    if (isprint(my_ptr))
		printf("%c", *my_ptr);
	    else
		putchar('.');
	    my_ptr++;
	}
    }
    my_ptr_row = my_ptr;
}


int sunxi_can_open(const char *device) {
    /* open /dev/mem */
    can_mem = open(device, O_RDWR | O_SYNC);
    if (can_mem < 0) {
        printf("%s: warning: open failed [%s]\n", __func__, strerror(errno));
        can_mem = -1;
        return EXIT_FAILURE;
    }

    /* Memory map GPIO */
    can_map = mmap(NULL, CAN_MAP_LEN, PROT_READ, MAP_SHARED, can_mem, CAN_BASE_ADDR);
    if (can_map == MAP_FAILED) {
        printf("%s: warning: mmap failed [%s]\n", __func__, strerror(errno));
        close(can_mem);
        can_mem = -1;
        return EXIT_FAILURE;
    }

    return can_mem;
}

void sunxi_can_close(void) {
    if (can_map) {
        if (munmap(can_map, CAN_MAP_LEN)) {
            printf("%s: warning: munmap failed\n", __func__);
        }
        can_map = NULL;
    }
    if (can_mem >= 0) {
        if (close(can_mem)) {
            printf("%s: warning: close failed\n", __func__);
        }
        can_mem = -1;
    }
}

int main(int argc, char **argv) {
    int ret;

    ret = sunxi_can_open("/dev/mem");
    if (ret == EXIT_FAILURE)
	return EXIT_FAILURE;
    print_byte_rows(can_map + 0x40, 8);
    sunxi_can_close();
}
