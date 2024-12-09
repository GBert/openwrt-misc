/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <math.h>
#include <pthread.h>
#include <libgen.h>
#include "snmp.h"
#include "sml_server.h"

#include <sml/sml_file.h>
#include <sml/sml_transport.h>

#define SML_BUFFER_LEN	8096
#define SNMP_PORT	161
#define MAX_STRING_LEN	32
#define MAXLINE		128

pthread_mutex_t value_mutex = PTHREAD_MUTEX_INITIALIZER;

extern void *snmp_agent(void *);
const char obis_tarif0[] = { 0x01, 0x00, 0x01, 0x08, 0x00 };
const char obis_tarif1[] = { 0x01, 0x00, 0x01, 0x08, 0x01 };
const char obis_tarif2[] = { 0x01, 0x00, 0x01, 0x08, 0x02 };
const char obis_deliver0[] = { 0x01, 0x00, 0x02, 0x08, 0x00 };
const char power_meter[] = { 0x01, 0x00, 0x10, 0x07, 0x00 };

unsigned int counter_tarif0;
unsigned int counter_tarif1;
unsigned int counter_tarif2;
unsigned int counter_deliver0;
unsigned int pmeter;
FILE *log_file_ptr = NULL;

int verbose = 0;

void print_usage(char *prg) {
    fprintf(stderr, "\nUsage: %s -p <snmp_port> -i <interface>\n", prg);
    fprintf(stderr, "   Version 1.3\n\n");
    fprintf(stderr, "         -p <port>           SNMP port - default 161\n");
    fprintf(stderr, "         -i <interface>      serial interface - default /dev/ttyUSB0\n");
    fprintf(stderr, "         -l <log file>       raw serial log file\n");
    fprintf(stderr, "         -f                  running in foreground\n\n");
}

void print_hex(unsigned char c) {
    unsigned char hex = c;
    hex = (hex >> 4) & 0x0f;
    if (hex > 9) {
	hex = hex + 'A' - 10;
    } else {
	hex = hex + '0';
    }
    putchar(hex);
    hex = c & 0x0f;
    if (hex > 9) {
	hex = hex + 'A' - 10;
    } else {
	hex = hex + '0';
    }
    putchar(hex);
    putchar(0x20);
}

void print_octet_str(octet_string * object) {
    int i;
    for (i = 0; i < object->len; i++) {
	print_hex(object->str[i]);
    }
}

void print_char_str(char *object, int len) {
    int i;
    for (i = 0; i < len; i++) {
	print_hex(object[i]);
    }
}

int serial_port_open(const char *device) {
    int bits;
    struct termios config;
    memset(&config, 0, sizeof(config));

    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
	printf("error: open(%s): %s\n", device, strerror(errno));
	return -1;
    }
    // set RTS
    ioctl(fd, TIOCMGET, &bits);
    bits |= TIOCM_RTS;
    ioctl(fd, TIOCMSET, &bits);

    tcgetattr(fd, &config);

    // set 8-N-1
    config.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    config.c_oflag &= ~OPOST;
    config.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    config.c_cflag &= ~(CSIZE | PARENB | PARODD | CSTOPB);
    config.c_cflag |= CS8;

    // set speed to 9600 baud
    cfsetispeed(&config, B9600);
    cfsetospeed(&config, B9600);

    tcsetattr(fd, TCSANOW, &config);
    return fd;
}

