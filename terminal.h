

#ifndef TERMINAL_H
#define TERMINAL_H

typedef int (*term_command_handler_t) (int, char **);

typedef struct {
	const char *command;
	term_command_handler_t handler;	
	const char *brief;
	const char *help;
} term_command_t;

void term_init(void);
void print(const char *format, ...);

void terminal_task(void);

#endif //end TERMINAL_H

