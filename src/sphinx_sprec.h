#ifndef _SPHINX_SPREC_H
#define _SPHINX_SPREC_H
#include <pocketsphinx.h>
#include "sndcap.h"

#define SS_ERR_SZ	2048

struct sphinx_sprec {
	cmd_ln_t	*sr_conf;
	ps_decoder_t	*sr_ps;
	char		*sr_hyp;
};
struct sphinx_sprec *sphinx_sprec_new();
void sphinx_sprec_delete(struct sphinx_sprec *ss);
int sphinx_sprec_lookup(struct sphinx_sprec *ss, struct sndcap *snd);
int sphinx_sprec_searching(struct sphinx_sprec *ss);
bool sphinx_sprec_is_prompt(struct sphinx_sprec *ss, char *prompt);
void sphinx_sprec_display(struct sphinx_sprec *ss);
#endif
