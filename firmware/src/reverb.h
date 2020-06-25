#ifndef REVERB_H
#define REVERB_H
/* Reverb adapted from https://github.com/gordonjcp/reverb */

/*
Copyright (c) 2015 Gordon JC Pearce <gordonjcp@gjcp.net>

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice
and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
*/

//#define LOW_QUALITY // reduce memory requirement

/* note that buffers need to be a power of 2 */
/* if we scale the tap sizes for higher sample rates, this will need to be larger */
#ifdef LOW_QUALITY
#define COMB_SIZE 4096
#define AP_SIZE 2048

#define NUM_COMBS 4
#define NUM_APS 3
#else
#define COMB_SIZE 4096
#define AP_SIZE 2048

#define NUM_COMBS 4
#define NUM_APS 3
#endif

#define COMB_MASK (COMB_SIZE-1)
#define AP_MASK (AP_SIZE-1)

#define MAX_NUM_COMBS 4
#define MAX_NUM_APS 3

typedef struct {
    /* structure for reverb parameters */
    /* controls */
    float decay;
    float size;
    float colour;

    int16_t lpo;
    int16_t a0, b1;
    int16_t gl, gh;
    int16_t d1, d2;

    uint32_t tap[MAX_NUM_COMBS];
    uint32_t ap_tap[MAX_NUM_APS];

    int16_t comp_gain[MAX_NUM_COMBS];
    int16_t ap_gain;

    uint32_t comb_pos;             /* position within comb filter */
    uint32_t ap_pos;               /* position within allpass filter */
#ifdef LOW_QUALITY
    int8_t comb[NUM_COMBS][COMB_SIZE];   /* buffers for comb filters */
    int8_t ap[NUM_APS][AP_SIZE];        /* buffers for ap filters */
#else
    int16_t comb[NUM_COMBS][COMB_SIZE];   /* buffers for comb filters */
    int16_t ap[NUM_APS][AP_SIZE];        /* buffers for ap filters */
#endif
} reverb_t; // 22604 bytes

void reverb_set_colour(reverb_t *rev, float colour);
void reverb_set_size(reverb_t *rev, float size);
void reverb_set_decay(reverb_t *rev, float decay);
void reverb_init(reverb_t *rev);
void reverb_process(reverb_t *params, int16_t *in, int16_t *out, uint32_t sample_count, unsigned int offset, unsigned int incr);

void reverb_set_colour(reverb_t *rev, float colour) {
    rev->colour = colour;

    float gl, gh;

    if (colour > 0) {
        gl = -5 * colour;
        gh = colour;
    } else {
        gl = -colour;
        gh = 5 * colour;
    }

    gl = expf(gl / 8.66) - 1;
    gh = expf(gh / 8.66) - 1;

    rev->gl = (int16_t)(gl * 32768.0f);
    rev->gh = (int16_t)(gh * 32768.0f);
}

void reverb_set_size(reverb_t *rev, float size) {
    rev->size = size;

    rev->tap[0] = (int)(2975 * rev->size);
    rev->tap[1] = (int)(2824 * (rev->size / 2));
    rev->tap[2] = (int)(3621 * rev->size);
    rev->tap[3] = (int)(3970 * (rev->size / 1.5));

    rev->ap_tap[2] = (int)(400 * rev->size);
}

void reverb_set_decay(reverb_t *rev, float decay) {
    int c;
    float tap_gain[MAX_NUM_COMBS] = {0.964, 1.0, 0.939, 0.913};
    rev->decay = decay;

    for (c = 0; c < MAX_NUM_COMBS; c++) {
        rev->comp_gain[c] = (int16_t)(decay * tap_gain[c] * 32768.0f);
    }

    rev->ap_gain = (int16_t)(decay * -0.3535 * 32768.0f);

    rev->d1 = (int16_t)(decay * 0.3535 * 32768.0f);
    rev->d2 = (int16_t)(0.35 * 32768.0f);
}

