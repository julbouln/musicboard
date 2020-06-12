/*

  Freeverb

  Written by Jezar at Dreampoint, June 2000
  http://www.dreampoint.co.uk
  This code is public domain

  Translated to C by Peter Hanappe, Mai 2001
*/

#include "fluid_rev.h"

/***************************************************************
 *
 *                           REVERB
 */

/* Denormalising:
 *
 * According to music-dsp thread 'Denormalise', Pentium processors
 * have a hardware 'feature', that is of interest here, related to
 * numeric underflow.  We have a recursive filter. The output decays
 * exponentially, if the input stops.  So the numbers get smaller and
 * smaller... At some point, they reach 'denormal' level.  This will
 * lead to drastic spikes in the CPU load.  The effect was reproduced
 * with the reverb - sometimes the average load over 10 s doubles!!.
 *
 * The 'undenormalise' macro fixes the problem: As soon as the number
 * is close enough to denormal level, the macro forces the number to
 * 0.0f.  The original macro is:
 *
 * #define undenormalise(sample) if(((*(unsigned int*)&sample)&0x7f800000)==0) sample=0.0f
 *
 * This will zero out a number when it reaches the denormal level.
 * Advantage: Maximum dynamic range Disadvantage: We'll have to check
 * every sample, expensive.  The alternative macro comes from a later
 * mail from Jon Watte. It will zap a number before it reaches
 * denormal level. Jon suggests to run it once per block instead of
 * every sample.
 */

# if defined(WITH_FLOATX)
# define zap_almost_zero(sample) (((*(unsigned int*)&(sample))&0x7f800000) < 0x08000000)?0.0f:(sample)
# else
/* 1e-20 was chosen as an arbitrary (small) threshold. */
#define zap_almost_zero(sample) FLUID_ABS(sample)<1e-10 ? 0 : sample;
#endif

/* Denormalising part II:
 *
 * Another method fixes the problem cheaper: Use a small DC-offset in
 * the filter calculations.  Now the signals converge not against 0,
 * but against the offset.  The constant offset is invisible from the
 * outside world (i.e. it does not appear at the output.  There is a
 * very small turn-on transient response, which should not cause
 * problems.
 */

//#define DC_OFFSET 0
#ifdef FLUID_FIXED_POINT
#define DC_OFFSET 3
#else
#define DC_OFFSET 1e-8
#endif
//#define DC_OFFSET 0.001f
typedef struct _fluid_allpass fluid_allpass;
typedef struct _fluid_comb fluid_comb;

struct _fluid_allpass {
  fluid_buf_t feedback;
  fluid_buf_t *buffer;

  int bufsize;
  int bufidx;
};

void fluid_allpass_setbuffer(fluid_allpass* allpass, fluid_buf_t *buf, int size);
void fluid_allpass_init(fluid_allpass* allpass);
void fluid_allpass_setfeedback(fluid_allpass* allpass, fluid_buf_t val);
fluid_buf_t fluid_allpass_getfeedback(fluid_allpass* allpass);

void fluid_allpass_setbuffer(fluid_allpass* allpass, fluid_buf_t *buf, int size)
{
  allpass->bufidx = 0;
  allpass->buffer = buf;
  allpass->bufsize = size;
}

void fluid_allpass_init(fluid_allpass* allpass)
{
  int i;
  int len = allpass->bufsize;
  fluid_buf_t* buf = allpass->buffer;
  for (i = 0; i < len; i++) {
    buf[i] = DC_OFFSET; /* this is not 100 % correct. */
  }
}

void fluid_allpass_setfeedback(fluid_allpass* allpass, fluid_buf_t val)
{
  allpass->feedback = val;
}

fluid_buf_t fluid_allpass_getfeedback(fluid_allpass* allpass)
{
  return allpass->feedback;
}

#define fluid_allpass_process(_allpass, _input) \
{ \
  fluid_buf_t output; \
  fluid_buf_t bufout; \
  bufout = _allpass.buffer[_allpass.bufidx]; \
  output = bufout-_input; \
  _allpass.buffer[_allpass.bufidx] = _input + FLUID_BUF_MULT32(bufout,_allpass.feedback); \
  if (++_allpass.bufidx >= _allpass.bufsize) { \
    _allpass.bufidx = 0; \
  } \
  _input = output; \
}

