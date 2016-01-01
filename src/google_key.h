#ifndef _GOOGLE_KEY_H
#define _GOOGLE_KEY_H

struct google_key {
	char			*gk_key;	/* pointer to google key. */
	struct google_key	*gk_next;
};

int google_key_init(const char *conf);
char *google_key_next(void);
#endif
