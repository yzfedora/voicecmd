/* This is a voice recogniton program written in C language.
 *		Copyright (C)  2015 Yang Zhang  <yzfedora@gmail.com>
 */
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "voice_command.h"

static void action_what_is_the_time(void *arg)
{
	time_t t = time(NULL);
	printf("Current time is: %s\n", ctime(&t));
}

static void action_who_am_i(void *arg)
{
	system("who");
}

static void action_play_music(void *arg)
{
	system("mpg123 e.m.a.mp3");
}

void voice_action_register_all(void)
{
	voice_command_register("what is the time", action_what_is_the_time);
	voice_command_register("who am i", action_who_am_i);
	voice_command_register("play music", action_play_music);
}
