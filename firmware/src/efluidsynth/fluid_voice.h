#ifndef _FLUID_VOICE_H
#define _FLUID_VOICE_H

#include "fluid_phase.h"
#include "fluid_gen.h"
#include "fluid_mod.h"

#define NO_CHANNEL             0xff

enum fluid_voice_status
{
	FLUID_VOICE_CLEAN,
	FLUID_VOICE_ON,
	FLUID_VOICE_SUSTAINED,
	FLUID_VOICE_OFF
};

/*
 * envelope data
 */
struct _fluid_env_data_t {
	uint32_t count;
	fluid_real_t coeff;
	fluid_real_t incr;
	fluid_real_t min;
	fluid_real_t max;
};

/* Indices for envelope tables */
enum fluid_voice_envelope_index_t {
	FLUID_VOICE_ENVDELAY,
	FLUID_VOICE_ENVATTACK,
	FLUID_VOICE_ENVHOLD,
	FLUID_VOICE_ENVDECAY,
	FLUID_VOICE_ENVSUSTAIN,
	FLUID_VOICE_ENVRELEASE,
	FLUID_VOICE_ENVFINISHED,
	FLUID_VOICE_ENVLAST
};

/*
 * fluid_voice_t
 */
struct _fluid_voice_t
{
	uint32_t id;                /* the id is incremented for every new noteon.
					   it's used for noteoff's  */
	uint8_t status;
	uint8_t chan;             /* the channel number, quick access for channel messages */
	uint8_t key;              /* the key, quick acces for noteoff */
	uint8_t vel;              /* the velocity */
	fluid_channel_t* channel;
	fluid_list_t *gen;
	fluid_list_t *mod;
	uint8_t mod_count;
	uint8_t has_looped;                 /* Flag that is set as soon as the first loop is completed. */
	fluid_sample_t* sample;

	uint8_t check_sample_sanity_flag;   /* Flag that initiates, that sample-related parameters
					   have to be checked. */

	/* basic parameters */
	fluid_real_t output_rate;        /* the sample rate of the synthesizer */

	uint32_t start_time;
	uint32_t ticks;

	fluid_buf_t amp;                /* current linear amplitude */
	fluid_phase_t phase;             /* the phase of the sample wave */

	/* Temporary variables used in fluid_voice_write() */

	fluid_phase_t phase_incr;	/* the phase increment for the next 64 samples */
	fluid_buf_t amp_incr;		/* amplitude increment value */
	fluid_buf_t *dsp_buf;		/* buffer to store interpolated sample data to */

	uint8_t is_looping;

	/* End temporary variables */

	/* basic parameters */
	fluid_real_t pitch;              /* the pitch in midicents */
	fluid_real_t attenuation;        /* the attenuation in centibels */
	fluid_real_t min_attenuation_cB; /* Estimate on the smallest possible attenuation
					  * during the lifetime of the voice */
	fluid_real_t root_pitch;

	/* sample and loop start and end points (offset in sample memory).  */
	uint32_t start;
	uint32_t end;
	uint32_t loopstart;
	uint32_t loopend;	/* Note: first point following the loop (superimposed on loopstart) */

	uint64_t loop_size;
	uint32_t end_index;

	/* master gain */
	fluid_real_t synth_gain;

	/* vol env */
	fluid_env_data_t volenv_data[FLUID_VOICE_ENVLAST];
	uint32_t volenv_count;
	uint8_t volenv_section;
	fluid_real_t volenv_val;
	fluid_real_t amplitude_that_reaches_noise_floor_nonloop;
	fluid_real_t amplitude_that_reaches_noise_floor_loop;

	/* mod env */
	fluid_env_data_t modenv_data[FLUID_VOICE_ENVLAST];
	uint32_t modenv_count;
	uint8_t modenv_section;
	fluid_real_t modenv_val;         /* the value of the modulation envelope */
	fluid_real_t modenv_to_fc;
	fluid_real_t modenv_to_pitch;

