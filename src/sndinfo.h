#ifndef _SNDINFO_H
#define _SNDINFO_H
#include <stdint.h>
#include <alsa/asoundlib.h>

struct sndinfo {
	snd_pcm_format_t	si_format;
#define SI_FORMAT_S16_LE	SND_PCM_FORMAT_S16_LE
	uint32_t		si_bits_per_sample;
	uint32_t		si_channels;
	uint32_t		si_sample_rate;
	snd_pcm_uframes_t	si_frames_per_period;
#define SI_FRAMES_PER_PERIOD_DEF 32
	uint32_t		si_time_per_period;
	uint32_t		si_bytes_per_period;
	uint32_t		si_total_sample;
};

#endif
