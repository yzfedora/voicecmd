/* This is a voice recogniton program written in C language.
 *		Copyright (C)  2015 Yang Zhang  <yzfedora@gmail.com>
 */
#define VOICECMD_DEMO	/* disable it in you formal program. */
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

#ifdef VOICECMD_DEMO
/*
 * following is the part of demo. just print the voice command it matched.
 */
static void action_open_the_door(void *arg)
{
	printf("\e[1m\e[4mOpen the door...\e[0m\n");
}

static void action_close_the_door(void *arg)
{
	printf("\e[1m\e[4mClose the door...\e[0m\n");
}

static void action_turn_on_the_light(void *arg)
{
	printf("\e[1m\e[4mTurn on the light...\e[0m\n");
}

static void action_turn_off_the_light(void *arg)
{
	printf("\e[1m\e[4mTurn off the light...\e[0m\n");
}

static void action_turn_on_all_lights(void *arg)
{
	printf("\e[1m\e[4mTurn on all lights...\e[0m\n");
}

static void action_turn_off_all_lights(void *arg)
{
	printf("\e[1m\e[4mTurn off all lights...\e[0m\n");
}

#endif


void voice_action_register_all(void)
{
	voice_command_register("what is the time", action_what_is_the_time);
	voice_command_register("who am i", action_who_am_i);
	voice_command_register("play music", action_play_music);
#ifdef VOICECMD_DEMO
	voice_command_register("open the door", action_open_the_door);
	voice_command_register("close the door", action_close_the_door);
	voice_command_register("turn on the light", action_turn_on_the_light);
	voice_command_register("turn off the light", action_turn_off_the_light);
	voice_command_register("turn on all lights", action_turn_on_all_lights);
	voice_command_register("turn off all lights", action_turn_off_all_lights);
#endif
}