	/* mod lfo */
	fluid_real_t modlfo_val;          /* the value of the modulation LFO */
	uint32_t modlfo_delay;       /* the delay of the lfo in samples */
	fluid_real_t modlfo_incr;         /* the lfo frequency is converted to a per-buffer increment */
	fluid_real_t modlfo_to_fc;
	fluid_real_t modlfo_to_pitch;
	fluid_real_t modlfo_to_vol;

	/* vib lfo */
	fluid_real_t viblfo_val;        /* the value of the vibrato LFO */
	uint32_t viblfo_delay;      /* the delay of the lfo in samples */
	fluid_real_t viblfo_incr;       /* the lfo frequency is converted to a per-buffer increment */
	fluid_real_t viblfo_to_pitch;

	/* resonant filter */
	fluid_real_t fres;              /* the resonance frequency, in cents (not absolute cents) */
	fluid_real_t last_fres;         /* Current resonance frequency of the IIR filter */
	/* Serves as a flag: A deviation between fres and last_fres */
	/* indicates, that the filter has to be recalculated. */


	fluid_real_t q_lin;             /* the q-factor on a linear scale */
	fluid_real_t filter_gain;       /* Gain correction factor, depends on q */
	uint8_t filter_startup;             /* Flag: If set, the filter will be set directly.
					   Else it changes smoothly. */

#ifdef FLUID_SIMPLE_IIR
	float vibrapos;
	float vibraspeed;
	float c;
	float r;
#else
	fluid_buf_t hist1, hist2;      /* Sample history for the IIR filter */

	/* filter coefficients */
	/* The coefficients are normalized to a0. */
	/* b0 and b2 are identical => b02 */
	fluid_buf_t a1;              /* a0 / a0 */
	fluid_buf_t a2;              /* a1 / a0 */
	fluid_buf_t b1;              /* b1 / a0 */
	fluid_buf_t b02;              /* b0 / a0 */

	fluid_buf16_t b02_incr;
	fluid_buf16_t b1_incr;
	fluid_buf16_t a1_incr;
	fluid_buf16_t a2_incr;
	int filter_coeff_incr_count;
#endif

	/* pan */
	fluid_real_t pan;
	fluid_real_t amp_left;
	fluid_real_t amp_right;

	/* reverb */
	fluid_real_t reverb_send;
	fluid_real_t amp_reverb;

	/* chorus */
	fluid_real_t chorus_send;
	fluid_real_t amp_chorus;

	/* interpolation method, as in fluid_interp in fluidsynth.h */
	uint8_t interp_method;
};


fluid_voice_t* new_fluid_voice(fluid_real_t output_rate);
int delete_fluid_voice(fluid_voice_t* voice);

void fluid_voice_start(fluid_voice_t* voice);

int fluid_voice_write(fluid_voice_t* voice,
                      fluid_buf_t* left, fluid_buf_t* right,
                      fluid_buf_t* reverb_buf, fluid_buf_t* chorus_buf);

int fluid_voice_init(fluid_voice_t* voice, fluid_sample_t* sample,
                     fluid_channel_t* channel, int key, int vel,
                     unsigned int id, unsigned int time, fluid_real_t gain);

int fluid_voice_modulate(fluid_voice_t* voice, int cc, int ctrl);
int fluid_voice_modulate_all(fluid_voice_t* voice);

/** Set the NRPN value of a generator. */
int fluid_voice_set_param(fluid_voice_t* voice, uint8_t gen, fluid_real_t value, int abs);


/** Set the gain. */
int fluid_voice_set_gain(fluid_voice_t* voice, fluid_real_t gain);

/** Update all the synthesis parameters, which depend on generator
    'gen'. This is only necessary after changing a generator of an
    already operating voice.  Most applications will not need this
    function.*/
void fluid_voice_update_param(fluid_voice_t* voice, uint8_t gen);

