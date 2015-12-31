/* This is a voice recogniton program written in C language.
 *		Copyright (C)  2015 Yang Zhang  <yzfedora@gmail.com>
 */
#include <errno.h>
#include <string.h>
#include <jansson.h>
#include "google_sprec.h"
#include "voice_command.h"

/* 
 * Note: following macros is used to reduce the error handle process.
 * anyone must not use in other place.
 */

#define google_sprec_err_goto(x, err, str)				\
	do {								\
		if (err) {						\
			char buf[512];					\
			strerror_r(err, buf, sizeof(buf));		\
			snprintf((x)->sr_err_ptr, GS_ERR_SZ, __FILE__	\
			", %d: " str " error: %s\n", __LINE__, buf);	\
		} else {						\
			snprintf((x)->sr_err_ptr, GS_ERR_SZ,		\
			__FILE__ ", %d: " str " error\n", __LINE__);	\
		}							\
		goto out;						\
	} while (0)

#define google_sprec_curl_err_goto(x, err, str, ...)			\
	do {								\
		if (err != CURLE_OK) {					\
			snprintf((x)->sr_err_ptr, GS_ERR_SZ, __FILE__	\
				", %d: " str " error: %s\n", __LINE__,	\
				##__VA_ARGS__, curl_easy_strerror(err));\
		} else {						\
			snprintf((x)->sr_err_ptr, GS_ERR_SZ,		\
				__FILE__ ", %d: " str "\n",		\
				__LINE__, ##__VA_ARGS__);		\
		}							\
		goto out;						\
	} while (0)

static int google_sprec_res_add(struct google_sprec *gs, const char *str)
{
	struct google_sprec_res **last, *next, *new;

	if (!(new = calloc(1, sizeof(*new))))
		return -1;

	last = &gs->sr_res;
	next = *last;
	while (next) {
		last = &next->rs_next;
		next = next->rs_next;
	}

	new->rs_string = strdup(str);
	*last = new;
	return 0;
}

static void google_sprec_res_free(struct google_sprec *gs)
{
	struct google_sprec_res *next = gs->sr_res, *tmp;

	while (next) {
		tmp = next;
		next = next->rs_next;
		free(tmp->rs_string);
		free(tmp);
	}
	gs->sr_res = NULL;
}

static int google_sprec_json_alternative_get(json_t *root, json_t **alternative)
{
	json_t *index;
	json_t *result;
	json_t *obj;
	json_t *final;
	
	if (!(result = json_object_get(root, "result")) ||
	    !(index = json_object_get(root, "result_index")))
		return -1;

	if (!(obj = json_array_get(result, (size_t)json_integer_value(index))))
		return -1;

	if (!(*alternative = json_object_get(obj, "alternative")))
		return -1;

	if (!(final = json_object_get(obj, "final")) ||
	    !json_is_boolean(final) || json_is_false(final))
		return -1;

	return 0;
}

static void google_sprec_json_parsing_data(struct google_sprec *gs, char *data)
{
	json_error_t err;
	size_t index, size;
	json_t *alternative;
	json_t *transcript_obj;
	json_t *transcript;
	const char *string;
	json_t *root = json_loads(data, 0, &err);

	if (!root) {
		snprintf(gs->sr_err_ptr, GS_ERR_SZ,
			"json_load error: line %d, %s\n",
			err.line, err.text);
		return;
	}

	if (google_sprec_json_alternative_get(root, &alternative) == -1)
		return;

	if (!(size = json_array_size(alternative)))
		return;

	/* process all result in array aternative, and store it */
	for (index = 0; index < size; index++) {
		if (!(transcript_obj = json_array_get(alternative, index)))
			continue;
		if (!(transcript = json_object_get(transcript_obj,
							"transcript")))
			continue;

		if ((string = json_string_value(transcript)))
			google_sprec_res_add(gs, string);
	}

	return;
}

static int google_sprec_json_parsing(struct google_sprec *gs)
{
	char *newline = NULL;
	char *data = gs->sr_data_ptr;

	do {
		if ((newline = strchr(data, '\n')))
			*newline = 0;

		google_sprec_json_parsing_data(gs, data);
		
		if (newline)
			data = newline + 1;
	} while (newline);
	return 0;
}

static size_t google_sprec_curl_read_callback(char *buffer,
					      size_t size,
					      size_t nitems,
					      void *instream)
{
	struct google_sprec *gs = instream;
	size_t nbytes = size * nitems;

	if (gs->sr_snd_len > 0) {
		nbytes = gs->sr_snd_len < nbytes ? gs->sr_snd_len : nbytes;
		memcpy(buffer, gs->sr_snd_ptr + gs->sr_snd_pos, nbytes);
		gs->sr_snd_pos += nbytes;
		gs->sr_snd_len -= nbytes;

		return nbytes;
	}
	return 0;
}

static size_t google_sprec_curl_write_callback(char *ptr,
					       size_t size,
					       size_t nmemb,
					       void *userdata)
{
	struct google_sprec *gs = userdata;
	size_t nbytes = size *nmemb;

	if ((gs->sr_data_len + nbytes) < gs->sr_data_size) {
		memcpy(gs->sr_data_ptr + gs->sr_data_len, ptr, nbytes);
		gs->sr_data_len += nbytes;
		gs->sr_data_ptr[gs->sr_data_len] = 0;
		return nbytes;
	}
	return 0;
}

static void google_sprec_curl_setopt(struct google_sprec *gs,
				     struct curl_slist **headers)
{
	char content_type[128];

	
	curl_easy_setopt(gs->sr_curl_handle, CURLOPT_URL, gs->sr_url);
	
	
	snprintf(content_type, sizeof(content_type), "Content-Type: audio/l16;"
				" rate=%d;", gs->sr_capinfo_samprt);
	
	*headers = curl_slist_append(*headers, content_type);
	
	
	curl_easy_setopt(gs->sr_curl_handle, CURLOPT_HTTPHEADER, *headers);

	
	curl_easy_setopt(gs->sr_curl_handle, CURLOPT_POST, 1L);
	curl_easy_setopt(gs->sr_curl_handle, CURLOPT_POSTFIELDSIZE, gs->sr_snd_len);

	gs->sr_snd_pos = 0;	/* reset position to 0 */
	curl_easy_setopt(gs->sr_curl_handle, CURLOPT_READFUNCTION,
			google_sprec_curl_read_callback);
	curl_easy_setopt(gs->sr_curl_handle, CURLOPT_READDATA, gs);

	gs->sr_data_len = 0;
	curl_easy_setopt(gs->sr_curl_handle, CURLOPT_WRITEFUNCTION,
			google_sprec_curl_write_callback);
	curl_easy_setopt(gs->sr_curl_handle, CURLOPT_WRITEDATA, gs);
}

/*
 * calling the voice_command_searching() to search the command whether
 * registered or not before.
 * 
 * Notes:
 *     if the matched a command, and it already registered before. it will to
 * call the action function automatically. following is the type:
 *
 *     void (*action)(void *);
 */
int google_sprec_searching(struct google_sprec *gs)
{
	struct google_sprec_res *res = gs->sr_res;
	
	while (res) {
		if (voice_command_searching(res->rs_string) == 0)
			return 0;
		res = res->rs_next;
	}
	return -1;
}

/*
 * by passing a sndcap structure as the second argument. it will send the audio
 * data which in the member 'sc_buf_ptr' to Google Speech Server, and the result
 * will be stored as json in 'sr_data_ptr'. and finally, will be parsed by
 * libjansson. the recognition result will be stored in a liked-list, which is
 * a google_sprec_res structure.
 * return 0 on success, or -1 on error. call google_sprec_strerror() get detail.
 */
int google_sprec_lookup(struct google_sprec *gs, struct sndcap *snd)
{
	int ret = -1;
	int err;
	int http_code;
	struct curl_slist *headers = NULL;	/* DON'T forget */

	google_sprec_res_free(gs);	/* free result of the last time */

	gs->sr_snd_ptr = snd->sc_buf_ptr;
	gs->sr_snd_len = snd->sc_buf_len;
	memcpy(&gs->sr_capinfo, &snd->sc_capinfo, sizeof(gs->sr_capinfo));
	google_sprec_curl_setopt(gs, &headers);

	
	if ((err = curl_easy_perform(gs->sr_curl_handle)) != CURLE_OK)
		google_sprec_curl_err_goto(gs, err, "curl_easy_perform");

	
        curl_easy_getinfo(gs->sr_curl_handle, CURLINFO_RESPONSE_CODE,
			  &http_code);
        if (http_code != 200) {
		google_sprec_curl_err_goto(gs, 0, "curl_easy_getinfo, return code %d", http_code);
                goto out;
        }


	if (google_sprec_json_parsing(gs) == -1)
		goto out;

	ret = 0;
out:
	curl_slist_free_all(headers);
	return ret;
}

/* free all kind of resources. */
void google_sprec_delete(struct google_sprec *gs)
{
	if (!gs)
		return;
	
	if (gs->sr_curl_handle)
		curl_easy_cleanup(gs->sr_curl_handle);
	curl_global_cleanup();
	
	if (gs->sr_url)
		free(gs->sr_url);
	if (gs->sr_data_ptr)
		free(gs->sr_data_ptr);
	
	google_sprec_res_free(gs);
	free(gs);
}

/*
 * Display the recognition result to stdout.
 */
void google_sprec_res_display(struct google_sprec *gs)
{
	struct google_sprec_res *next = gs->sr_res;

	while (next) {
		printf("recogniton result from google: %s\n", next->rs_string);
		next = next->rs_next;
	}
}

/*
 * initial the google_sprec structure. return 0 on success, or -1 on error.
 * and google_sprec_strerror() will return a string which described the error.
 */
int google_sprec_init(struct google_sprec *gs)
{
	curl_global_init(CURL_GLOBAL_ALL);
	if (!(gs->sr_curl_handle = curl_easy_init()))
		google_sprec_curl_err_goto(gs, 0, "curl_easy_init");

	return 0;
out:
	return -1;
}

/*
 * create a google_sprec structure pointer. return this pointer on success.
 * or NULL on error.
 */
struct google_sprec *google_sprec_new(void)
{
	struct google_sprec *gs = calloc(1, sizeof(*gs));

	if (!gs)
		goto out;

	if (!(gs->sr_url = malloc(GS_URL_SZ)) ||
	    !(gs->sr_err_ptr = malloc(GS_ERR_SZ)) ||
	    !(gs->sr_data_ptr = malloc(GS_DATA_SZ)))
		goto out;

	gs->sr_data_size = GS_DATA_SZ;
	snprintf(gs->sr_url, GS_URL_SZ, GOOGLE_URL, GOOGLE_KEY);

	return gs;
out:
	google_sprec_delete(gs);
	return NULL;
}
