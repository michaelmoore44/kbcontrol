/*
 * terminal_commands.h
 *
 * Created: 4/21/2016 5:17:12 PM
 *  Author: michael
 */ 


#ifndef TERMINAL_COMMANDS_H_
#define TERMINAL_COMMANDS_H_

#include "terminal.h"

term_command_t* get_commands(void);
uint16_t get_command_count(void);
void terminal_commands_init(void);

#endif /* TERMINAL_COMMANDS_H_ */