int fluid_voice_noteoff(fluid_voice_t* voice);
int fluid_voice_off(fluid_voice_t* voice);
int fluid_voice_calculate_runtime_synthesis_parameters(fluid_voice_t* voice);
fluid_channel_t* fluid_voice_get_channel(fluid_voice_t* voice);
int calculate_hold_decay_buffers(fluid_voice_t* voice, int gen_base,
                                 int gen_key2base, int is_decay);
int fluid_voice_kill_excl(fluid_voice_t* voice);
fluid_real_t fluid_voice_get_lower_boundary_for_attenuation(fluid_voice_t* voice);
fluid_real_t fluid_voice_determine_amplitude_that_reaches_noise_floor_for_sample(fluid_voice_t* voice);
void fluid_voice_check_sample_sanity(fluid_voice_t* voice);

#define fluid_voice_set_id(_voice, _id)  { (_voice)->id = (_id); }
#define fluid_voice_get_chan(_voice)     (_voice)->chan


#define _PLAYING(voice)  (((voice)->status == FLUID_VOICE_ON) || ((voice)->status == FLUID_VOICE_SUSTAINED))

/* A voice is 'ON', if it has not yet received a noteoff
 * event. Sending a noteoff event will advance the envelopes to
 * section 5 (release). */
#define _ON(voice)  ((voice)->status == FLUID_VOICE_ON && (voice)->volenv_section < FLUID_VOICE_ENVRELEASE)
#define _SUSTAINED(voice)  ((voice)->status == FLUID_VOICE_SUSTAINED)
#define _AVAILABLE(voice)  (((voice)->status == FLUID_VOICE_CLEAN) || ((voice)->status == FLUID_VOICE_OFF))
#define _RELEASED(voice)  ((voice)->chan == NO_CHANNEL)
 #define _SAMPLEMODE(voice) \
	((fluid_voice_gen_val_or_default(voice, GEN_SAMPLEMODE)))

fluid_real_t fluid_voice_gen_value(fluid_voice_t* voice, int num);

#define _GEN(_voice, _n) \
	(fluid_voice_gen_val_all_or_default(_voice,_n))

#define FLUID_SAMPLESANITY_CHECK (1 << 0)
#define FLUID_SAMPLESANITY_STARTUP (1 << 1)

/* defined in fluid_dsp_float.c */

void fluid_dsp_float_config (void);

/*
 *  The interface to the synthesizer's voices
 *  Examples on using them can be found in fluid_defsfont.c
 */

/* for fluid_voice_add_mod */
enum fluid_voice_add_mod {
	FLUID_VOICE_OVERWRITE,
	FLUID_VOICE_ADD,
	FLUID_VOICE_DEFAULT
};

fluid_gen_t *fluid_voice_gen_get_or_add(fluid_voice_t *voice, uint8_t num);
fluid_real_t fluid_voice_gen_val_or_default(fluid_voice_t *voice, uint8_t num);
fluid_real_t fluid_voice_gen_val_all_or_default(fluid_voice_t *voice, uint8_t num);

/* Add a modulator to a voice (SF2.1 only). */
void fluid_voice_add_mod(fluid_voice_t* voice, fluid_mod_t* mod, int mode);

/** Set the value of a generator */
void fluid_voice_gen_set(fluid_voice_t* voice, int gen, float val);

/** Get the value of a generator */
float fluid_voice_gen_get(fluid_voice_t* voice, int gen);

/** Modify the value of a generator by val */
void fluid_voice_gen_incr(fluid_voice_t* voice, int gen, float val);


/** Return the unique ID of the noteon-event. A sound font loader
 *  may store the voice processes it has created for * real-time
 *  control during the operation of a voice (for example: parameter
 *  changes in sound font editor). The synth uses a pool of
 *  voices, which are 'recycled' and never deallocated.
 *
 * Before modifying an existing voice, check
 * - that its state is still 'playing'
 * - that the ID is still the same
 * Otherwise the voice has finished playing.
 */
unsigned int fluid_voice_get_id(fluid_voice_t* voice);

int fluid_voice_is_playing(fluid_voice_t* voice);

#endif /* _FLUID_VOICE_H */
