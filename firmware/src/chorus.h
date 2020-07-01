/* Chorus adapted from Sox chorus */
/*
 * August 24, 1998
 * Copyright (C) 1998 Juergen Mueller And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Juergen Mueller And Sundry Contributors are not responsible for
 * the consequences of using this software.
 */

/*
 * 	Chorus effect.
 *
 * Flow diagram scheme for n delays ( 1 <= n <= MAX_CHORUS ):
 *
 *        * gain-in                                           ___
 * ibuff -----+--------------------------------------------->|   |
 *            |      _________                               |   |
 *            |     |         |                   * decay 1  |   |
 *            +---->| delay 1 |----------------------------->|   |
 *            |     |_________|                              |   |
 *            |        /|\                                   |   |
 *            :         |                                    |   |
 *            : +-----------------+   +--------------+       | + |
 *            : | Delay control 1 |<--| mod. speed 1 |       |   |
 *            : +-----------------+   +--------------+       |   |
 *            |      _________                               |   |
 *            |     |         |                   * decay n  |   |
 *            +---->| delay n |----------------------------->|   |
 *                  |_________|                              |   |
 *                     /|\                                   |___|
 *                      |                                      |
 *              +-----------------+   +--------------+         | * gain-out
 *              | Delay control n |<--| mod. speed n |         |
 *              +-----------------+   +--------------+         +----->obuff
 *
 *
 * The delay i is controled by a sine or triangle modulation i ( 1 <= i <= n).
 *
 * Usage:
 *   chorus gain-in gain-out delay-1 decay-1 speed-1 depth-1 -s1|t1 [
 *       delay-2 decay-2 speed-2 depth-2 -s2|-t2 ... ]
 *
 * Where:
 *   gain-in, decay-1 ... decay-n :  0.0 ... 1.0      volume
 *   gain-out :  0.0 ...      volume
 *   delay-1 ... delay-n :  20.0 ... 100.0 msec
 *   speed-1 ... speed-n :  0.1 ... 5.0 Hz       modulation 1 ... n
 *   depth-1 ... depth-n :  0.0 ... 10.0 msec    modulated delay 1 ... n
 *   -s1 ... -sn : modulation by sine 1 ... n
 *   -t1 ... -tn : modulation by triangle 1 ... n
 *
 * Note:
 *   when decay is close to 1.0, the samples can begin clipping and the output
 *   can saturate!
 *
 * Hint:
 *   1 / out-gain < gain-in ( 1 + decay-1 + ... + decay-n )
 *
*/

/*
 * Sound Tools chorus effect file.
 */

#ifndef CHORUS_H
#define CHORUS_H

#include <stdlib.h> /* Harmless, and prototypes atof() etc. --dgc */

#define MOD_SINE	0
#define MOD_TRIANGLE 1

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

typedef struct _chorus_t {
	int	modulation;
	int	counter;
	int32_t	phase;
	int16_t	*chorusbuf;
	float	delay;
	int16_t decay;
	float	speed, depth;
	int32_t	length;
	int	depth_samples, samples;
	int	maxsamples, fade_out;
} chorus_t;

static inline int32_t _chorus_sine(int i, long len, int max, int depth)
{
	int32_t smpl = 0;
	int32_t offset;
	float val;

	offset = max - depth;
	val = sinf((float)i / (float)len * 2.0 * M_PI);
	smpl = offset + (int32_t) (val * (float)depth);

	return smpl;
}

static inline int32_t _chorus_triangle(int i, long len, int max, int depth)
{
	int32_t smpl = 0;
	int32_t offset;

	offset = max - 2 * depth;
	if (i < len / 2) {
		smpl = offset + (int32_t) (i * 2 * 2 * depth)/len;
	} else {
		smpl = offset + (int32_t) ((len - i) * 2 * 2 * depth)/len;
	}

	return smpl;
}


int chorus_init(chorus_t *chorus, int rate, float delay, float decay, float speed, float depth, int mod) {
	int i;

	chorus->delay = delay;
	chorus->decay = (int16_t)(decay * 32768.0f);
	chorus->speed = speed;
	chorus->depth = depth;
	chorus->modulation = mod;

	chorus->maxsamples = 0;

	chorus->samples = (int) ( ( chorus->delay + chorus->depth ) * rate / 1000.0);
	chorus->depth_samples = (int) (chorus->depth * rate / 1000.0);

	if ( chorus->delay < 20.0 )
		chorus->delay = 20.0;

	if ( chorus->delay > 100.0 )
		chorus->delay = 100.0;

	if ( chorus->speed < 0.1 )
		chorus->speed = 0.1;

	if ( chorus->speed > 5.0 )
		chorus->speed = 5.0;

	if ( chorus->depth < 0.0 )
		chorus->depth = 0.0;

	if ( chorus->depth > 10.0 )
		chorus->depth = 10.0;

	if ( chorus->decay < 0 )
		chorus->decay = 0;

/*
	if ( chorus->decay > 1.0 )
		chorus->decay = 1.0;
*/
	chorus->length = (rate / chorus->speed);
	chorus->phase = 0;

	if ( chorus->samples > chorus->maxsamples )
		chorus->maxsamples = chorus->samples;

//	printf("MALLOC %ld\n", sizeof (float) * chorus->maxsamples);
	if(!chorus->chorusbuf)
		chorus->chorusbuf = (int16_t *) TSF_MALLOC(sizeof (int16_t) * chorus->maxsamples);
	else
		chorus->chorusbuf = (int16_t *) TSF_REALLOC(chorus->chorusbuf, sizeof (int16_t) * chorus->maxsamples);

	for ( i = 0; i < chorus->maxsamples; i++ )
		chorus->chorusbuf[i] = 0.0f;

	chorus->counter = 0;
	chorus->fade_out = chorus->maxsamples;

	return 1;
}

/*
 * Processed signed long samples from ibuf to obuf.
 * Return number of samples processed.
 */
int chorus_process(chorus_t *chorus, int32_t *in, int32_t *out, uint32_t samples)
{
	int16_t in_m = 0;
	int32_t out_m = 0;
	int32_t out_l, out_r;

	for (uint32_t c = 0; c < samples; c ++) {
		in_m = __SSAT(((in[c] >> 15)/4), 16);
		int32_t mod_val;

		if (chorus->modulation == MOD_SINE)
			mod_val = _chorus_sine(chorus->phase, chorus->length, chorus->depth_samples - 1, chorus->depth_samples);
		else
			mod_val = _chorus_triangle(chorus->phase, chorus->length, chorus->samples - 1, chorus->depth_samples);

		out_m = (int32_t)chorus->chorusbuf[(chorus->maxsamples + chorus->counter - mod_val) % chorus->maxsamples] * (int32_t)chorus->decay;

		out_m = out_m >> 15;

		out_l = *out++ >> 15;
		out_r = *out++ >> 15;

		out-=2;

		*out++ = (out_m + out_l * 3/4) << 15;
		*out++ = (out_m + out_r * 3/4) << 15;

		/* Mix decay of delay and input */
		chorus->chorusbuf[chorus->counter] = in_m;
		chorus->counter = ( chorus->counter + 1 ) % chorus->maxsamples;
		chorus->phase = (chorus->phase + 1 ) % chorus->length;
	}
	/* processed all samples */
	return 1;
}

#endif
