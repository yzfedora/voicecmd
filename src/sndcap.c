/* This is a voice recogniton program written in C language.
 *		Copyright (C)  2015 Yang Zhang  <yzfedora@gmail.com>
 */
#include <stdlib.h>
#include <math.h>
#include "sndcap.h"

#define sndcap_err_goto(x, err, s)					\
	do {								\
		snprintf((x)->sc_err_ptr, SC_ERR_SZ,			\
			__FILE__ ", %d: " s " error: %s\n", __LINE__,	\
			snd_strerror(err));				\
		goto out;						\
	} while (0)

#define sndcap_syserr_goto(x, err, s)					\
	do {								\
		char buf[512];						\
		strerror_r(err, buf, sizeof(buf));			\
		snprintf((x)->sc_err_ptr, SC_ERR_SZ,			\
			__FILE__ ", %d: " s " error: %s\n", __LINE__,	\
			buf);						\
		goto out;						\
	} while (0)


static int sndcap_beep_preload(struct sndcap *snd)
{
	int ret = -1;
	int fd = -1;
	ssize_t nread, offset = 0, total;

	if ((fd = open("../conf/beep.raw", O_RDONLY)) == -1)
		sndcap_syserr_goto(snd, errno, "open");

	/* get the file size, and allocate a chunk of memory to store it. */
	if ((snd->sc_beep_size = lseek(fd, 0, SEEK_END)) == (off_t)-1)
		sndcap_syserr_goto(snd, errno, "lseek");

	if (lseek(fd, 0, SEEK_SET) == (off_t)-1)
		sndcap_syserr_goto(snd, errno, "lseek");

	if (!(snd->sc_beep_ptr = malloc(snd->sc_beep_size)))
		sndcap_syserr_goto(snd, errno, "malloc");

	/* load the beep auido data to memory */
	total = snd->sc_beep_size;
	while ((nread = read(fd, snd->sc_beep_ptr + offset, total)) > 0) {
		offset += nread;
		total -= nread;
	}

	if (nread == -1)
		sndcap_syserr_goto(snd, errno, "read");

	ret = 0;
out:
	if (fd != -1)
		close(fd);
	return ret;
}

