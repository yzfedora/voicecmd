#ifndef _SOUND_CAPTURE_H
#define _SOUND_CAPTURE_H
#include <stdint.h>
#include <alsa/asoundlib.h>
#include "sndinfo.h"

typedef enum { false, true } bool;

#define SC_ERR_SZ	2048
#define SC_BUF_SZ	(1024 * 1024 *4)
#define SC_DEVICE_NAME	"default"

struct sndcap {
	char		*sc_device_name;
	char		*sc_err_ptr;

	snd_pcm_t	*sc_pcm_cap;	/* for capture sound */
	snd_pcm_t	*sc_pcm_beep;	/* for play beep */

	/*
	 * used to store the beep audio file, by load it to the memory, make
	 * it speed up when we try to play!
	 */
	char		*sc_beep_ptr;
	size_t		sc_beep_size;

	/*
	 * Note: sc_buf_ptr will be a larger enough space to store the sound
	 * data. to prevent the disk access.
	 * normally, I only provide 16bit, 16kHz, and mono sound capture. if
	 * the time of capture in 0~30 seconds. this is no problem for default
	 * SC_BUF_SZ(4MiB). YOU CAN INCREASE IT TO SUITABLE SIZE.
	 */
	char		*sc_buf_ptr;
	uint32_t	sc_buf_len;
	uint32_t	sc_buf_size;


	struct sndinfo	sc_capinfo;	/* auido info for capture */
	struct sndinfo	sc_beepinfo;	/* for playback info */
};

#define sc_capinfo_format	sc_capinfo.si_format
#define sc_capinfo_bps		sc_capinfo.si_bits_per_sample
#define sc_capinfo_chn		sc_capinfo.si_channels
#define sc_capinfo_samprt	sc_capinfo.si_sample_rate
#define sc_capinfo_fpp		sc_capinfo.si_frames_per_period
#define sc_capinfo_tpp		sc_capinfo.si_time_per_period
#define sc_capinfo_bpp		sc_capinfo.si_bytes_per_period
#define sc_capinfo_totsamp	sc_capinfo.si_total_sample

#define sc_beepinfo_format	sc_beepinfo.si_format
#define sc_beepinfo_bps		sc_beepinfo.si_bits_per_sample
#define sc_beepinfo_chn		sc_beepinfo.si_channels
#define sc_beepinfo_samprt	sc_beepinfo.si_sample_rate
#define sc_beepinfo_fpp		sc_beepinfo.si_frames_per_period
#define sc_beepinfo_tpp		sc_beepinfo.si_time_per_period
#define sc_beepinfo_bpp		sc_beepinfo.si_bytes_per_period
#define sc_beepinfo_totsamp	sc_beepinfo.si_total_sample


int sndcap_listen(struct sndcap *snd, uint32_t silent_time, uint32_t timedout);
void sndcap_delete(struct sndcap *snd);
int sndcap_init(struct sndcap *snd);
struct sndcap *sndcap_new(char *device_name,
			  snd_pcm_format_t format,
			  uint32_t channels,
			  uint32_t sample_rate);
#define sndcap_strerror(snd) ((snd)->sc_err_ptr)
#endif
