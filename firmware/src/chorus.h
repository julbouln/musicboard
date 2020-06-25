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
#include <math.h>

#define MOD_SINE	0
#define MOD_TRIANGLE 1

typedef struct _chorus_t {
	int	modulation;
	int	counter;
	int32_t	phase;
	int16_t	*chorusbuf;
	int16_t	in_gain, out_gain;
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
	float val;

	offset = max - 2 * depth;
	if (i < len / 2) {
		smpl = offset + (int32_t) (i * 2 * 2 * depth)/len;
	} else {
		smpl = offset + (int32_t) ((len - i) * 2 * 2 * depth)/len;
	}

	return smpl;
}


int chorus_init(chorus_t *chorus, int rate, float in_gain, float out_gain, float delay, float decay, float speed, float depth, int mod) {
	int i;

	chorus->in_gain = (int16_t)(in_gain * 32768.0f);
	chorus->out_gain = (int16_t)(out_gain * 32768.0f);

	chorus->delay = delay;
	chorus->decay = (int16_t)(decay * 32768.0f);
	chorus->speed = speed;
	chorus->depth = depth;
	chorus->modulation = mod;

	float sum_in_volume;
	chorus->maxsamples = 0;

	if ( chorus->in_gain < 0 )
		chorus->in_gain = 0;
/*
	if ( chorus->in_gain > 1.0 )
		chorus->in_gain = 1.0;
*/
	if ( chorus->out_gain < 0 )
		chorus->out_gain = 0;

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

	/* Be nice and check the hint with warning, if... */
	sum_in_volume = 1.0f;
	sum_in_volume += chorus->decay;
//	if ( chorus->in_gain * ( sum_in_volume ) > 1.0 / chorus->out_gain )
//	st_warn("chorus: warning >>> gain-out can cause saturation or clipping of output <<<");

//	printf("MALLOC %ld\n", sizeof (float) * chorus->maxsamples);
	if (! (chorus->chorusbuf = (int16_t *) TSF_MALLOC(sizeof (int16_t) * chorus->maxsamples)))
		return 0;

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
int chorus_process(chorus_t *chorus, int16_t *ibuf, int16_t *obuf, uint32_t len, unsigned int offset, unsigned int incr)
{
	int c;

	int16_t d_in = 0;
	int32_t d_out = 0;

	for (c = offset; c < len; c += incr) {
		/* Store delays as 24-bit signed longs */
		d_in = ibuf[c];
		/* Compute output first */
		d_out = (int32_t)d_in * (int32_t)chorus->in_gain;
		//int32_t
		int32_t mod_val;

		if (chorus->modulation == MOD_SINE)
			mod_val = _chorus_sine(chorus->phase, chorus->length, chorus->depth_samples - 1, chorus->depth_samples);
		else
			mod_val = _chorus_triangle(chorus->phase, chorus->length, chorus->samples - 1, chorus->depth_samples);

		d_out += (int32_t)chorus->chorusbuf[(chorus->maxsamples + chorus->counter - mod_val) % chorus->maxsamples] * (int32_t)chorus->decay;
		/* Adjust the output volume and size to 24 bit */
		d_out = (d_out >> 15) * (int32_t)chorus->out_gain;
		// out = st_clip24((LONG) d_out);
		obuf[c] =  __SSAT((int32_t)(d_out >> 15), 16);
		/* Mix decay of delay and input */
		chorus->chorusbuf[chorus->counter] = d_in;
		chorus->counter = ( chorus->counter + 1 ) % chorus->maxsamples;
		chorus->phase = (chorus->phase + 1 ) % chorus->length;
	}
	/* processed all samples */
	return 1;
}

#endif