static int sndcap_beep_play(struct sndcap *snd)
{
	int err;
	uint32_t bytes_per_frame = snd->sc_beepinfo_bps *
					snd->sc_beepinfo_chn / 8;
	ssize_t nwrt, offset = 0;
	size_t frames = snd->sc_beep_size / bytes_per_frame;

try_again:
	if ((err = snd_pcm_prepare(snd->sc_pcm_beep)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_prepare");

	while (frames > 0) {
		if ((nwrt = snd_pcm_writei(snd->sc_pcm_beep,
					   snd->sc_beep_ptr + offset,
					   frames)) < 0) {
			if (nwrt == -EPIPE)
				goto try_again;
			sndcap_err_goto(snd, errno, "snd_pcm_writei");
		}

		offset += nwrt * bytes_per_frame;
		frames -= nwrt;
	}

	snd_pcm_drain(snd->sc_pcm_beep);

	return 0;
out:
	return -1;
}

/*
 * the playback for beep only support: stereo, 16-bits little-endian, 16kHz.
 */
static int sndcap_beep_init(struct sndcap *snd)
{
	int err;
	int dir;
	snd_pcm_hw_params_t *params = NULL;

	if ((err = snd_pcm_open(&snd->sc_pcm_beep,
				snd->sc_device_name,
				SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_open");

	snd_pcm_hw_params_alloca(&params);
	if ((err = snd_pcm_hw_params_any(snd->sc_pcm_beep, params)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_hw_params_any");


	if ((err = snd_pcm_hw_params_set_access(snd->sc_pcm_beep, params,
					SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_hw_param_set_access");

	if ((err = snd_pcm_hw_params_set_format(snd->sc_pcm_beep, params,
						snd->sc_beepinfo_format)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_hw_params_set_format");

	if ((err = snd_pcm_hw_params_set_rate_near(snd->sc_pcm_beep, params,
				&snd->sc_beepinfo_samprt, &dir)) < 0)
		sndcap_err_goto(snd, err,  "snd_pcm_hw_params_set_rate_near");
	
	if ((err = snd_pcm_hw_params_set_channels(snd->sc_pcm_beep, params,
						snd->sc_beepinfo_chn)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_hw_params_set_channel");

	if ((err = snd_pcm_hw_params_set_period_size_near(
					snd->sc_pcm_beep,
					params,
					&snd->sc_capinfo_fpp,
					&dir)) < 0)
		sndcap_err_goto(snd, err,
				"snd_pcm_hw_params_set_period_size_near");

	if ((err = snd_pcm_hw_params(snd->sc_pcm_beep, params)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_hw_params");

	if ((err = snd_pcm_hw_params_get_period_size(
				params,
				&snd->sc_capinfo_fpp,
				&dir)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_hw_params_get_period_size");

	snd->sc_capinfo_bpp = snd->sc_capinfo_fpp * snd->sc_capinfo_bps / 8 *
					   snd->sc_capinfo_chn;

	snd_pcm_hw_params_get_period_time(params, &snd->sc_capinfo_tpp, &dir);

	if (sndcap_beep_preload(snd) == -1)
		goto out;

	return 0;
out:
	if (snd->sc_pcm_beep)
		snd_pcm_close(snd->sc_pcm_beep);
	return -1;
}

static bool sndcap_is_silent(struct sndcap *snd)
{
	int i;
	size_t based = pow(2, snd->sc_capinfo_bps - 1);
	short *samples = (short *)(snd->sc_buf_ptr + snd->sc_buf_len -
				(snd->sc_capinfo_samprt * snd->sc_capinfo_bps *
				snd->sc_capinfo_chn / 8));
	size_t total_sample = snd->sc_capinfo_samprt;
	size_t normal_sample = 0;
	double silent_ratio;

	for (i = 0; i < total_sample; i++) {
		if (samples[i] == 0)
			continue;

		if ((20 * log10((double)abs(samples[i]) / based)) > -60) {
			normal_sample++;
		}

	}

	silent_ratio = (double)(total_sample - normal_sample) /
			total_sample * 100;
	DEBUG("Critical ratio is %d%%, current voice silent ratio is %.2f%%",
				snd->sc_critical_ratio, silent_ratio);
	/*
	 * Here to adjust the silent ratio. according to you system
	 * microphone's volume.
	 */
	if (silent_ratio > snd->sc_critical_ratio)
		return true;
	return false;
}

/*
 * by calling this function to capture the voice from device.
 * - snd         sndcap structure pointer, we created before.
 * - silent_time waiting a number of silent time and go to exit. (>= 0).
 *               zero meaning don't use it.
 * - timedout    maximum time to capture. (must be > 0)
 * - playbeep    if true, play the beep sound. notify user start to talking.
 *
 * Note: silent_time and timedout both are in seconds.
 */
int sndcap_listen(struct sndcap *snd,
		  uint32_t silent_time,
		  uint32_t timedout,
		  bool playbeep)
{
	int err;
	int times, times_per_sec, silent_time_cnt = 0;

	if (!snd || silent_time < 0 || timedout <= 0) {
		errno = -EINVAL;
		return -1;
	}

	snd->sc_buf_len = 0;
	snd->sc_capinfo_totsamp = 0;
	times = timedout * 1000000 / snd->sc_capinfo_tpp;
	times_per_sec  = 1000000 / snd->sc_capinfo_tpp;

	if (playbeep && sndcap_beep_play(snd) == -1)
		goto out;

try_again:
	if ((err = snd_pcm_prepare(snd->sc_pcm_cap)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_prepapre");

	while (times-- > 0) {
	
		if ((snd->sc_buf_size - snd->sc_buf_len) < snd->sc_capinfo_bpp)
			break;	/* or re-allocate the size by use realloc() */

	
		if ((err = snd_pcm_readi(snd->sc_pcm_cap,
					 snd->sc_buf_ptr + snd->sc_buf_len,
					 snd->sc_capinfo_fpp)) !=
					 snd->sc_capinfo_fpp) {
			if (err == -EPIPE)
				goto try_again;
			
			sndcap_err_goto(snd, err, "snd_pcm_readi");
		}

	
		snd->sc_buf_len += err * snd->sc_capinfo_bps / 8 *
						snd->sc_capinfo_chn;
		snd->sc_capinfo_totsamp += err * snd->sc_capinfo_chn;

	
		/*
		 * only when silent_time is greater than 0, following counter
		 * will be start. if silent_time is zero, meaning don't use
		 * silent_time.
		 */	
		if (silent_time > 0 && --times_per_sec == 0) {
			if (sndcap_is_silent(snd)) {
				if (++silent_time_cnt >= silent_time)
					goto success;	/* silent time exceed */
			} else
				silent_time_cnt = 0;

			times_per_sec = 1000000 / snd->sc_capinfo_tpp;
		}
	}
success:
	return 0;
out:
	return -1;
}

void sndcap_delete(struct sndcap *snd)
{
	if (!snd)
		return;

	if (snd->sc_buf_ptr)
		free(snd->sc_buf_ptr);
	if (snd->sc_err_ptr)
		free(snd->sc_err_ptr);
	if (snd->sc_pcm_cap)
		snd_pcm_close(snd->sc_pcm_cap);

	if (snd->sc_beep_ptr)
		free(snd->sc_beep_ptr);
	if (snd->sc_pcm_beep)
		snd_pcm_close(snd->sc_pcm_beep);

	free(snd);
}

char *sndcap_strerror(struct sndcap *snd)
{
	if (!snd)
		return "Invalid argument";
	return snd->sc_err_ptr;
}

/* 
 * initial the sound card for audio capture or playback the beep when listening.
 * return 0 on success. or -1 on error. sndcap_error() will give you details.
 *
 * sound device initialization steps:
 * sndcap_init() ---> sndcap_beep_init() ---> sndcap_beep_preload()
 */
int sndcap_init(struct sndcap *snd)
{
	int err;
	int dir;
	snd_pcm_hw_params_t *params = NULL;

	if (!snd) {
		errno = -EINVAL;
		return -1;
	}

	if ((err = snd_pcm_open(&snd->sc_pcm_cap,
				snd->sc_device_name,
				SND_PCM_STREAM_CAPTURE, 0)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_open");

	snd_pcm_hw_params_alloca(&params);
	if ((err = snd_pcm_hw_params_any(snd->sc_pcm_cap, params)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_hw_params_any");


	if ((err = snd_pcm_hw_params_set_access(snd->sc_pcm_cap, params,
					SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_hw_param_set_access");

	if ((err = snd_pcm_hw_params_set_format(snd->sc_pcm_cap, params,
						snd->sc_capinfo_format)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_hw_params_set_format");

	if ((err = snd_pcm_hw_params_set_rate_near(snd->sc_pcm_cap, params,
				&snd->sc_capinfo_samprt, &dir)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_hw_params_set_rate_near");
	
	if ((err = snd_pcm_hw_params_set_channels(snd->sc_pcm_cap, params,
						snd->sc_capinfo_chn)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_hw_params_set_channel");

	if ((err = snd_pcm_hw_params_set_period_size_near(
					snd->sc_pcm_cap,
					params,
					&snd->sc_capinfo_fpp,
					&dir)) < 0)
		sndcap_err_goto(snd, err,
				"snd_pcm_hw_params_set_period_size_near");

	if ((err = snd_pcm_hw_params(snd->sc_pcm_cap, params)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_hw_params");

	if ((err = snd_pcm_hw_params_get_period_size(params,
						     &snd->sc_capinfo_fpp,
						     &dir)) < 0)
		sndcap_err_goto(snd, err, "snd_pcm_hw_params_get_period_size");

	snd->sc_capinfo_bpp = snd->sc_capinfo_fpp * snd->sc_capinfo_bps / 8 *
					   snd->sc_capinfo_chn;

	snd_pcm_hw_params_get_period_time(params, &snd->sc_capinfo_tpp, &dir);

	/*
	 * initial the pointer sc_pcm_beep, for we to play the beep when
	 * sndcap_listen is ready. and using should speacking or talking
	 * after heard the beep.
	 */	
	if (sndcap_beep_init(snd) == -1)
		goto out;

	return 0;
out:
	if (snd->sc_pcm_cap)
		snd_pcm_close(snd->sc_pcm_cap);
	return -1;
}

/*
 * create a sndcap structure by according the arguments.
 * - device_name    normally, it is string "default", or "plughw:x,y", or
 *                  "hw:x,y". when program error occured. and error message is
 *                  "Invalid Arguments". you should change the device_name to
 *                  appropriate, and suit for you system. more details please
 *                  go to website: "alsa-project.org".
 *
 * - format         current support SI_FORMAT_S16_LE only.
 * 
 * - channels       only support mono.
 * 
 * - sample_rate    Google Speech API accept 8Hkz ~ 44kHz, but using 16kHz
 *                  is sufficient.
 *
 *			
 */
struct sndcap *sndcap_new(char *device_name,
			  snd_pcm_format_t format,
			  uint32_t channels,
			  uint32_t sample_rate)
{
	struct sndcap *snd;

	if (!(snd = calloc(1, sizeof(*snd))) ||
	    !(snd->sc_err_ptr = malloc(SC_ERR_SZ)) ||
	    !(snd->sc_buf_ptr = malloc(SC_BUF_SZ)))
		goto out;

	snd->sc_buf_size = SC_BUF_SZ;

	snd->sc_device_name = SC_DEVICE_NAME;	/* default device name */
	if (device_name)
		snd->sc_device_name = device_name;

	/* sound capture format, specifies by caller. */
	snd->sc_capinfo_format = format;
	snd->sc_capinfo_bps = snd_pcm_format_physical_width(
						snd->sc_capinfo_format);
	snd->sc_capinfo_chn = channels;
	snd->sc_capinfo_samprt = sample_rate;
	snd->sc_capinfo_fpp = SI_FRAMES_PER_PERIOD_DEF;

	/* Fixed format of beep audio file. */
	snd->sc_beepinfo_format = SND_PCM_FORMAT_S16_LE;
	snd->sc_beepinfo_bps = snd_pcm_format_physical_width(
						snd->sc_capinfo_format);
	snd->sc_beepinfo_chn = 2;
	snd->sc_beepinfo_samprt = 16000;
	snd->sc_beepinfo_fpp = SI_FRAMES_PER_PERIOD_DEF;

	snd->sc_critical_ratio = 90;	/* default standard for silent ratio */

	return snd;
out:
	sndcap_delete(snd);
	return NULL;
}
