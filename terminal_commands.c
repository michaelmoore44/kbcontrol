/*
 * terminal_commands.c
 *
 * Created: 4/21/2016 5:25:02 PM
 *  Author: michael
 */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "terminal.h"
#include "terminal_commands.h"

#define ARRAY_SIZE(a)      (sizeof(a) / sizeof((a)[0]))

#define TERM_OK   (0)
#define TERM_FAIL (1)

#define BUF_TEMP_SIZE (256)
uint8_t buf_temp[BUF_TEMP_SIZE];


static const char help_brief[] =
	"Print help for command or show all commands";

static const char help_help[] =
	"Usage: help [command]\r\n"
	"	Prints the help text for the command  argument\r\n"
	"	If no argument all commands will be printed\r\n";

static const char num_commands_brief[] =
	"Print the number of available terminal commands";

static const char num_commands_help[] =
	"Usage: num-commands\r\n"
	"	Prints the help text for the command  argument\r\n"
	"	If no argument all commands will be printed\r\n";

static const char test_brief[] =
	"Generic test function available for development";

static const char test_help[] =
	"Usage: test\r\n"
	"	Generic test function available for development\r\n";



static void print_available_commands(void);
static void print_command_help(const char* command);

static int term_help_handler(int argc, char **argv)
{
	if(argc < 2)
		print_available_commands();
	else
		print_command_help(argv[1]);
		
	return TERM_OK;
}

static int term_num_commands_handler(int argc, char **argv)
{
	print("There are %u available terminal commands\r\n", get_command_count());
	return TERM_OK;
}

static int term_test_handler(int argc, char **argv)
{
	unsigned long address;
	uint8_t i;

	if(argc < 2){
		print("test h for help menu\r\n");
		return TERM_FAIL;
	}

	if(strcmp(argv[1], "h") == 0){
		print("\tNo help functionality yet\r\n");
	}
	//else if(strcmp(argv[1], "flat") == 0){
    //    ext_flash_test();
	//}
	else
		print("??\r\n");
		
	return TERM_OK;
}



term_command_t term_commands[] = {
	{"help",               term_help_handler,            help_brief,            help_help},
	{"num-commands",       term_num_commands_handler,    num_commands_brief,    num_commands_help},
	{"test",               term_test_handler,            test_brief,            test_help},
};

static const uint16_t term_command_count = ARRAY_SIZE(term_commands);

static void print_available_commands(void)
{
	uint8_t i;
	
	print("Commands:\r\n");
	
	for (i = 0; i < ARRAY_SIZE(term_commands); i++) {
		if(!term_commands[i].command)		
			break;
		print("	%s - %s\r\n", term_commands[i].command,
		                                    term_commands[i].brief);
	}
}

static void print_command_help(const char* commando)
{
	uint8_t i;
	
	for (i = 0; i < ARRAY_SIZE(term_commands); i++) {
		if(!term_commands[i].command)		
			break;

		if(strcmp(commando, term_commands[i].command) == 0){
			print(term_commands[i].help);
			return;
		}
	}

	print("Unknown Command: %s\r\n", commando);
}

term_command_t* get_commands(void)
{
	return  term_commands;
}

uint16_t get_command_count(void)
{
	return  term_command_count;
}

void terminal_commands_init(void)
{
}

