/* This is a voice recogniton program written in C language.
 *		Copyright (C)  2015 Yang Zhang  <yzfedora@gmail.com>
 */
#include "sndcap.h"
#include "google_sprec.h"
#include "sphinx_sprec.h"
#include "voice_action.h"
#include "debug.h"


static struct sndcap *snd;
static struct google_sprec *gs;
static struct sphinx_sprec *ss;
char *prompt = "Fido";

static int voicecmd_init(void)
{
	/*
	 * register all kind of actions the voice command. If you want to
	 * add new voice command, you need to add the action function in the
	 * voice_action_register_all(), by calling voice_command_register().
	 * more details please read the "voice_action.c"
	 */
	voice_action_register_all();
	DEBUG("All kind of voice actions has been registered.");

	/*
	 * register alsa library for capture sound.
	 */
	if (!(snd = sndcap_new(NULL, SI_FORMAT_S16_LE, 1, 16000))) {
		fprintf(stderr, "sndcap_new error\n");
		return -1;
	}

	if (sndcap_init(snd) == -1) {
		fprintf(stderr, "%s", sndcap_strerror(snd));
		return -1;
	}
	DEBUG("ALSA library has been initialized.");

	/*
	 * register PocketSphinx library.
	 */
	if (!(ss = sphinx_sprec_new())) {
		fprintf(stderr, "sphinx_sprec_new error\n");
		return -1;
	}
	DEBUG("Pocketsphinx library has been initialized");

	/*
	 * register the Google Speech API library.
	 */
	if (!(gs = google_sprec_new())) {
		fprintf(stderr, "google_sprec_new error\n");
		return -1;
	}

	if (google_sprec_init(gs) == -1) {
		fprintf(stderr, "%s", google_sprec_strerror(gs));
		return -1;
	}
	DEBUG("Google Speech API has been initialized.");
	return 0;
}

static void voicecmd_delete(void)
{

	sphinx_sprec_delete(ss);
	DEBUG("Pocketsphinx library has been freed.");

	google_sprec_delete(gs);
	DEBUG("Google Speech API has been freed.");

	sndcap_delete(snd);
	DEBUG("ALSA library has been freed.");
}

int main(int argc, char *argv[])
{

	if (voicecmd_init() == -1)
		return -1;
	
	while (1) {
		/*
		 * step 1.
		 *	running in listen model(24 hours), waiting user
		 * active this program.
		 */
		if (sndcap_listen(snd, 1, 3, false) == -1) {
			fprintf(stderr, "%s", sndcap_strerror(snd));
			continue;
		}

		/* no any voice was been recognized. */
		if (sphinx_sprec_lookup(ss, snd) == -1)
			continue;

		sphinx_sprec_display(ss);

		/* not the prompt */
		if (!sphinx_sprec_is_prompt(ss, prompt))
			continue;

		sleep(1);	/* sleep 1 second... */
		/*
		 * step 2. after the prompt has been detected. now ready to
		 * listen the real voice command from user.(by play the beep
		 * to notify the user, program has been ready.)
		 */
		if (sndcap_listen(snd, 1, 5, true) == -1) {
			fprintf(stderr, "%s", sndcap_strerror(snd));
			continue;
		}

		/* 
		 * try to searching the voice command, return 0 meaning
		 * voice command has been found. and action has been executed.
		 */
		DEBUG("Try using CMU Sphinx to recognition the voice data...");
		if (!sphinx_sprec_searching(ss))
			continue;
		DEBUG("CMU Sphinx recognition failure...");
		
	
		DEBUG("Try sending voice data to Google...");
		if (google_sprec_lookup(gs, snd) == -1) {
			fprintf(stderr, "%s", google_sprec_strerror(gs));
			continue;
		}
		DEBUG("Voice data has been recognized by Google.");
	
		google_sprec_res_display(gs);
		
		/* implemented to call the function voice_command_searching().*/
		DEBUG("Try to searching voice command...");
		if (google_sprec_searching(gs) == -1) {
			fprintf(stderr, "\e[31mcommand not found...\e[0m\n");
		}
	}

	voicecmd_delete();
	return 0;
}
