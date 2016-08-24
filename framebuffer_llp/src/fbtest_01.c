#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

int main(void) {
    int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    char *fbp = 0;

    /* ppen the file for reading and writing */
    fbfd = open("/dev/fb0", O_RDWR);
    if (!fbfd) {
	printf("Error: cannot open framebuffer device.\n");
	exit(EXIT_FAILURE);
    }
    printf("The framebuffer device was opened successfully.\n");

    /* Get fixed screen information */
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
	printf("Error reading fixed information.\n");
    }
    /* Get variable screen information */
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
	printf("Error reading variable information.\n");
    }
    printf("%dx%d, %d bpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    /* map framebuffer to user memory */
    screensize = finfo.smem_len;

    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

    if ((void *)fbp == MAP_FAILED) {
	printf("Failed to mmap.\n");
	exit(EXIT_FAILURE);

    } else {
	/* draw...
	   just fill upper half of the screen with something */
	memset(fbp, 0xff, screensize / 2);
	/* and lower half with something else */
	memset(fbp + screensize / 2, 0x18, screensize / 2);
    }

    /* cleanup */
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}
