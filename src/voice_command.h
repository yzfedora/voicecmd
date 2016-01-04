#ifndef _VOICE_COMMAND_H
#define _VOICE_COMMAND_H

#define CMDREC_TABLE_SZ	27

struct voice_command {
	char	*vc_cmdstr;		/* associated command */
	char	*vc_cmdstr_end;		/* end pointer of command string */

	void (*vc_action)(void *arg);	/* associated function */
	struct voice_command	*vc_next;
};

int voice_command_register(char *command, void (*action)(void *arg));
int voice_command_searching(char *command);
#endif
