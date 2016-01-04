/* This is a voice recogniton program written in C language.
 *		Copyright (C)  2015 Yang Zhang  <yzfedora@gmail.com>
 */
#define _GNU_SOURCE
#include <string.h>
#include "sphinx_sprec.h"
#include "voice_command.h"

bool sphinx_sprec_is_prompt(struct sphinx_sprec *ss, char *prompt)
{
	if (strcasestr(ss->sr_hyp, prompt))
		return true;
	return false;
}

int sphinx_sprec_searching(struct sphinx_sprec *ss)
{
	return voice_command_searching(ss->sr_hyp);
}

void sphinx_sprec_display(struct sphinx_sprec *ss)
{
	printf("\e[32mCMU Sphinx recognition result is: \e[0m"
			"\e[1m%s\e[0m\n", ss->sr_hyp);
}

int sphinx_sprec_lookup(struct sphinx_sprec *ss, struct sndcap *snd)
{
	const char *hyp;

	ss->sr_hyp = NULL;	/* reset result pointer */
	ps_start_utt(ss->sr_ps);
	ps_process_raw(ss->sr_ps, (int16_t *)snd->sc_buf_ptr,
			snd->sc_capinfo_totsamp, FALSE, FALSE);
	ps_end_utt(ss->sr_ps);
	if (!(hyp = ps_get_hyp(ss->sr_ps, NULL)))
		return -1;

	ss->sr_hyp = hyp;
	return 0;
}

void sphinx_sprec_delete(struct sphinx_sprec *ss)
{
	if (!ss)
		return;
	if (ss->sr_ps)
		ps_free(ss->sr_ps);
	if (ss->sr_conf)
		cmd_ln_free_r(ss->sr_conf);
}

struct sphinx_sprec *sphinx_sprec_new()
{
	struct sphinx_sprec *ss;

	if (!(ss = calloc(1, sizeof(*ss))))
		goto out;

	if (!(ss->sr_conf = cmd_ln_init(NULL, ps_args(), TRUE,
					"-logfn", "/dev/null",
					"-hmm", MODDIR "/en-us/en-us",
					"-lm", "../conf/language_model.lm",
					"-dict", "../conf/dictionary.dict",
					NULL)))
		goto out;

	if (!(ss->sr_ps = ps_init(ss->sr_conf)))
		goto out;

	return ss;
out:
	sphinx_sprec_delete(ss);
	return NULL;
}
