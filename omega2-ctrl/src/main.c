#include "main.h"

void usage(char *progname) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s gpiomux get\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "Functionality:\n");
    fprintf(stderr, "\tList the current GPIO muxing configuration\n");
    fprintf(stderr, "\n");

    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s gpiomux set <GPIO group> <mux setting>\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "Functionality:\n");
    fprintf(stderr, "\tSet the GPIO muxing for the specified GPIO signal group\n");
    fprintf(stderr, "\n");

    fprintf(stderr, "\n");

    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s refclk get\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "Functionality:\n");
    fprintf(stderr, "\tDisplay the current refclk setting\n");
    fprintf(stderr, "\n");

    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s refclk set <frequency (MHz)>\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "Functionality:\n");
    fprintf(stderr, "\tSet the refclk to the specified frequency if possible\n");
    fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
    int status = EXIT_FAILURE;

    // read the command argument
    if (argc >= 2) {
	if (strcmp(argv[1], "gpiomux") == 0) {
	    if (gpiomux_mmap_open() == EXIT_FAILURE) {
		return EXIT_FAILURE;
	    }

	    if (argc >= 5 && !strcmp(argv[2], "set")) {
		status = gpiomux_set(argv[3], argv[4]);
	    } else if (argc >= 3 && !strcmp(argv[2], "get")) {
		status = gpiomux_get();
	    } else {
		usage(*argv);
	    }

	    gpiomux_mmap_close();
	} else if (strcmp(argv[1], "refclk") == 0) {
	    if (refclk_mmap_open() == EXIT_FAILURE) {
		return EXIT_FAILURE;
	    }

	    if (argc >= 4 && !strcmp(argv[2], "set")) {
		status = refclk_set(atoi(argv[3]));
	    } else if (argc >= 3 && !strcmp(argv[2], "get")) {
		status = refclk_get();
	    } else {
		usage(*argv);
	    }

	    refclk_mmap_close();
	} else {
	    usage(*argv);
	}
    } else {
	usage(*argv);
    }

    return status;
}
