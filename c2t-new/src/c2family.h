#ifndef __C2FAMILY_H
#define __C2FAMILY_H

enum {
	C2_MEM_TYPE_FLASH,
	C2_MEM_TYPE_OTP
};

enum {
	C2_SECURITY_JTAG,
	C2_SECURITY_C2_1,
	C2_SECURITY_C2_2,
	C2_SECURITY_C2_3
};

struct c2_setupcmd {
	unsigned char token;
	unsigned char reg;
	unsigned char value;
	unsigned int time;
};

struct c2family {
	unsigned int device_id; /* c2/jtag family id */
	const char *name; /* name of device family */
	unsigned int mem_type; /* flash or otp */
	unsigned int page_size; /* number of bytes per code page */
	int has_sfle; /* true if device has sfle bit */
	unsigned int security_type; /* flash security type */
	unsigned char fpdat; /* flash programming data register address */
	struct c2_setupcmd *setup;	/* list of initialization commands */
};

struct c2tool_state;

int c2family_find(unsigned int device_id, struct c2family **family);
int c2family_setup(struct c2tool_state *state);

#endif /* __C2FAMILY_H */