struct _fluid_comb {
  fluid_buf_t feedback;
  fluid_buf_t filterstore;
  fluid_buf_t damp1;
  fluid_buf_t damp2;
  fluid_buf_t *buffer;

  int bufsize;
  int bufidx;
};

void fluid_comb_setbuffer(fluid_comb* comb, fluid_buf_t *buf, int size);
void fluid_comb_init(fluid_comb* comb);
void fluid_comb_setdamp(fluid_comb* comb, fluid_buf_t val);
fluid_buf_t fluid_comb_getdamp(fluid_comb* comb);
void fluid_comb_setfeedback(fluid_comb* comb, fluid_buf_t val);
fluid_buf_t fluid_comb_getfeedback(fluid_comb* comb);

void fluid_comb_setbuffer(fluid_comb* comb, fluid_buf_t *buf, int size)
{
  comb->filterstore = 0;
  comb->bufidx = 0;
  comb->buffer = buf;
  comb->bufsize = size;
}

void fluid_comb_init(fluid_comb* comb)
{
  int i;
  fluid_buf_t* buf = comb->buffer;
  int len = comb->bufsize;
  for (i = 0; i < len; i++) {
    buf[i] = DC_OFFSET; /* This is not 100 % correct. */
  }
}

void fluid_comb_setdamp(fluid_comb* comb, fluid_buf_t val)
{
  comb->damp1 = val;
  #ifdef FLUID_FIXED_POINT
  comb->damp2 = 32768 - val;
  #else
  comb->damp2 = 1 - val;
  #endif
}

fluid_buf_t fluid_comb_getdamp(fluid_comb* comb)
{
  return comb->damp1;
}

void fluid_comb_setfeedback(fluid_comb* comb, fluid_buf_t val)
{
  comb->feedback = val;
}

fluid_buf_t fluid_comb_getfeedback(fluid_comb* comb)
{
  return comb->feedback;
}

#define fluid_comb_process(_comb, _input, _output) \
{ \
  fluid_buf_t _tmp = _comb.buffer[_comb.bufidx]; \
  _comb.filterstore = FLUID_BUF_MULT32(_tmp,_comb.damp2) + FLUID_BUF_MULT32(_comb.filterstore,_comb.damp1); \
  _comb.buffer[_comb.bufidx] = _input + FLUID_BUF_MULT32(_comb.filterstore,_comb.feedback); \
  if (++_comb.bufidx >= _comb.bufsize) { \
    _comb.bufidx = 0; \
  } \
  _output += _tmp; \
}

#define NUMCOMBS 8
#define NUMALLPASSES 4

#ifdef FLUID_FIXED_POINT
#define FIXEDGAIN 256
#else
#define	FIXEDGAIN 0.015f
#endif
#define scalewet 3.0f
#define scaledamp 1.0f
#define scaleroom 0.28f
#define offsetroom 0.7f
#define initialroom 0.5f
#define initialdamp 0.2f
#define initialwet 1
#define initialdry 0
#define initialwidth 1
#define stereospread 23

/*
 These values assume 44.1KHz sample rate
 they will probably be OK for 48KHz sample rate
 but would need scaling for 96KHz (or other) sample rates.
 The values were obtained by listening tests.
*/
#define combtuningL1 1116
#define combtuningR1 1116 + stereospread
#define combtuningL2 1188
#define combtuningR2 1188 + stereospread
#define combtuningL3 1277
#define combtuningR3 1277 + stereospread
#define combtuningL4 1356
#define combtuningR4 1356 + stereospread
#define combtuningL5 1422
#define combtuningR5 1422 + stereospread
#define combtuningL6 1491
#define combtuningR6 1491 + stereospread
#define combtuningL7 1557
#define combtuningR7 1557 + stereospread
#define combtuningL8 1617
#define combtuningR8 1617 + stereospread
#define allpasstuningL1 556
#define allpasstuningR1 556 + stereospread
#define allpasstuningL2 441
#define allpasstuningR2 441 + stereospread
#define allpasstuningL3 341
#define allpasstuningR3 341 + stereospread
#define allpasstuningL4 225
#define allpasstuningR4 225 + stereospread

struct _fluid_revmodel_t {
  fluid_real_t roomsize;
  fluid_real_t damp;
  fluid_real_t wet;
  fluid_buf_t wet1, wet2;
  fluid_real_t width;
  fluid_buf16_t gain;

