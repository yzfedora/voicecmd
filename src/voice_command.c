/* This is a voice recogniton program written in C language.
 *		Copyright (C)  2015 Yang Zhang  <yzfedora@gmail.com>
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "voice_command.h"

/*
 * the voice_command structure will be stored in a certainly order, according the
 * name of command.
 */
static struct voice_command *cr_entry[CMDREC_TABLE_SZ];

static int voice_command_get_index(const char *cmd)
{
	char ch = *cmd;
	if (ch >= 'a' && ch <= 'z')
		return ch - 'a';
	if (ch >= 'A' && ch <= 'Z')
		return (ch - 'A');
	return CMDREC_TABLE_SZ - 1;
}

static void voice_command_add(struct voice_command *cmd)
{
	int idx = voice_command_get_index(cmd->vc_cmdstr);
	struct voice_command **last = &cr_entry[idx], *next;

	next = *last;
	while (next) {
		last = &next->vc_next;
		next = next->vc_next;
	}
	*last = cmd;
}

/*
 * When program going to exit, it will automatically free the allocated memory.
 */
static void __attribute__((destructor)) voice_command_free(void)
{
	struct voice_command *save, *next;
	int i;

	for (i = 0; i < CMDREC_TABLE_SZ; i++) {
		next = cr_entry[i];
		while (next) {
			save = next;
			next = next->vc_next;
			free(save->vc_cmdstr);
			free(save);
		}
	}
}

static void voice_command_split(char *string)
{
	char *ptr;

	while ((ptr = strchr(string, ' '))) {
		*ptr++ = 0;
		string = ptr + 1;
	}
}

static int voice_command_is_found(char *string, struct voice_command *cmd)
{
	char *key = cmd->vc_cmdstr;

	/* this compare method will let some command never be executed.
	while (key < cmd->vc_cmdstr_end) {
		if (!(p2 = strcasestr(p1, key)))
			return 0;
		p1 = p2 + strlen(key) + 1;
		key += strlen(key) + 1;
	}*/

	/*
	 * compare every words in sort of order. ignore the case and other
	 * words in the end if voice command has found.
	 */
	voice_command_split(string);
	while (key < cmd->vc_cmdstr_end) {
		if (!strcasestr(string, key))
			return 0;
		string += strlen(string) + 1;
		key += strlen(key) + 1;
	}

	return 1;
}

/*
 * register the command, when the searching function matched a command.
 * it will call the action function automatically.
 * Note:
 *	parameter 'command' is a string, and must be start with letter.
 * ('a' - 'z' or 'A' - 'Z'), any space, tab, or others will be mapped
 * a invalid index in the global command table 'cr_entry'
 */
int voice_command_register(char *command, void (*action)(void *arg))
{
	struct voice_command *cmd;

	if (!command || !action) {
		errno = -EINVAL;
		return -1;
	}

	if (!(cmd = calloc(1, sizeof(*cmd))))
		return -1;

	cmd->vc_cmdstr = strdup(command);
	cmd->vc_cmdstr_end = cmd->vc_cmdstr + strlen(cmd->vc_cmdstr);
	cmd->vc_action = action;

	voice_command_split(cmd->vc_cmdstr);
	voice_command_add(cmd);
	return 0;
}


/*
 * Note:
 *	this command searching function using the some string contain
 * method to determine the command is or not found! for example: to
 * match the command string "who am i". if the recognition result is
 * contains this 3 keyword: "who", "am", "i" in the sort of order, the
 * finally searching result will be true. and to call the registered
 * function(action) before. in other word, "who is am i", and "who am i"
 * is same searching result.
 *
 * Why I using this method to determine a command is or not be recognized?
 *	because the recognition result is depending the Google Speech API
 * or other Speech Recogniton Engine. normally, they're not 100% confident
 * the result. the accuracy of Google Speech API at least is 80%~97%. and
 * CMU sphinx only have %30~50%. except you training your own acourstic
 * model or using Keyword Spotting model.
 */
int voice_command_searching(char *string)
{
	int idx;
	struct voice_command *last, *next;

	if (!string)
		return -1;

	idx  = voice_command_get_index(string);
	next = last = cr_entry[idx];
	while (next) {
		if (voice_command_is_found(string, next)) {
			next->vc_action(string);
			return 0;
		}
		next = next->vc_next;
	}

	return -1;
}

