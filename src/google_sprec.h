#ifndef _GOOGLE_SPREC_H
#define _GOOGLE_SPREC_H
#include <stdint.h>
#include <curl/curl.h>
#include <stdint.h>
#include "sndinfo.h"
#include "sndcap.h"


#define KEY_FILE	"../conf/key.conf"
#define GOOGLE_URL	"https://www.google.com/speech-api/v2/recognize?"\
			"output=json&lang=en-us&key=%s"

#define GS_URL_SZ	1024
#define GS_ERR_SZ	2048
#define GS_DATA_SZ	4096	/* size to store the data of response */

/*
 * a linked-list to store the stringify recoginition result from Google.
 * and will be created automatically, when recognition results received from
 * Google in form of Json.
 * Note: This will be free automatically, when try to next recoginition or
 * delete the structure of google_sprec.
 */
struct google_sprec_res {
	char *rs_string;
	struct google_sprec_res *rs_next;
};

/*
 * for store all necessary handle, pointer, and structure. before to call any
 * google_sprec_*() functions, must call google_sprec_new() first.
 */
struct google_sprec {
	CURL			*sr_curl_handle;
	char			*sr_url;	/* request google url */
	char			*sr_err_ptr;	/* error string if have */

	/*
	 * for pointer to auido data we want to send.
	 * sr_snd_pos used in read callback funcion, for cURL.
	 * Note: this is a pointer just. no need to free.
	 */
	char			*sr_snd_ptr;	/* sound pointer */
	uint32_t		sr_snd_len;	/* sound length in bytes */
	uint32_t		sr_snd_pos;	/* used by callback func */

	/*
	 * used to store the response from Google, later will be parsed by
	 * using libjansson.
	 */
	char			*sr_data_ptr;	/* data pointer */
	uint32_t		sr_data_len;	/* data length */
	uint32_t		sr_data_size;	/* allocated size */

	struct sndinfo		sr_capinfo;	/* basic auido info */
	struct google_sprec_res *sr_res;	/* result of recognition */
};

#define sr_capinfo_format	sr_capinfo.si_format
#define sr_capinfo_bps		sr_capinfo.si_bits_per_sample
#define sr_capinfo_chn		sr_capinfo.si_channels
#define sr_capinfo_samprt	sr_capinfo.si_sample_rate
#define sr_capinfo_fpp		sr_capinfo.si_frames_per_period
#define sr_capinfo_tpp		sr_capinfo.si_time_per_period
#define sr_capinfo_bpp		sr_capinfo.si_bytes_per_period
#define sr_capinfo_totsamp	sr_capinfo.si_total_sample

int google_sprec_searching(struct google_sprec *gs);
int google_sprec_lookup(struct google_sprec *gs, struct sndcap *snd);
void google_sprec_delete(struct google_sprec *gs);
void google_sprec_res_display(struct google_sprec *gs);
int google_sprec_init(struct google_sprec *gs);
struct google_sprec *google_sprec_new(void);
char *google_sprec_strerror(struct google_sprec *gs);
#endif