  /*
   The following are all declared inline
   to remove the need for dynamic allocation
   with its subsequent error-checking messiness
  */
  /* Comb filters */
  fluid_comb combL[NUMCOMBS];
  fluid_comb combR[NUMCOMBS];
  /* Allpass filters */
  fluid_allpass allpassL[NUMALLPASSES];
  fluid_allpass allpassR[NUMALLPASSES];
  /* Buffers for the combs */

  fluid_buf_t bufcombL1[combtuningL1];
  fluid_buf_t bufcombR1[combtuningR1];
  fluid_buf_t bufcombL2[combtuningL2];
  fluid_buf_t bufcombR2[combtuningR2];
  fluid_buf_t bufcombL3[combtuningL3];
  fluid_buf_t bufcombR3[combtuningR3];
  fluid_buf_t bufcombL4[combtuningL4];
  fluid_buf_t bufcombR4[combtuningR4];
  fluid_buf_t bufcombL5[combtuningL5];
  fluid_buf_t bufcombR5[combtuningR5];
  fluid_buf_t bufcombL6[combtuningL6];
  fluid_buf_t bufcombR6[combtuningR6];
  fluid_buf_t bufcombL7[combtuningL7];
  fluid_buf_t bufcombR7[combtuningR7];
  fluid_buf_t bufcombL8[combtuningL8];
  fluid_buf_t bufcombR8[combtuningR8];
  /* Buffers for the allpasses */
  fluid_buf_t bufallpassL1[allpasstuningL1];
  fluid_buf_t bufallpassR1[allpasstuningR1];
  fluid_buf_t bufallpassL2[allpasstuningL2];
  fluid_buf_t bufallpassR2[allpasstuningR2];
  fluid_buf_t bufallpassL3[allpasstuningL3];
  fluid_buf_t bufallpassR3[allpasstuningR3];
  fluid_buf_t bufallpassL4[allpasstuningL4];
  fluid_buf_t bufallpassR4[allpasstuningR4];

};

void fluid_revmodel_update(fluid_revmodel_t* rev);
void fluid_revmodel_init(fluid_revmodel_t* rev);

fluid_revmodel_t* new_fluid_revmodel()
{
  fluid_revmodel_t* rev;
  rev = FLUID_NEW(fluid_revmodel_t);
  if (rev == NULL) {
    return NULL;
  }

  /* Tie the components to their buffers */
  fluid_comb_setbuffer(&rev->combL[0], rev->bufcombL1, combtuningL1);
  fluid_comb_setbuffer(&rev->combR[0], rev->bufcombR1, combtuningR1);
  fluid_comb_setbuffer(&rev->combL[1], rev->bufcombL2, combtuningL2);
  fluid_comb_setbuffer(&rev->combR[1], rev->bufcombR2, combtuningR2);
  fluid_comb_setbuffer(&rev->combL[2], rev->bufcombL3, combtuningL3);
  fluid_comb_setbuffer(&rev->combR[2], rev->bufcombR3, combtuningR3);
  fluid_comb_setbuffer(&rev->combL[3], rev->bufcombL4, combtuningL4);
  fluid_comb_setbuffer(&rev->combR[3], rev->bufcombR4, combtuningR4);
  fluid_comb_setbuffer(&rev->combL[4], rev->bufcombL5, combtuningL5);
  fluid_comb_setbuffer(&rev->combR[4], rev->bufcombR5, combtuningR5);
  fluid_comb_setbuffer(&rev->combL[5], rev->bufcombL6, combtuningL6);
  fluid_comb_setbuffer(&rev->combR[5], rev->bufcombR6, combtuningR6);
  fluid_comb_setbuffer(&rev->combL[6], rev->bufcombL7, combtuningL7);
  fluid_comb_setbuffer(&rev->combR[6], rev->bufcombR7, combtuningR7);
  fluid_comb_setbuffer(&rev->combL[7], rev->bufcombL8, combtuningL8);
  fluid_comb_setbuffer(&rev->combR[7], rev->bufcombR8, combtuningR8);
  fluid_allpass_setbuffer(&rev->allpassL[0], rev->bufallpassL1, allpasstuningL1);
  fluid_allpass_setbuffer(&rev->allpassR[0], rev->bufallpassR1, allpasstuningR1);
  fluid_allpass_setbuffer(&rev->allpassL[1], rev->bufallpassL2, allpasstuningL2);
  fluid_allpass_setbuffer(&rev->allpassR[1], rev->bufallpassR2, allpasstuningR2);
  fluid_allpass_setbuffer(&rev->allpassL[2], rev->bufallpassL3, allpasstuningL3);
  fluid_allpass_setbuffer(&rev->allpassR[2], rev->bufallpassR3, allpasstuningR3);
  fluid_allpass_setbuffer(&rev->allpassL[3], rev->bufallpassL4, allpasstuningL4);
  fluid_allpass_setbuffer(&rev->allpassR[3], rev->bufallpassR4, allpasstuningR4);

  fluid_buf_t default_feedback = FLUID_REAL_TO_FRAC(0.5f);

  /* Set default values */
  fluid_allpass_setfeedback(&rev->allpassL[0], default_feedback);
  fluid_allpass_setfeedback(&rev->allpassR[0], default_feedback);
  fluid_allpass_setfeedback(&rev->allpassL[1], default_feedback);
  fluid_allpass_setfeedback(&rev->allpassR[1], default_feedback);
  fluid_allpass_setfeedback(&rev->allpassL[2], default_feedback);
  fluid_allpass_setfeedback(&rev->allpassR[2], default_feedback);
  fluid_allpass_setfeedback(&rev->allpassL[3], default_feedback);
  fluid_allpass_setfeedback(&rev->allpassR[3], default_feedback);

  /* set values manually, since calling set functions causes update
     and all values should be initialized for an update */
  rev->roomsize = initialroom * scaleroom + offsetroom;
  rev->damp = initialdamp * scaledamp;
  rev->wet = initialwet * scalewet;
  rev->width = initialwidth;
  rev->gain = FIXEDGAIN;

  /* now its okay to update reverb */
  fluid_revmodel_update(rev);

  /* Clear all buffers */
  fluid_revmodel_init(rev);
  return rev;
}

