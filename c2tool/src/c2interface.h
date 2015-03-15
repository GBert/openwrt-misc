#ifndef __C2INTERFACE_H
#define __C2INTERFACE_H

struct c2interface {
	int gpio_c2d;
	int gpio_c2ck;
	int gpio_c2ckstb;
};

struct c2_device_info {
	unsigned char device_id;
	unsigned char revision_id;
};

struct c2_pi_info {
	unsigned char version;
	unsigned char derivative;
};

struct c2tool_state;

void c2_reset(struct c2interface *c2if);
int c2_halt(struct c2interface *c2if);
int c2_get_device_info(struct c2interface *c2if, struct c2_device_info *info);

int c2_get_pi_info(struct c2tool_state *state, struct c2_pi_info *info);

int c2_write_sfr(struct c2interface *c2if,unsigned char reg, unsigned char value);
int c2_write_direct(struct c2tool_state *state, unsigned char reg, unsigned char value);
int c2_read_sfr(struct c2interface *c2if,unsigned char reg);
int c2_read_direct(struct c2tool_state *state, unsigned char reg);

int c2_flash_read(struct c2tool_state *state, unsigned int addr, unsigned int length, unsigned char *dest);
int c2_flash_erase_device(struct c2tool_state *state);
int flash_chunk(struct c2tool_state *state, unsigned int addr, unsigned int length, unsigned char *src);

#endif /* __C2INTERFACE_H */
