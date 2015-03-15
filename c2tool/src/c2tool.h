#ifndef __C2TOOL_H
#define __C2TOOL_H

#include "c2family.h"
#include "c2interface.h"

struct c2tool_state {
	struct c2interface c2if;
	struct c2family *family;
};

struct cmd {
	const char *name;
	const char *args;
	const char *help;
	int hidden;
	/*
	 * The handler should return a negative error code,
	 * zero on success, 1 if the arguments were wrong
	 * and the usage message should and 2 otherwise.
	 */
	int (*handler)(struct c2tool_state *state,
		       int argc, char **argv);
};

#define ARRAY_SIZE(ar) (sizeof(ar)/sizeof(ar[0]))
#define DIV_ROUND_UP(x, y) (((x) + (y - 1)) / (y))

#define __COMMAND(_symname, _name, _args, _hidden, _handler, _help)\
	static struct cmd						\
	__cmd ## _ ## _symname\
	__attribute__((used)) __attribute__((section("__cmd")))	= {	\
		.name = (_name),					\
		.args = (_args),					\
		.hidden = (_hidden),					\
		.handler = (_handler),					\
		.help = (_help),					\
	}
#define COMMAND(name, args, handler, help)	\
	__COMMAND(name, #name, args, 0, handler, help)
#define HIDDEN(name, args, handler)		\
	__COMMAND(name, #name, args, 1, handler, NULL)

extern const char c2tool_version[];

int handle_cmd(struct c2tool_state *state, int argc, char **argv);

#endif /* __C2TOOL_H */