void delete_fluid_revmodel(fluid_revmodel_t* rev)
{
  FLUID_FREE(rev);
}

void fluid_revmodel_init(fluid_revmodel_t* rev)
{
  int i;
  for (i = 0; i < NUMCOMBS;i++) {
    fluid_comb_init(&rev->combL[i]);
    fluid_comb_init(&rev->combR[i]);
  }
  for (i = 0; i < NUMALLPASSES; i++) {
    fluid_allpass_init(&rev->allpassL[i]);
    fluid_allpass_init(&rev->allpassR[i]);
  }
}

void fluid_revmodel_reset(fluid_revmodel_t* rev)
{
  fluid_revmodel_init(rev);
}

void fluid_revmodel_processmix(fluid_revmodel_t* rev, fluid_buf_t *in,
			 fluid_buf_t *left_out, fluid_buf_t *right_out)
{
  int i, k = 0;
  fluid_buf_t outL, outR, input;

#if 0
  fluid_buf_t in0,in1,in2,in3;
  fluid_buf_t outL0,outL1,outL2,outL3;
  fluid_buf_t outR0,outR1,outR2,outR3;

  uint32_t dsp_cnt;

  dsp_cnt = FLUID_BUFSIZE >> 2;

  while(dsp_cnt > 0) {

    outL0 = outL1 = outL2 = outL3 = 0;
    outR0 = outR1 = outR2 = outR3 = 0;

    in0 = FLUID_BUF_MULT(2*rev->gain, *(in++) ) + DC_OFFSET;
    in1 = FLUID_BUF_MULT(2*rev->gain, *(in++) ) + DC_OFFSET;
    in2 = FLUID_BUF_MULT(2*rev->gain, *(in++) ) + DC_OFFSET;
    in3 = FLUID_BUF_MULT(2*rev->gain, *(in++) ) + DC_OFFSET;


    /* Accumulate comb filters in parallel */
    for (i = 0; i < NUMCOMBS; i++) {
      fluid_comb_process(rev->combL[i], in0, outL0);
      fluid_comb_process(rev->combR[i], in0, outR0);
    }
    for (i = 0; i < NUMCOMBS; i++) {
      fluid_comb_process(rev->combL[i], in1, outL1);
      fluid_comb_process(rev->combR[i], in1, outR1);
    }
    for (i = 0; i < NUMCOMBS; i++) {
      fluid_comb_process(rev->combL[i], in2, outL2);
      fluid_comb_process(rev->combR[i], in2, outR2);
    }
    for (i = 0; i < NUMCOMBS; i++) {
      fluid_comb_process(rev->combL[i], in3, outL3);
      fluid_comb_process(rev->combR[i], in3, outR3);
    }

    /* Feed through allpasses in series */
//    for (i = 0; i < NUMALLPASSES; i++) {
    i=0;
      fluid_allpass_process(rev->allpassL[i], outL0);
      fluid_allpass_process(rev->allpassR[i], outR0);
      fluid_allpass_process(rev->allpassL[i+1], outL0);
      fluid_allpass_process(rev->allpassR[i+1], outR0);
      fluid_allpass_process(rev->allpassL[i+2], outL0);
      fluid_allpass_process(rev->allpassR[i+2], outR0);
      fluid_allpass_process(rev->allpassL[i+3], outL0);
      fluid_allpass_process(rev->allpassR[i+3], outR0);

      fluid_allpass_process(rev->allpassL[i], outL1);
      fluid_allpass_process(rev->allpassR[i], outR1);
      fluid_allpass_process(rev->allpassL[i+1], outL1);
      fluid_allpass_process(rev->allpassR[i+1], outR1);
      fluid_allpass_process(rev->allpassL[i+2], outL1);
      fluid_allpass_process(rev->allpassR[i+2], outR1);
      fluid_allpass_process(rev->allpassL[i+3], outL1);
      fluid_allpass_process(rev->allpassR[i+3], outR1);

      fluid_allpass_process(rev->allpassL[i], outL2);
      fluid_allpass_process(rev->allpassR[i], outR2);
      fluid_allpass_process(rev->allpassL[i+1], outL2);
      fluid_allpass_process(rev->allpassR[i+1], outR2);
      fluid_allpass_process(rev->allpassL[i+2], outL2);
      fluid_allpass_process(rev->allpassR[i+2], outR2);
      fluid_allpass_process(rev->allpassL[i+3], outL2);
      fluid_allpass_process(rev->allpassR[i+3], outR2);

      fluid_allpass_process(rev->allpassL[i], outL3);
      fluid_allpass_process(rev->allpassR[i], outR3);
      fluid_allpass_process(rev->allpassL[i+1], outL3);
      fluid_allpass_process(rev->allpassR[i+1], outR3);
      fluid_allpass_process(rev->allpassL[i+2], outL3);
      fluid_allpass_process(rev->allpassR[i+2], outR3);
      fluid_allpass_process(rev->allpassL[i+3], outL3);
      fluid_allpass_process(rev->allpassR[i+3], outR3);
//    }

/* Remove the DC offset */
    outL0 -= DC_OFFSET;
    outR0 -= DC_OFFSET;

    outL1 -= DC_OFFSET;
    outR1 -= DC_OFFSET;

    outL2 -= DC_OFFSET;
    outR2 -= DC_OFFSET;

    outL3 -= DC_OFFSET;
    outR3 -= DC_OFFSET;

    /* Calculate output MIXING with anything already there */
    *(left_out++) += FLUID_BUF_MULT32(rev->wet1,outL0) + FLUID_BUF_MULT32(rev->wet2,outR0);
    *(right_out++) += FLUID_BUF_MULT32(rev->wet1,outR0) + FLUID_BUF_MULT32(rev->wet2,outL0);

    *(left_out++) += FLUID_BUF_MULT32(rev->wet1,outL1) + FLUID_BUF_MULT32(rev->wet2,outR1);
    *(right_out++) += FLUID_BUF_MULT32(rev->wet1,outR1) + FLUID_BUF_MULT32(rev->wet2,outL1);

    *(left_out++) += FLUID_BUF_MULT32(rev->wet1,outL2) + FLUID_BUF_MULT32(rev->wet2,outR2);
    *(right_out++) += FLUID_BUF_MULT32(rev->wet1,outR2) + FLUID_BUF_MULT32(rev->wet2,outL2);

    *(left_out++) += FLUID_BUF_MULT32(rev->wet1,outL3) + FLUID_BUF_MULT32(rev->wet2,outR3);
    *(right_out++) += FLUID_BUF_MULT32(rev->wet1,outR3) + FLUID_BUF_MULT32(rev->wet2,outL3);

    dsp_cnt--;
  } // 3 * 64 + 36 * 64 = 2496 loop penalty (12.48us@200mhz)
#endif
#if 1
  for (k = 0; k < FLUID_BUFSIZE; k++) {
    outL = outR = 0;

    /* The original Freeverb code expects a stereo signal and 'input'
     * is set to the sum of the left and right input sample. Since
     * this code works on a mono signal, 'input' is set to twice the
     * input sample. */
    input = FLUID_BUF_MULT(2*rev->gain, *(in++) ) + DC_OFFSET;

    /* Accumulate comb filters in parallel */
    for (i = 0; i < NUMCOMBS; i++) {
      fluid_comb_process(rev->combL[i], input, outL);
      fluid_comb_process(rev->combR[i], input, outR);
    } // 3*8 = 24 loop penalty

    /* Feed through allpasses in series */
    for (i = 0; i < NUMALLPASSES; i++) {
      fluid_allpass_process(rev->allpassL[i], outL);
      fluid_allpass_process(rev->allpassR[i], outR);
    } // 3*4 = 12 loop penalty

    /* Remove the DC offset */
    outL -= DC_OFFSET;
    outR -= DC_OFFSET;

    /* Calculate output MIXING with anything already there */
    *(left_out++) += FLUID_BUF_MULT32(rev->wet1,outL) + FLUID_BUF_MULT32(rev->wet2,outR);
    *(right_out++) += FLUID_BUF_MULT32(rev->wet1,outR) + FLUID_BUF_MULT32(rev->wet2,outL);

  } // 256 * 3 + 256 * 36 = 9984 loop cost (49.92us@200mhz)
  #endif
}