void transport_receiver(unsigned char *buffer, size_t buffer_len) {
    short i;
    //unsigned char message_buffer[SML_BUFFER_LEN];
    sml_get_list_response *body;
    sml_list *entry = NULL;
    int m = 0, n = 10;
    struct timeval time;

    // the buffer contains the whole message, with transport escape sequences.
    // these escape sequences are stripped here.
    sml_file *file = sml_file_parse(buffer + 8, buffer_len - 16);

    if (log_file_ptr)
	fwrite(buffer, 1, buffer_len, log_file_ptr);
    // the sml file is parsed now

    // read here some values ..
    for (i = 0; i < file->messages_len; i++) {
	sml_message *message = file->messages[i];

	if (*message->message_body->tag == SML_MESSAGE_GET_LIST_RESPONSE) {
	    body = (sml_get_list_response *) message->message_body->data;
	    // printf("new message from: %*s\n", body->server_id->len, body->server_id->str);
	    entry = body->val_list;

	    int time_mode = 1;
	    if (body->act_sensor_time) {
		time.tv_sec = *body->act_sensor_time->data.timestamp;
		time.tv_usec = 0;
		time_mode = 1;
		if (verbose)
		    printf("sensor time: %lu.%lu, %i\n", time.tv_sec, time.tv_usec, *body->act_sensor_time->tag);
	    }
	    if (body->act_gateway_time) {
		time.tv_sec = *body->act_sensor_time->data.timestamp;
		time.tv_usec = 0;
		time_mode = -1;
		if (verbose)
		    printf("sensor time: %lu.%lu, %i\n", time.tv_sec, time.tv_usec, *body->act_sensor_time->tag);
	    }
	    for (entry = body->val_list; entry != NULL; entry = entry->next) {	/* linked list */
		//int unit = (entry->unit) ? *entry->unit : 0;
		int scaler = (entry->scaler) ? *entry->scaler : 1;
		// printf("  scale: %d  unit: %d\n",unit,scaler);
		double value;

		switch (entry->value->type) {
		case 0x51:
		    value = *entry->value->data.int8;
		    break;
		case 0x52:
		    value = *entry->value->data.int16;
		    break;
		case 0x54:
		    value = *entry->value->data.int32;
		    break;
		case 0x58:
		    value = *entry->value->data.int64;
		    break;
		case 0x61:
		    value = *entry->value->data.uint8;
		    break;
		case 0x62:
		    value = *entry->value->data.uint16;
		    break;
		case 0x64:
		    value = *entry->value->data.uint32;
		    break;
		case 0x68:
		    value = *entry->value->data.uint64;
		    break;
		default:
		    //      fprintf(stderr, "Unknown value type: %x\n", entry->value->type);
		    value = 0;
		    break;
		}

		/* apply scaler */
		value *= pow(10, scaler);
		/* printf("Value: %f\n",value); */

		/* get time
		 if (entry->val_time) { // TODO handle SML_TIME_SEC_INDEX
		      time.tv_sec = *entry->val_time->data.timestamp;
		        time.tv_usec = 0;
		        time_mode = 2;
		 } else if (time_mode == 0) {
		        gettimeofday(&time, NULL);
		        time_mode = 3;
		} */
		gettimeofday(&time, NULL);

		pthread_mutex_lock(&value_mutex);
		if (!memcmp(entry->obj_name->str, obis_tarif0, sizeof(obis_tarif0))) {
		    counter_tarif0 = (int)(value + 0.5);
		    if (verbose)
			printf("CT0:%d\n", counter_tarif0);
		}
		if (!memcmp(entry->obj_name->str, obis_tarif1, sizeof(obis_tarif1))) {
		    counter_tarif1 = (int)(value + 0.5);
		    if (verbose)
			printf("CT1:%d\n", counter_tarif1);
		}
		if (!memcmp(entry->obj_name->str, obis_tarif2, sizeof(obis_tarif2))) {
		    counter_tarif2 = (int)(value + 0.5);
		    if (verbose)
			printf("CT2:%d\n", counter_tarif2);
		}
		if (!memcmp(entry->obj_name->str, obis_deliver0, sizeof(obis_deliver0))) {
		    counter_deliver0 = (int)(value + 0.5);
		    if (verbose)
			printf("CB0:%d\n", counter_deliver0);
		}
		if (!memcmp(entry->obj_name->str, power_meter, sizeof(power_meter))) {
		    pmeter = (int)(value + 0.5);
		    if (verbose)
			printf("Power:%d\n", pmeter);
		}
		pthread_mutex_unlock(&value_mutex);

		/* printf("%lu.%lu (%i)\t%.2f %s\n", time.tv_sec, time.tv_usec, time_mode, value, dlms_get_unit(unit)); */
		if (verbose) {
		    print_octet_str(entry->obj_name);
		    printf("%lu.%lu (%i)\t%.2f\n", time.tv_sec, time.tv_usec, time_mode, value);
		}
	    }
	}
	/* iterating through linked list */
	for (m = 0; m < n && entry != NULL; m++) {
	    /*  meter_sml_parse(entry, &rds[m]); */
	    printf(" -> entry %d\n", m);
	    entry = entry->next;
	}
    }

    if (verbose)
	sml_file_print(file);

    sml_file_free(file);
}

void *reader_thread(void *threadarg) {
    struct edl21_data *data;

    data = (struct edl21_data *) threadarg;
    sml_transport_listen(data->fd, &transport_receiver);
    return 0;
}

int main(int argc, char **argv) {
    pid_t pid;
    int opt, foreground;
    char device[MAX_STRING_LEN];
    char *logfile;

    struct edl21_data edl21_thread_data;
    struct snmp_data snmp_thread_data;

    foreground = 0;
    snmp_thread_data.snmp_port = SNMP_PORT;
    bzero(device, sizeof(device));
    strcpy(device, "/dev/ttyUSB0");

    while ((opt = getopt(argc, argv, "p:i:l:fh?")) != -1) {
	switch (opt) {
	case 'p':
	    snmp_thread_data.snmp_port = strtoul(optarg, (char **)NULL, 10);
	    break;
	case 'f':
	    foreground = 1;
	    break;
	case 'i':
	    if (strlen(optarg) < MAX_STRING_LEN) {
		bzero(device, MAX_STRING_LEN);
		strcpy(device, optarg);
	    } else {
		fprintf(stderr, "device name to long\n");
		exit(1);
	    }
	    break;
	case 'l':
	    if (strlen(optarg) < MAX_STRING_LEN) {
		asprintf(&logfile, "%s", optarg);
		log_file_ptr = fopen(logfile, "wb+");
		if (!log_file_ptr) {
		    fprintf(stderr, "can't open log file: %s\n", strerror(errno));
		    exit(1);
		}
	    } else {
		fprintf(stderr, "log file name to long\n");
		exit(1);
            }
	    break;
	case 'h':
	case '?':
	    print_usage(basename(argv[0]));
	    exit(0);
	default:
	    fprintf(stderr, "Unknown option %c\n", opt);
	    print_usage(basename(argv[0]));
	    exit(1);
	}
    }
    verbose = foreground;

    pthread_t thread_reader;
    pthread_t thread_snmp;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    edl21_thread_data.fd = serial_port_open(device);
    if (edl21_thread_data.fd > 0) {
	if (!foreground) {
	    pid = fork();
	    if (pid < 0)
		exit(EXIT_FAILURE);
	    if (pid > 0)
		exit(EXIT_SUCCESS);
	}
	if (pthread_create(&thread_reader, NULL, reader_thread, &edl21_thread_data)) {
	    pthread_exit(NULL);
	    exit(1);
	}
	if (pthread_create(&thread_snmp, NULL, snmp_agent, &snmp_thread_data)) {
	    pthread_cancel(thread_reader);
	    pthread_exit(NULL);
	    exit(1);
	}
	pthread_join(thread_reader, NULL);
	exit(1);
	pthread_join(thread_snmp, NULL);
	exit(1);
    } else {
	fprintf(stderr, "can't open %s\n", device);
	exit(1);
    }
    return 0;
}
