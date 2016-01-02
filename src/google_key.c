#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "google_key.h"

static struct google_key *key_head;
static struct google_key *key_curr;	/* for google_key_next() */
static struct google_key **key_last;

static int google_key_add(char *key)
{
	struct google_key *new;

	if (!(new = calloc(1, sizeof(*new))))
		return -1;

	new->gk_key = strdup(key);
	if (!key_last)
		key_last = &key_head;

	*key_last = new;
	key_last = &(*key_last)->gk_next;
	return 0;
}

static void strip_newline(char *ptr)
{
	char *p = strchr(ptr, '\n');

	if (p)
		*p = 0;
}

static void __attribute__((destructor)) google_key_free(void)
{
	struct google_key *next = key_head, *save;

	while (next) {
		save = next;
		next = next->gk_next;
		free(save->gk_key);
		free(save);
	}
}

int google_key_init(const char *conf)
{
	int ret = -1;
	FILE *fp = NULL;
	char *ptr = NULL;
	size_t len;

	if (!(fp = fopen(conf, "r")))
		goto out;

	while (!feof(fp)) {
		ptr = NULL;
		len = 0;
		
		if (getline(&ptr, &len, fp) == -1)
			goto next;

		strip_newline(ptr);

		if (strlen(ptr) != 39)
			goto next;

		if (google_key_add(ptr) == -1) {
			free(ptr);
			goto out;
		}
next:
		if (ptr)
			free(ptr);
	}

	ret = 0;
out:
	if (fp)
		fclose(fp);
	return ret;
}

char *google_key_next(void)
{
	char *key;

	if (!key_curr)
		key_curr = key_head;

	if (!key_curr)
		return NULL;

	key = key_curr->gk_key;
	key_curr = key_curr->gk_next;
	return key;
}