void reverb_init(reverb_t *rev) {
    rev->ap_tap[0] = 612;
    rev->ap_tap[1] = 199;
    rev->ap_tap[2] = 113;

    float n = 1 / (5340 + 132300.0);
    float a0, b1;

    a0 = 2 * 5340 * n;
    b1 = (132300 - 5340) * n;

    rev->a0 = (int16_t)(a0 * 32768.0f);
    rev->b1 = (int16_t)(b1 * 32768.0f);

#ifdef LOW_QUALITY
    bzero(rev->comb, sizeof(int8_t) * COMB_SIZE * NUM_COMBS);
    bzero(rev->ap, sizeof(int8_t) * AP_SIZE * NUM_APS);
#else
    bzero(rev->comb, sizeof(int16_t) * COMB_SIZE * NUM_COMBS);
    bzero(rev->ap, sizeof(int16_t) * AP_SIZE * NUM_APS);
#endif

    reverb_set_colour(rev, 0.0f);
    reverb_set_size(rev, 0.1f);
    reverb_set_decay(rev, 0.0f);
}

void reverb_process(reverb_t *rev, int16_t *in, int16_t *out, unsigned int sample_count, unsigned int offset, unsigned int incr) {
    // handle the actual processing
    uint32_t pos;
    uint32_t comb_pos = rev->comb_pos;
    uint32_t ap_pos = rev->ap_pos;

    int c;

    int16_t * input = in;
    int16_t * output = out;

    int32_t in_s, in_s1, temp;

    /* loop around the buffer */
    for (pos = offset; pos < sample_count; pos+=incr) {
        /* loop around the comb filters */
        temp = 0;
        in_s = input[pos] / 3;

        rev->lpo = ((int32_t)rev->a0 * (int32_t)in_s + (int32_t)rev->b1 * (int32_t)rev->lpo) >> 15;
        in_s1 = (in_s << 15) + (int32_t)rev->gl * (int32_t)rev->lpo + (int32_t)rev->gh * (int32_t)(in_s - rev->lpo);

        for (c = 0; c < NUM_COMBS; c++) {
#ifdef LOW_QUALITY
            int32_t v = (int32_t)rev->comb[c][comb_pos] << 8;
            rev->comb[c][(comb_pos + rev->tap[c]) & COMB_MASK] = __SSAT((in_s1 + (int32_t)rev->comp_gain[c] * v) >> 23, 8);
#else
            int32_t v = (int32_t)rev->comb[c][comb_pos];
            rev->comb[c][(comb_pos + rev->tap[c]) & COMB_MASK] = __SSAT((in_s1 + (int32_t)rev->comp_gain[c] * v) >> 15, 16);
#endif
            temp = __SSAT(temp + v, 16);
        }

        /* loop around the allpass filters */
        for (c = 0; c < NUM_APS; c++) {
#ifdef LOW_QUALITY
            int32_t v = (int32_t)rev->ap[c][ap_pos] << 8;
            rev->ap[c][(ap_pos + rev->ap_tap[c]) & AP_MASK] = __SSAT(((temp << 15) + ((int32_t)rev->ap_gain * (int32_t)v)) >> 23, 8);
#else
            int32_t v = (int32_t)rev->ap[c][ap_pos];
            rev->ap[c][(ap_pos + rev->ap_tap[c]) & AP_MASK] = __SSAT(((temp << 15) + ((int32_t)rev->ap_gain * (int32_t)v)) >> 15, 16);
#endif
            temp = __SSAT((((int32_t)rev->d1 * temp) >> 15) + v, 16);
        }

        output[pos] = __SSAT((((int32_t)rev->d2 * temp) >> 15) + input[pos] * 2/3, 16);
        comb_pos++;
        comb_pos &= COMB_MASK; /* increment and wrap buffer */
        ap_pos++;
        ap_pos &= AP_MASK; /* increment and wrap buffer */
    }

    rev->comb_pos = comb_pos;
    rev->ap_pos = ap_pos;
}
#endif