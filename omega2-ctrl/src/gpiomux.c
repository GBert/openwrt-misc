#include "gpiomux.h"

static uint8_t *gpio_mmap_reg = NULL;
static int gpio_mmap_fd = 0;

void __gpiomux_set(unsigned int mask, unsigned int shift, unsigned int val) {
    volatile uint32_t *reg = (volatile uint32_t *)(gpio_mmap_reg + 0x60);
    unsigned int v;

    if (shift > 31) {
	shift -= 32;
	reg++;
    }

    v = *reg;
    v &= ~(mask << shift);
    v |= (val << shift);
    *(volatile uint32_t *)(reg) = v;
}

int gpiomux_set(char *group, char *name) {
    int i;
    int id;

    for (id = 0; id < _O2_NUM_GPIO_MUX; id++)
	if (!strcmp(omega2GpioMux[id].name, group))
	    break;

    if (id < _O2_NUM_GPIO_MUX)
	for (i = 0; i < 4; i++) {
	    if (!omega2GpioMux[id].func[i] || strcmp(omega2GpioMux[id].func[i], name))
		continue;
	    __gpiomux_set(omega2GpioMux[id].mask, omega2GpioMux[id].shift, i);
	    fprintf(stderr, "set gpiomux %s -> %s\n", omega2GpioMux[id].name, name);
	    return EXIT_SUCCESS;
	}
    fprintf(stderr, "unknown group/function combination\n");
    return EXIT_FAILURE;
}

int gpiomux_get(void) {
    unsigned int reg = *(volatile uint32_t *)(gpio_mmap_reg + 0x60);
    unsigned int reg2 = *(volatile uint32_t *)(gpio_mmap_reg + 0x64);
    int id;

    for (id = 0; id < _O2_NUM_GPIO_MUX; id++) {
	unsigned int val;
	int i;

	if (omega2GpioMux[id].shift < 32)
	    val = (reg >> omega2GpioMux[id].shift) & omega2GpioMux[id].mask;
	else
	    val = (reg2 >> (omega2GpioMux[id].shift - 32)) & omega2GpioMux[id].mask;

	fprintf(stderr, "Group %s - ", omega2GpioMux[id].name);
	for (i = 0; i < 4; i++) {
	    if (!omega2GpioMux[id].func[i])
		continue;
	    if (i == val)
		fprintf(stderr, "[%s] ", omega2GpioMux[id].func[i]);
	    else
		fprintf(stderr, "%s ", omega2GpioMux[id].func[i]);
	}
	fprintf(stderr, "\n");
    }

    return EXIT_SUCCESS;
}

int gpiomux_mmap_open(void) {
    if ((gpio_mmap_fd = open(MMAP_PATH, O_RDWR)) < 0) {
	fprintf(stderr, "unable to open mmap file");
	return EXIT_FAILURE;
    }

    gpio_mmap_reg = (uint8_t *) mmap(NULL, 1024, PROT_READ | PROT_WRITE,
				     MAP_FILE | MAP_SHARED, gpio_mmap_fd, 0x10000000);
    if (gpio_mmap_reg == MAP_FAILED) {
	perror("failed to mmap");
	fprintf(stderr, "failed to mmap");
	gpio_mmap_reg = NULL;
	close(gpio_mmap_fd);
	return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void gpiomux_mmap_close(void) {
    close(gpio_mmap_fd);
}