void fluid_revmodel_update(fluid_revmodel_t* rev)
{
  /* Recalculate internal values after parameter change */
  int i;

  rev->wet1 = FLUID_REAL_TO_FRAC(rev->wet * (rev->width / 2 + 0.5f));
  rev->wet2 = FLUID_REAL_TO_FRAC(rev->wet * ((1 - rev->width) / 2));

  for (i = 0; i < NUMCOMBS; i++) {
    fluid_comb_setfeedback(&rev->combL[i], FLUID_REAL_TO_FRAC(rev->roomsize));
    fluid_comb_setfeedback(&rev->combR[i], FLUID_REAL_TO_FRAC(rev->roomsize));
  }

  for (i = 0; i < NUMCOMBS; i++) {
    fluid_comb_setdamp(&rev->combL[i], FLUID_REAL_TO_FRAC(rev->damp));
    fluid_comb_setdamp(&rev->combR[i], FLUID_REAL_TO_FRAC(rev->damp));
  }
}

/*
 The following get/set functions are not inlined, because
 speed is never an issue when calling them, and also
 because as you develop the reverb model, you may
 wish to take dynamic action when they are called.
*/
void fluid_revmodel_setroomsize(fluid_revmodel_t* rev, fluid_real_t value)
{
/*   fluid_clip(value, 0.0f, 1.0f); */
  rev->roomsize = (value * scaleroom) + offsetroom;
  fluid_revmodel_update(rev);
}

