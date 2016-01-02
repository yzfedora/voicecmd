/* This is a voice recogniton program written in C language.
 *		Copyright (C)  2015 Yang Zhang  <yzfedora@gmail.com>
 */
#define __DEBUG__
#include "sndcap.h"
#include "google_sprec.h"
#include "voice_action.h"
#include "debug.h"


int main(int argc, char *argv[])
{
	struct sndcap *snd;
	struct google_sprec *gs;
	/*
	 * regiser all of the voice commands action.
	 */
	voice_action_register_all();
	DEBUG("all kind of voice actions has been registered.");

	if (!(snd = sndcap_new(NULL, SI_FORMAT_S16_LE, 1, 16000))) {
		fprintf(stderr, "sndcap_new error\n");
		return 1;
	}

	if (sndcap_init(snd) == -1) {
		fprintf(stderr, "%s", sndcap_strerror(snd));
		return 1;
	}
	DEBUG("sound device has been initialized.");

	if (!(gs = google_sprec_new())) {
		fprintf(stderr, "google_sprec_new error\b");
		return 1;
	}

	if (google_sprec_init(gs) == -1) {
		fprintf(stderr, "%s", google_sprec_strerror(gs));
		return 1;
	}
	DEBUG("google speech recognition data structures has "
					"been initialized.");
	
	while (1) {
		printf("###### Ready to talking ######\n");
		if (sndcap_listen(snd, 1, 5) == -1) {
			fprintf(stderr, "%s", sndcap_strerror(snd));
			continue;
		}
		
		DEBUG("try using google speech to recognize voice data...");
		if (google_sprec_lookup(gs, snd) == -1) {
			fprintf(stderr, "%s", google_sprec_strerror(gs));
			continue;
		}
	
		google_sprec_res_display(gs);
		
		/* implemented to call the function voice_command_searching().*/
		DEBUG("try to seaching voice command...\n");
		if (google_sprec_searching(gs) == -1) {
			fprintf(stderr, "\e[31mcommand not found...\e[0m\n");
		}
	}

	google_sprec_delete(gs);
	DEBUG("google speech recognition associated data structure has "
					"been free successfully.");
	sndcap_delete(snd);
	DEBUG("sound device has been closed successfully.");
	return 0;
}