fluid_real_t fluid_revmodel_getroomsize(fluid_revmodel_t* rev)
{
  return (rev->roomsize - offsetroom) / scaleroom;
}

void fluid_revmodel_setdamp(fluid_revmodel_t* rev, fluid_real_t value)
{
/*   fluid_clip(value, 0.0f, 1.0f); */
  rev->damp = value * scaledamp;
  fluid_revmodel_update(rev);
}

fluid_real_t fluid_revmodel_getdamp(fluid_revmodel_t* rev)
{
  return rev->damp / scaledamp;
}

void fluid_revmodel_setlevel(fluid_revmodel_t* rev, fluid_real_t value)
{
	fluid_clip(value, 0.0f, 1.0f);
	rev->wet = value * scalewet;
	fluid_revmodel_update(rev);
}

fluid_real_t fluid_revmodel_getlevel(fluid_revmodel_t* rev)
{
  return rev->wet / scalewet;
}

void fluid_revmodel_setwidth(fluid_revmodel_t* rev, fluid_real_t value)
{
/*   fluid_clip(value, 0.0f, 1.0f); */
  rev->width = value;
  fluid_revmodel_update(rev);
}

fluid_real_t fluid_revmodel_getwidth(fluid_revmodel_t* rev)
{
  return rev->width;
}
