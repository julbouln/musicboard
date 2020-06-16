#include "fluid_synth.h"
#include "fluid_sys.h"
#include "fluid_chan.h"
#include "fluid_tuning.h"
#include "fluid_settings.h"
#include "fluid_sfont.h"
#include "fluid_altsfont.h"
#include "fluid_log.h"
#include "fluid_version.h"

/************************************************************************
 *
 * These functions were added after the v1.0 API freeze. They are not
 * in synth.h. They should be added as soon as a new development
 * version is started.
 *
 ************************************************************************/

int fluid_synth_program_select2(fluid_synth_t* synth,
                                int chan,
                                char* sfont_name,
                                unsigned int bank_num,
                                unsigned int preset_num);

fluid_sfont_t* fluid_synth_get_sfont_by_name(fluid_synth_t* synth, char *name);

int fluid_synth_set_gen2(fluid_synth_t* synth, int chan,
                         int param, float value,
                         int absolute, int normalized);


/***************************************************************
 *
 *                         GLOBAL
 */

/* has the synth module been initialized? */
static int fluid_synth_initialized = 0;
static void fluid_synth_init(void);

/* default modulators
 * SF2.01 page 52 ff:
 *
 * There is a set of predefined default modulators. They have to be
 * explicitly overridden by the sound font in order to turn them off.
 */

fluid_mod_t default_vel2att_mod;        /* SF2.01 section 8.4.1  */
fluid_mod_t default_vel2filter_mod;     /* SF2.01 section 8.4.2  */
fluid_mod_t default_at2viblfo_mod;      /* SF2.01 section 8.4.3  */
fluid_mod_t default_mod2viblfo_mod;     /* SF2.01 section 8.4.4  */
fluid_mod_t default_att_mod;            /* SF2.01 section 8.4.5  */
fluid_mod_t default_pan_mod;            /* SF2.01 section 8.4.6  */
fluid_mod_t default_expr_mod;           /* SF2.01 section 8.4.7  */
fluid_mod_t default_reverb_mod;         /* SF2.01 section 8.4.8  */
fluid_mod_t default_chorus_mod;         /* SF2.01 section 8.4.9  */
fluid_mod_t default_pitch_bend_mod;     /* SF2.01 section 8.4.10 */

/* reverb presets */
static fluid_revmodel_presets_t revmodel_preset[] = {
  /* name */    /* roomsize */ /* damp */ /* width */ /* level */
  { "Test 1",          0.2f,      0.0f,       0.5f,       0.9f },
  { "Test 2",          0.4f,      0.2f,       0.5f,       0.8f },
  { "Test 3",          0.6f,      0.4f,       0.5f,       0.7f },
  { "Test 4",          0.8f,      0.7f,       0.5f,       0.6f },
  { "Test 5",          0.8f,      1.0f,       0.5f,       0.5f },
  { NULL, 0.0f, 0.0f, 0.0f, 0.0f }
};


/***************************************************************
 *
 *               INITIALIZATION & UTILITIES
 */


void fluid_synth_settings(fluid_settings_t* settings)
{
  settings->verbose = 0;
  settings->dump = 0;
  settings->reverb = 0;
  settings->chorus = 0;
  settings->polyphony = 256;
  settings->midi_channels = 16;
  settings->gain = 0.2f;
  settings->audio_channels = 1;
  settings->audio_groups = 1;
  settings->effects_channels = 2;
  settings->sample_rate = 44100.0f;
}

/*
 * fluid_version
 */
void fluid_version(int *major, int *minor, int *micro)
{
  *major = FLUIDSYNTH_VERSION_MAJOR;
  *minor = FLUIDSYNTH_VERSION_MINOR;
  *micro = FLUIDSYNTH_VERSION_MICRO;
}

/*
 * fluid_version_str
 */
char* fluid_version_str(void)
{
  return FLUIDSYNTH_VERSION;
}

/*
 * void fluid_synth_init
 *
 * Does all the initialization for this module.
 */
static void
fluid_synth_init()
{
  fluid_synth_initialized++;

  fluid_conversion_config();

  fluid_dsp_float_config();

  fluid_sys_config();

  /* SF2.01 page 53 section 8.4.1: MIDI Note-On Velocity to Initial Attenuation */
  fluid_mod_set_source1(&default_vel2att_mod, /* The modulator we are programming here */
                        FLUID_MOD_VELOCITY,    /* Source. VELOCITY corresponds to 'index=2'. */
                        FLUID_MOD_GC           /* Not a MIDI continuous controller */
                        | FLUID_MOD_CONCAVE    /* Curve shape. Corresponds to 'type=1' */
                        | FLUID_MOD_UNIPOLAR   /* Polarity. Corresponds to 'P=0' */
                        | FLUID_MOD_NEGATIVE   /* Direction. Corresponds to 'D=1' */
                       );
  fluid_mod_set_source2(&default_vel2att_mod, 0, 0); /* No 2nd source */
  fluid_mod_set_dest(&default_vel2att_mod, GEN_ATTENUATION);  /* Target: Initial attenuation */
  fluid_mod_set_amount(&default_vel2att_mod, 960.0);          /* Modulation amount: 960 */

  /* SF2.01 page 53 section 8.4.2: MIDI Note-On Velocity to Filter Cutoff
   * Have to make a design decision here. The specs don't make any sense this way or another.
   * One sound font, 'Kingston Piano', which has been praised for its quality, tries to
   * override this modulator with an amount of 0 and positive polarity (instead of what
   * the specs say, D=1) for the secondary source.
   * So if we change the polarity to 'positive', one of the best free sound fonts works...
   */
  fluid_mod_set_source1(&default_vel2filter_mod, FLUID_MOD_VELOCITY, /* Index=2 */
                        FLUID_MOD_GC                        /* CC=0 */
                        | FLUID_MOD_LINEAR                  /* type=0 */
                        | FLUID_MOD_UNIPOLAR                /* P=0 */
                        | FLUID_MOD_NEGATIVE                /* D=1 */
                       );
  fluid_mod_set_source2(&default_vel2filter_mod, FLUID_MOD_VELOCITY, /* Index=2 */
                        FLUID_MOD_GC                                 /* CC=0 */
                        | FLUID_MOD_SWITCH                           /* type=3 */
                        | FLUID_MOD_UNIPOLAR                         /* P=0 */
                        // do not remove       | FLUID_MOD_NEGATIVE                         /* D=1 */
                        | FLUID_MOD_POSITIVE                         /* D=0 */
                       );
  fluid_mod_set_dest(&default_vel2filter_mod, GEN_FILTERFC);        /* Target: Initial filter cutoff */
  fluid_mod_set_amount(&default_vel2filter_mod, -2400);

  /* SF2.01 page 53 section 8.4.3: MIDI Channel pressure to Vibrato LFO pitch depth */
  fluid_mod_set_source1(&default_at2viblfo_mod, FLUID_MOD_CHANNELPRESSURE, /* Index=13 */
                        FLUID_MOD_GC                        /* CC=0 */
                        | FLUID_MOD_LINEAR                  /* type=0 */
                        | FLUID_MOD_UNIPOLAR                /* P=0 */
                        | FLUID_MOD_POSITIVE                /* D=0 */
                       );
  fluid_mod_set_source2(&default_at2viblfo_mod, 0, 0); /* no second source */
  fluid_mod_set_dest(&default_at2viblfo_mod, GEN_VIBLFOTOPITCH);        /* Target: Vib. LFO => pitch */
  fluid_mod_set_amount(&default_at2viblfo_mod, 50);

  /* SF2.01 page 53 section 8.4.4: Mod wheel (Controller 1) to Vibrato LFO pitch depth */
  fluid_mod_set_source1(&default_mod2viblfo_mod, 1, /* Index=1 */
                        FLUID_MOD_CC                        /* CC=1 */
                        | FLUID_MOD_LINEAR                  /* type=0 */
                        | FLUID_MOD_UNIPOLAR                /* P=0 */
                        | FLUID_MOD_POSITIVE                /* D=0 */
                       );
  fluid_mod_set_source2(&default_mod2viblfo_mod, 0, 0); /* no second source */
  fluid_mod_set_dest(&default_mod2viblfo_mod, GEN_VIBLFOTOPITCH);        /* Target: Vib. LFO => pitch */
  fluid_mod_set_amount(&default_mod2viblfo_mod, 50);

  /* SF2.01 page 55 section 8.4.5: MIDI continuous controller 7 to initial attenuation*/
  fluid_mod_set_source1(&default_att_mod, 7,                     /* index=7 */
                        FLUID_MOD_CC                              /* CC=1 */
                        | FLUID_MOD_CONCAVE                       /* type=1 */
                        | FLUID_MOD_UNIPOLAR                      /* P=0 */
                        | FLUID_MOD_NEGATIVE                      /* D=1 */
                       );
  fluid_mod_set_source2(&default_att_mod, 0, 0);                 /* No second source */
  fluid_mod_set_dest(&default_att_mod, GEN_ATTENUATION);         /* Target: Initial attenuation */
  fluid_mod_set_amount(&default_att_mod, 960.0);                 /* Amount: 960 */

  /* SF2.01 page 55 section 8.4.6 MIDI continuous controller 10 to Pan Position */
  fluid_mod_set_source1(&default_pan_mod, 10,                    /* index=10 */
                        FLUID_MOD_CC                              /* CC=1 */
                        | FLUID_MOD_LINEAR                        /* type=0 */
                        | FLUID_MOD_BIPOLAR                       /* P=1 */
                        | FLUID_MOD_POSITIVE                      /* D=0 */
                       );
  fluid_mod_set_source2(&default_pan_mod, 0, 0);                 /* No second source */
  fluid_mod_set_dest(&default_pan_mod, GEN_PAN);                 /* Target: pan */
  /* Amount: 500. The SF specs $8.4.6, p. 55 syas: "Amount = 1000
     tenths of a percent". The center value (64) corresponds to 50%,
     so it follows that amount = 50% x 1000/% = 500. */
  fluid_mod_set_amount(&default_pan_mod, 500.0);

  /* SF2.01 page 55 section 8.4.7: MIDI continuous controller 11 to initial attenuation*/
  fluid_mod_set_source1(&default_expr_mod, 11,                     /* index=11 */
                        FLUID_MOD_CC                              /* CC=1 */
                        | FLUID_MOD_CONCAVE                       /* type=1 */
                        | FLUID_MOD_UNIPOLAR                      /* P=0 */
                        | FLUID_MOD_NEGATIVE                      /* D=1 */
                       );
  fluid_mod_set_source2(&default_expr_mod, 0, 0);                 /* No second source */
  fluid_mod_set_dest(&default_expr_mod, GEN_ATTENUATION);         /* Target: Initial attenuation */
  fluid_mod_set_amount(&default_expr_mod, 960.0);                 /* Amount: 960 */

  /* SF2.01 page 55 section 8.4.8: MIDI continuous controller 91 to Reverb send */
  fluid_mod_set_source1(&default_reverb_mod, 91,                 /* index=91 */
                        FLUID_MOD_CC                              /* CC=1 */
                        | FLUID_MOD_LINEAR                        /* type=0 */
                        | FLUID_MOD_UNIPOLAR                      /* P=0 */
                        | FLUID_MOD_POSITIVE                      /* D=0 */
                       );
  fluid_mod_set_source2(&default_reverb_mod, 0, 0);              /* No second source */
  fluid_mod_set_dest(&default_reverb_mod, GEN_REVERBSEND);       /* Target: Reverb send */
  fluid_mod_set_amount(&default_reverb_mod, 200);                /* Amount: 200 ('tenths of a percent') */

  /* SF2.01 page 55 section 8.4.9: MIDI continuous controller 93 to Reverb send */
  fluid_mod_set_source1(&default_chorus_mod, 93,                 /* index=93 */
                        FLUID_MOD_CC                              /* CC=1 */
                        | FLUID_MOD_LINEAR                        /* type=0 */
                        | FLUID_MOD_UNIPOLAR                      /* P=0 */
                        | FLUID_MOD_POSITIVE                      /* D=0 */
                       );
  fluid_mod_set_source2(&default_chorus_mod, 0, 0);              /* No second source */
  fluid_mod_set_dest(&default_chorus_mod, GEN_CHORUSSEND);       /* Target: Chorus */
  fluid_mod_set_amount(&default_chorus_mod, 200);                /* Amount: 200 ('tenths of a percent') */

  /* SF2.01 page 57 section 8.4.10 MIDI Pitch Wheel to Initial Pitch ... */
  fluid_mod_set_source1(&default_pitch_bend_mod, FLUID_MOD_PITCHWHEEL, /* Index=14 */
                        FLUID_MOD_GC                              /* CC =0 */
                        | FLUID_MOD_LINEAR                        /* type=0 */
                        | FLUID_MOD_BIPOLAR                       /* P=1 */
                        | FLUID_MOD_POSITIVE                      /* D=0 */
                       );
  fluid_mod_set_source2(&default_pitch_bend_mod, FLUID_MOD_PITCHWHEELSENS,  /* Index = 16 */
                        FLUID_MOD_GC                                        /* CC=0 */
                        | FLUID_MOD_LINEAR                                  /* type=0 */
                        | FLUID_MOD_UNIPOLAR                                /* P=0 */
                        | FLUID_MOD_POSITIVE                                /* D=0 */
                       );
  fluid_mod_set_dest(&default_pitch_bend_mod, GEN_PITCH);                 /* Destination: Initial pitch */
  fluid_mod_set_amount(&default_pitch_bend_mod, 12700.0);                 /* Amount: 12700 cents */
}


/***************************************************************
 *
 *                      FLUID SYNTH
 */

/*
 * new_fluid_synth
 */
fluid_synth_t*
new_fluid_synth(fluid_settings_t *settings)
{
  int i;
  fluid_synth_t* synth;
  fluid_sfloader_t* loader;

  /* initialize all the conversion tables and other stuff */
  if (fluid_synth_initialized == 0) {
    fluid_synth_init();
  }

  /* allocate a new synthesizer object */
  synth = FLUID_NEW(fluid_synth_t);
  if (synth == NULL) {
    FLUID_LOG(FLUID_ERR, "Out of memory");
    return NULL;
  }
  FLUID_MEMSET(synth, 0, sizeof(fluid_synth_t));

  fluid_mutex_init(synth->busy);

  synth->settings = settings;

  synth->with_reverb = settings->reverb;
  synth->with_chorus = settings->chorus;
  synth->verbose = settings->verbose;
  synth->dump = settings->dump;
  synth->polyphony = settings->polyphony;
  synth->sample_rate = settings->sample_rate;
  synth->midi_channels = settings->midi_channels;
  synth->audio_channels = settings->audio_channels;
  synth->audio_groups = settings->audio_groups;
  synth->effects_channels = settings->effects_channels;
  synth->gain = settings->gain;

  if (synth->audio_channels < 1) {
    FLUID_LOG(FLUID_WARN, "Requested number of audio channels is smaller than 1. "
              "Changing this setting to 1.");
    synth->audio_channels = 1;
  } else if (synth->audio_channels > 128) {
    FLUID_LOG(FLUID_WARN, "Requested number of audio channels is too big (%d). "
              "Limiting this setting to 128.", synth->audio_channels);
    synth->audio_channels = 128;
  }

  if (synth->audio_groups < 1) {
    FLUID_LOG(FLUID_WARN, "Requested number of audio groups is smaller than 1. "
              "Changing this setting to 1.");
    synth->audio_groups = 1;
  } else if (synth->audio_groups > 128) {
    FLUID_LOG(FLUID_WARN, "Requested number of audio groups is too big (%d). "
              "Limiting this setting to 128.", synth->audio_groups);
    synth->audio_groups = 128;
  }

  if (synth->effects_channels != 2) {
    FLUID_LOG(FLUID_WARN, "Invalid number of effects channels (%d)."
              "Setting effects channels to 2.", synth->effects_channels);
    synth->effects_channels = 2;
  }

  /* The number of buffers is determined by the higher number of nr
   * groups / nr audio channels.  If LADSPA is unused, they should be
   * the same. */
  synth->nbuf = synth->audio_channels;
  if (synth->audio_groups > synth->nbuf) {
    synth->nbuf = synth->audio_groups;
  }

  /* as soon as the synth is created it starts playing. */
  synth->state = FLUID_SYNTH_PLAYING;
  synth->sfont = NULL;
  synth->noteid = 0;
  synth->ticks = 0;
  synth->tuning = NULL;

  /* allocate and add the default sfont loader */
  loader = new_fluid_altsfloader();

  if (loader == NULL) {
    FLUID_LOG(FLUID_WARN, "Failed to create the default SoundFont loader");
  } else {
    fluid_synth_add_sfloader(synth, loader);
  }

  /* allocate all channel objects */
  synth->channel = FLUID_ARRAY(fluid_channel_t*, synth->midi_channels);
  if (synth->channel == NULL) {
    FLUID_LOG(FLUID_ERR, "Out of memory");
    goto error_recovery;
  }
  for (i = 0; i < synth->midi_channels; i++) {
    synth->channel[i] = new_fluid_channel(synth, i);
    if (synth->channel[i] == NULL) {
      goto error_recovery;
    }
  }

  /* allocate all synthesis processes */
  synth->nvoice = synth->polyphony;
  synth->voice = FLUID_ARRAY(fluid_voice_t*, synth->nvoice);
  if (synth->voice == NULL) {
    goto error_recovery;
  }
  for (i = 0; i < synth->nvoice; i++) {
    synth->voice[i] = new_fluid_voice(synth->sample_rate);
    if (synth->voice[i] == NULL) {
      goto error_recovery;
    }
  }

  /* Allocate the sample buffers */
  synth->left_buf = NULL;
  synth->right_buf = NULL;
  synth->fx_left_buf = NULL;
  synth->fx_right_buf = NULL;

  /* Left and right audio buffers */

  synth->left_buf = FLUID_ARRAY(fluid_buf_t*, synth->nbuf);
  synth->right_buf = FLUID_ARRAY(fluid_buf_t*, synth->nbuf);

  if ((synth->left_buf == NULL) || (synth->right_buf == NULL)) {
    FLUID_LOG(FLUID_ERR, "Out of memory");
    goto error_recovery;
  }

  FLUID_MEMSET(synth->left_buf, 0, synth->nbuf * sizeof(fluid_buf_t*));
  FLUID_MEMSET(synth->right_buf, 0, synth->nbuf * sizeof(fluid_buf_t*));

  for (i = 0; i < synth->nbuf; i++) {

    synth->left_buf[i] = FLUID_ARRAY(fluid_buf_t, FLUID_BUFSIZE);
    synth->right_buf[i] = FLUID_ARRAY(fluid_buf_t, FLUID_BUFSIZE);

    if ((synth->left_buf[i] == NULL) || (synth->right_buf[i] == NULL)) {
      FLUID_LOG(FLUID_ERR, "Out of memory");
      goto error_recovery;
    }
  }

  /* Effects audio buffers */

  synth->fx_left_buf = FLUID_ARRAY(fluid_buf_t*, synth->effects_channels);
  synth->fx_right_buf = FLUID_ARRAY(fluid_buf_t*, synth->effects_channels);

  if ((synth->fx_left_buf == NULL) || (synth->fx_right_buf == NULL)) {
    FLUID_LOG(FLUID_ERR, "Out of memory");
    goto error_recovery;
  }

  FLUID_MEMSET(synth->fx_left_buf, 0, 2 * sizeof(fluid_buf_t*));
  FLUID_MEMSET(synth->fx_right_buf, 0, 2 * sizeof(fluid_buf_t*));

  for (i = 0; i < synth->effects_channels; i++) {
    synth->fx_left_buf[i] = FLUID_ARRAY(fluid_buf_t, FLUID_BUFSIZE);
    synth->fx_right_buf[i] = FLUID_ARRAY(fluid_buf_t, FLUID_BUFSIZE);

    if ((synth->fx_left_buf[i] == NULL) || (synth->fx_right_buf[i] == NULL)) {
      FLUID_LOG(FLUID_ERR, "Out of memory");
      goto error_recovery;
    }
  }


  synth->cur = FLUID_BUFSIZE;

  if (synth->with_reverb) {
    /* allocate the reverb module */
    synth->reverb = new_fluid_revmodel();
    if (synth->reverb == NULL) {
      FLUID_LOG(FLUID_ERR, "Out of memory");
      goto error_recovery;
    }

    fluid_synth_set_reverb(synth,
                           FLUID_REVERB_DEFAULT_ROOMSIZE,
                           FLUID_REVERB_DEFAULT_DAMP,
                           FLUID_REVERB_DEFAULT_WIDTH,
                           FLUID_REVERB_DEFAULT_LEVEL);
  }

  if (synth->with_chorus) {
    /* allocate the chorus module */
    synth->chorus = new_fluid_chorus(synth->sample_rate);
    if (synth->chorus == NULL) {
      FLUID_LOG(FLUID_ERR, "Out of memory");
      goto error_recovery;
    }
  }

  /* FIXME */
  synth->start = fluid_curtime();

  return synth;

error_recovery:
  delete_fluid_synth(synth);
  return NULL;
}

int delete_fluid_synth(fluid_synth_t* synth)
{
  int i, k;
  fluid_list_t *list;
  fluid_sfont_t* sfont;
  fluid_bank_offset_t* bank_offset;
  fluid_sfloader_t* loader;

  if (synth == NULL) {
    return FLUID_OK;
  }

  fluid_profiling_print();

  synth->state = FLUID_SYNTH_STOPPED;

  /* turn off all voices, needed to unload SoundFont data */
  if (synth->voice != NULL) {
    for (i = 0; i < synth->nvoice; i++) {
      if (synth->voice[i] && fluid_voice_is_playing (synth->voice[i]))
        fluid_voice_off (synth->voice[i]);
    }
  }

  /* delete all the SoundFonts */
  for (list = synth->sfont; list; list = fluid_list_next(list)) {
    sfont = (fluid_sfont_t*) fluid_list_get(list);
    delete_fluid_sfont(sfont);
  }

  delete_fluid_list(synth->sfont);

  /* and the SoundFont offsets */
  for (list = synth->bank_offsets; list; list = fluid_list_next(list)) {
    bank_offset = (fluid_bank_offset_t*) fluid_list_get(list);
    FLUID_FREE(bank_offset);
  }

  delete_fluid_list(synth->bank_offsets);


  /* delete all the SoundFont loaders */

  for (list = synth->loaders; list; list = fluid_list_next(list)) {
    loader = (fluid_sfloader_t*) fluid_list_get(list);
    fluid_sfloader_delete(loader);
  }

  delete_fluid_list(synth->loaders);


  if (synth->channel != NULL) {
    for (i = 0; i < synth->midi_channels; i++) {
      if (synth->channel[i] != NULL) {
        delete_fluid_channel(synth->channel[i]);
      }
    }
    FLUID_FREE(synth->channel);
  }

  if (synth->voice != NULL) {
    for (i = 0; i < synth->nvoice; i++) {
      if (synth->voice[i] != NULL) {
        delete_fluid_voice(synth->voice[i]);
      }
    }
    FLUID_FREE(synth->voice);
  }

  /* free all the sample buffers */
  if (synth->left_buf != NULL) {
    for (i = 0; i < synth->nbuf; i++) {
      if (synth->left_buf[i] != NULL) {
        FLUID_FREE(synth->left_buf[i]);
      }
    }
    FLUID_FREE(synth->left_buf);
  }

  if (synth->right_buf != NULL) {
    for (i = 0; i < synth->nbuf; i++) {
      if (synth->right_buf[i] != NULL) {
        FLUID_FREE(synth->right_buf[i]);
      }
    }
    FLUID_FREE(synth->right_buf);
  }

  if (synth->fx_left_buf != NULL) {
    for (i = 0; i < 2; i++) {
      if (synth->fx_left_buf[i] != NULL) {
        FLUID_FREE(synth->fx_left_buf[i]);
      }
    }
    FLUID_FREE(synth->fx_left_buf);
  }

  if (synth->fx_right_buf != NULL) {
    for (i = 0; i < 2; i++) {
      if (synth->fx_right_buf[i] != NULL) {
        FLUID_FREE(synth->fx_right_buf[i]);
      }
    }
    FLUID_FREE(synth->fx_right_buf);
  }

  /* release the reverb module */
  if (synth->reverb != NULL) {
    delete_fluid_revmodel(synth->reverb);
  }

  /* release the chorus module */
  if (synth->chorus != NULL) {
    delete_fluid_chorus(synth->chorus);
  }

  /* free the tunings, if any */
  if (synth->tuning != NULL) {
    for (i = 0; i < 128; i++) {
      if (synth->tuning[i] != NULL) {
        for (k = 0; k < 128; k++) {
          if (synth->tuning[i][k] != NULL) {
            FLUID_FREE(synth->tuning[i][k]);
          }
        }
        FLUID_FREE(synth->tuning[i]);
      }
    }
    FLUID_FREE(synth->tuning);
  }

  fluid_mutex_destroy(synth->busy);

  FLUID_FREE(synth);

  return FLUID_OK;
}

/*
 * fluid_synth_error
 *
 * The error messages are not thread-save, yet. They are still stored
 * in a global message buffer (see fluid_sys.c).
 * */
char* fluid_synth_error(fluid_synth_t* synth)
{
  return fluid_error();
}

int fluid_synth_noteon(fluid_synth_t* synth, int chan, int key, int vel)
{
  fluid_channel_t* channel;
  int r = FLUID_FAILED;

  /* check the ranges of the arguments */
  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return FLUID_FAILED;
  }

  /* notes with velocity zero go to noteoff  */
  if (vel == 0) {
    return fluid_synth_noteoff(synth, chan, key);
  }

  channel = synth->channel[chan];

  /* make sure this channel has a preset */
  if (channel->preset == NULL) {
    if (synth->verbose) {
      FLUID_LOG(FLUID_INFO, "noteon\t%d\t%d\t%d\t%05d\t%.3f\t%.3f\t%.3f\t%d\t%s",
                chan, key, vel, 0,
                (float) synth->ticks / 44100.0f,
                (fluid_curtime() - synth->start) / 1000.0f,
                0.0f, 0, "channel has no preset");
    }
    return FLUID_FAILED;
  }

  /* If there is another voice process on the same channel and key,
     advance it to the release phase. */
  fluid_synth_release_voice_on_same_note(synth, chan, key);

  return fluid_synth_start(synth, synth->noteid++, channel->preset, 0, chan, key, vel);
}

int fluid_synth_noteoff(fluid_synth_t* synth, int chan, int key)
{
  int i;
  fluid_voice_t* voice;
  int status = FLUID_FAILED;
  /*   fluid_mutex_lock(synth->busy); /\* Don't interfere with the audio thread *\/ */
  /*   fluid_mutex_unlock(synth->busy); */

  for (i = 0; i < synth->polyphony; i++) {
    voice = synth->voice[i];
    if (_ON(voice) && (voice->chan == chan) && (voice->key == key)) {
      if (synth->verbose) {
        int used_voices = 0;
        int k;
        for (k = 0; k < synth->polyphony; k++) {
          if (!_AVAILABLE(synth->voice[k])) {
            used_voices++;
          }
        }
        FLUID_LOG(FLUID_INFO, "noteoff\t%d\t%d\t%d\t%05d\t%.3f\t%.3f\t%.3f\t%d",
                  voice->chan, voice->key, 0, voice->id,
                  (float) (voice->start_time + voice->ticks) / 44100.0f,
                  (fluid_curtime() - synth->start) / 1000.0f,
                  (float) voice->ticks / 44100.0f,
                  used_voices);
      } /* if verbose */
      fluid_voice_noteoff(voice);
      status = FLUID_OK;
    } /* if voice on */
  } /* for all voices */
  return status;
}

int fluid_synth_damp_voices(fluid_synth_t* synth, int chan)
{
  int i;
  fluid_voice_t* voice;

  /*   fluid_mutex_lock(synth->busy); /\* Don't interfere with the audio thread *\/ */
  /*   fluid_mutex_unlock(synth->busy); */

  for (i = 0; i < synth->polyphony; i++) {
    voice = synth->voice[i];
    if ((voice->chan == chan) && _SUSTAINED(voice)) {
      /*        printf("turned off sustained note: chan=%d, key=%d, vel=%d\n", voice->chan, voice->key, voice->vel); */
      fluid_voice_noteoff(voice);
    }
  }

  return FLUID_OK;
}

int fluid_synth_cc(fluid_synth_t* synth, int chan, int num, int val)
{
  /*   fluid_mutex_lock(synth->busy); /\* Don't interfere with the audio thread *\/ */
  /*   fluid_mutex_unlock(synth->busy); */

  /* check the ranges of the arguments */
  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return FLUID_FAILED;
  }
  if ((num < 0) || (num >= 128)) {
    FLUID_LOG(FLUID_WARN, "Ctrl out of range");
    return FLUID_FAILED;
  }
  if ((val < 0) || (val >= 128)) {
    FLUID_LOG(FLUID_WARN, "Value out of range");
    return FLUID_FAILED;
  }

  if (synth->verbose) {
    FLUID_LOG(FLUID_INFO, "cc\t%d\t%d\t%d", chan, num, val);
  }

  /* set the controller value in the channel */
  fluid_channel_cc(synth->channel[chan], num, val);

  return FLUID_OK;
}

int fluid_synth_get_cc(fluid_synth_t* synth, int chan, int num, int* pval)
{
  /* check the ranges of the arguments */
  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return FLUID_FAILED;
  }
  if ((num < 0) || (num >= 128)) {
    FLUID_LOG(FLUID_WARN, "Ctrl out of range");
    return FLUID_FAILED;
  }

  *pval = synth->channel[chan]->cc[num];
  return FLUID_OK;
}

/*
 * fluid_synth_all_notes_off
 *
 * put all notes on this channel into released state.
 */
int fluid_synth_all_notes_off(fluid_synth_t* synth, int chan)
{
  int i;
  fluid_voice_t* voice;

  for (i = 0; i < synth->polyphony; i++) {
    voice = synth->voice[i];
    if (_PLAYING(voice) && (voice->chan == chan)) {
      fluid_voice_noteoff(voice);
    }
  }
  return FLUID_OK;
}

/*
 * fluid_synth_all_sounds_off
 *
 * immediately stop all notes on this channel.
 */
int fluid_synth_all_sounds_off(fluid_synth_t* synth, int chan)
{
  int i;
  fluid_voice_t* voice;

  for (i = 0; i < synth->polyphony; i++) {
    voice = synth->voice[i];
    if (_PLAYING(voice) && (voice->chan == chan)) {
      fluid_voice_off(voice);
    }
  }
  return FLUID_OK;
}

/*
 * fluid_synth_system_reset
 *
 * Purpose:
 * Respond to the MIDI command 'system reset' (0xFF, big red 'panic' button)
 */
int fluid_synth_system_reset(fluid_synth_t* synth)
{
  int i;
  fluid_voice_t* voice;

  for (i = 0; i < synth->polyphony; i++) {
    voice = synth->voice[i];
    if (_PLAYING(voice)) {
      fluid_voice_off(voice);
    }
  }

  for (i = 0; i < synth->midi_channels; i++) {
    fluid_channel_reset(synth->channel[i]);
  }

  if (synth->chorus != NULL) {
    fluid_chorus_reset(synth->chorus);
  }
  if (synth->reverb != NULL) {
    fluid_revmodel_reset(synth->reverb);
  }
  return FLUID_OK;
}

/*
 * fluid_synth_modulate_voices
 *
 * tell all synthesis processes on this channel to update their
 * synthesis parameters after a control change.
 */
int fluid_synth_modulate_voices(fluid_synth_t* synth, int chan, int is_cc, int ctrl)
{
  int i;
  fluid_voice_t* voice;

  /*   fluid_mutex_lock(synth->busy); /\* Don't interfere with the audio thread *\/ */
  /*   fluid_mutex_unlock(synth->busy); */

  for (i = 0; i < synth->polyphony; i++) {
    voice = synth->voice[i];
    if (voice->chan == chan) {
      fluid_voice_modulate(voice, is_cc, ctrl);
    }
  }
  return FLUID_OK;
}

/*
 * fluid_synth_modulate_voices_all
 *
 * Tell all synthesis processes on this channel to update their
 * synthesis parameters after an all control off message (i.e. all
 * controller have been reset to their default value).
 */
int fluid_synth_modulate_voices_all(fluid_synth_t* synth, int chan)
{
  int i;
  fluid_voice_t* voice;

  /*   fluid_mutex_lock(synth->busy); /\* Don't interfere with the audio thread *\/ */
  /*   fluid_mutex_unlock(synth->busy); */

  for (i = 0; i < synth->polyphony; i++) {
    voice = synth->voice[i];
    if (voice->chan == chan) {
      fluid_voice_modulate_all(voice);
    }
  }
  return FLUID_OK;
}

/**
 * Set the MIDI channel pressure controller value.
 * @param synth FluidSynth instance
 * @param chan MIDI channel number
 * @param val MIDI channel pressure value (7 bit, 0-127)
 * @return FLUID_OK on success
 *
 * Assign to the MIDI channel pressure controller value on a specific MIDI channel
 * in real time.
 */
int fluid_synth_channel_pressure(fluid_synth_t* synth, int chan, int val)
{

  /*   fluid_mutex_lock(synth->busy); /\* Don't interfere with the audio thread *\/ */
  /*   fluid_mutex_unlock(synth->busy); */

  /* check the ranges of the arguments */
  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return FLUID_FAILED;
  }

  if (synth->verbose) {
    FLUID_LOG(FLUID_INFO, "channelpressure\t%d\t%d", chan, val);
  }

  /* set the channel pressure value in the channel */
  fluid_channel_pressure(synth->channel[chan], val);

  return FLUID_OK;
}

/**
 * Set the MIDI pitch bend controller value.
 * @param synth FluidSynth instance
 * @param chan MIDI channel number
 * @param val MIDI pitch bend value (14 bit, 0-16383 with 8192 being center)
 * @return FLUID_OK on success
 *
 * Assign to the MIDI pitch bend controller value on a specific MIDI channel
 * in real time.
 */
int fluid_synth_pitch_bend(fluid_synth_t* synth, int chan, int val)
{

  /*   fluid_mutex_lock(synth->busy); /\* Don't interfere with the audio thread *\/ */
  /*   fluid_mutex_unlock(synth->busy); */

  /* check the ranges of the arguments */
  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return FLUID_FAILED;
  }

  if (synth->verbose) {
    FLUID_LOG(FLUID_INFO, "pitchb\t%d\t%d", chan, val);
  }

  /* set the pitch-bend value in the channel */
  fluid_channel_pitch_bend(synth->channel[chan], val);

  return FLUID_OK;
}

/*
 * fluid_synth_pitch_bend
 */
int fluid_synth_get_pitch_bend(fluid_synth_t* synth, int chan, int* ppitch_bend)
{
  /* check the ranges of the arguments */
  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return FLUID_FAILED;
  }

  *ppitch_bend = synth->channel[chan]->pitch_bend;
  return FLUID_OK;
}

/*
 * Fluid_synth_pitch_wheel_sens
 */
int fluid_synth_pitch_wheel_sens(fluid_synth_t* synth, int chan, int val)
{

  /* check the ranges of the arguments */
  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return FLUID_FAILED;
  }

  if (synth->verbose) {
    FLUID_LOG(FLUID_INFO, "pitchsens\t%d\t%d", chan, val);
  }

  /* set the pitch-bend value in the channel */
  fluid_channel_pitch_wheel_sens(synth->channel[chan], val);

  return FLUID_OK;
}

/*
 * fluid_synth_get_pitch_wheel_sens
 *
 * Note : this function was added after version 1.0 API freeze.
 * So its API is not in the synth.h file. It should be added in some later
 * version of fluidsynth. Maybe v2.0 ? -- Antoine Schmitt May 2003
 */

int fluid_synth_get_pitch_wheel_sens(fluid_synth_t* synth, int chan, int* pval)
{

  // check the ranges of the arguments
  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return FLUID_FAILED;
  }

  // get the pitch-bend value in the channel
  *pval = synth->channel[chan]->pitch_wheel_sensitivity;

  return FLUID_OK;
}

fluid_preset_t* fluid_synth_get_preset(fluid_synth_t* synth, unsigned int sfontnum,
                       unsigned int banknum, unsigned int prognum)
{
  fluid_preset_t* preset = NULL;
  fluid_sfont_t* sfont = NULL;
  fluid_list_t* list = synth->sfont;
  int offset;

  sfont = fluid_synth_get_sfont_by_id(synth, sfontnum);

  if (sfont != NULL) {
    offset = fluid_synth_get_bank_offset(synth, sfontnum);
    preset = fluid_sfont_get_preset(sfont, banknum - offset, prognum);
    if (preset != NULL) {
      return preset;
    }
  }
  return NULL;
}

fluid_preset_t* fluid_synth_get_preset2(fluid_synth_t* synth, char* sfont_name,
                        unsigned int banknum, unsigned int prognum)
{
  fluid_preset_t* preset = NULL;
  fluid_sfont_t* sfont = NULL;
  int offset;

  sfont = fluid_synth_get_sfont_by_name(synth, sfont_name);

  if (sfont != NULL) {
    offset = fluid_synth_get_bank_offset(synth, fluid_sfont_get_id(sfont));
    preset = fluid_sfont_get_preset(sfont, banknum - offset, prognum);
    if (preset != NULL) {
      return preset;
    }
  }
  return NULL;
}

fluid_preset_t* fluid_synth_find_preset(fluid_synth_t* synth,
                                        unsigned int banknum,
                                        unsigned int prognum)
{
  fluid_preset_t* preset = NULL;
  fluid_sfont_t* sfont = NULL;
  fluid_list_t* list = synth->sfont;
  int offset;

  while (list) {

    sfont = (fluid_sfont_t*) fluid_list_get(list);
    offset = fluid_synth_get_bank_offset(synth, fluid_sfont_get_id(sfont));
    preset = fluid_sfont_get_preset(sfont, banknum - offset, prognum);

    if (preset != NULL) {
      preset->sfont = sfont; /* FIXME */
      return preset;
    }

    list = fluid_list_next(list);

  }
  return NULL;
}

int fluid_synth_program_change(fluid_synth_t* synth, int chan, int prognum)
{
  fluid_preset_t* preset = NULL;
  fluid_channel_t* channel;
  unsigned int banknum;
  unsigned int sfont_id;
  int subst_bank, subst_prog;

  if ((prognum < 0) || (prognum >= FLUID_NUM_PROGRAMS) ||
      (chan < 0) || (chan >= synth->midi_channels))
  {
    FLUID_LOG(FLUID_ERR, "Index out of range (chan=%d, prog=%d)", chan, prognum);
    return FLUID_FAILED;
  }

  channel = synth->channel[chan];
  banknum = fluid_channel_get_banknum(channel);

  /* inform the channel of the new program number */
  fluid_channel_set_prognum(channel, prognum);

  if (synth->verbose)
    FLUID_LOG(FLUID_INFO, "prog\t%d\t%d\t%d", chan, banknum, prognum);

  /* Special handling of channel 10 (or 9 counting from 0). channel
   * 10 is the percussion channel.
   *
   * FIXME - Shouldn't hard code bank selection for channel 10.  I think this
   * is a hack for MIDI files that do bank changes in GM mode.  Proper way to
   * handle this would probably be to ignore bank changes when in GM mode.
   */
  if (channel->channum == 9)
    preset = fluid_synth_find_preset(synth, DRUM_INST_BANK, prognum);
  else preset = fluid_synth_find_preset(synth, banknum, prognum);

  /* Fallback to another preset if not found */
  if (!preset)
  {
    subst_bank = banknum;
    subst_prog = prognum;

    /* Melodic instrument? */
    if (channel->channum != 9 && banknum != DRUM_INST_BANK)
    {
      subst_bank = 0;

      /* Fallback first to bank 0:prognum */
      preset = fluid_synth_find_preset(synth, 0, prognum);

      /* Fallback to first preset in bank 0 */
      if (!preset && prognum != 0)
      {
        preset = fluid_synth_find_preset(synth, 0, 0);
        subst_prog = 0;
      }
    }
    else /* Percussion: Fallback to preset 0 in percussion bank */
    {
      preset = fluid_synth_find_preset(synth, DRUM_INST_BANK, 0);
      subst_prog = 0;
    }

    if (preset)
      FLUID_LOG(FLUID_WARN, "Instrument not found on channel %d [bank=%d prog=%d], substituted [bank=%d prog=%d]",
                chan, banknum, prognum, subst_bank, subst_prog);
  }

  sfont_id = preset ? fluid_sfont_get_id(preset->sfont) : 0;
  fluid_channel_set_sfontnum(channel, sfont_id);
  fluid_channel_set_preset(channel, preset);

  return FLUID_OK;
}

int fluid_synth_bank_select(fluid_synth_t* synth, int chan, unsigned int bank)
{
  if ((chan >= 0) && (chan < synth->midi_channels)) {
    fluid_channel_set_banknum(synth->channel[chan], bank);
    return FLUID_OK;
  }
  return FLUID_FAILED;
}

int fluid_synth_sfont_select(fluid_synth_t* synth, int chan, unsigned int sfont_id)
{
  if ((chan >= 0) && (chan < synth->midi_channels)) {
    fluid_channel_set_sfontnum(synth->channel[chan], sfont_id);
    return FLUID_OK;
  }
  return FLUID_FAILED;
}

int fluid_synth_get_program(fluid_synth_t* synth, int chan,
                        unsigned int* sfont_id, unsigned int* bank_num, unsigned int* preset_num)
{
  fluid_channel_t* channel;
  if ((chan >= 0) && (chan < synth->midi_channels)) {
    channel = synth->channel[chan];
    *sfont_id = fluid_channel_get_sfontnum(channel);
    *bank_num = fluid_channel_get_banknum(channel);
    *preset_num = fluid_channel_get_prognum(channel);
    return FLUID_OK;
  }
  return FLUID_FAILED;
}

int fluid_synth_program_select(fluid_synth_t* synth,
                               int chan,
                               unsigned int sfont_id,
                               unsigned int bank_num,
                               unsigned int preset_num)
{
  fluid_preset_t* preset = NULL;
  fluid_channel_t* channel;

  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_ERR, "Channel number out of range (chan=%d)", chan);
    return FLUID_FAILED;
  }
  channel = synth->channel[chan];

  preset = fluid_synth_get_preset(synth, sfont_id, bank_num, preset_num);
  if (preset == NULL) {
    FLUID_LOG(FLUID_ERR,
              "There is no preset with bank number %d and preset number %d in SoundFont %d",
              bank_num, preset_num, sfont_id);
    return FLUID_FAILED;
  }

  /* inform the channel of the new bank and program number */
  fluid_channel_set_sfontnum(channel, sfont_id);
  fluid_channel_set_banknum(channel, bank_num);
  fluid_channel_set_prognum(channel, preset_num);

  fluid_channel_set_preset(channel, preset);

  return FLUID_OK;
}

int fluid_synth_program_select2(fluid_synth_t* synth,
                                int chan,
                                char* sfont_name,
                                unsigned int bank_num,
                                unsigned int preset_num)
{
  fluid_preset_t* preset = NULL;
  fluid_channel_t* channel;
  fluid_sfont_t* sfont = NULL;
  int offset;

  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_ERR, "Channel number out of range (chan=%d)", chan);
    return FLUID_FAILED;
  }
  channel = synth->channel[chan];

  sfont = fluid_synth_get_sfont_by_name(synth, sfont_name);
  if (sfont == NULL) {
    FLUID_LOG(FLUID_ERR, "Could not find SoundFont %s", sfont_name);
    return FLUID_FAILED;
  }

  offset = fluid_synth_get_bank_offset(synth, fluid_sfont_get_id(sfont));
  preset = fluid_sfont_get_preset(sfont, bank_num - offset, preset_num);
  if (preset == NULL) {
    FLUID_LOG(FLUID_ERR,
              "There is no preset with bank number %d and preset number %d in SoundFont %s",
              bank_num, preset_num, sfont_name);
    return FLUID_FAILED;
  }

  /* inform the channel of the new bank and program number */
  fluid_channel_set_sfontnum(channel, fluid_sfont_get_id(sfont));
  fluid_channel_set_banknum(channel, bank_num);
  fluid_channel_set_prognum(channel, preset_num);

  fluid_channel_set_preset(channel, preset);

  return FLUID_OK;
}

void fluid_synth_update_presets(fluid_synth_t* synth)
{
  int chan;
  fluid_channel_t* channel;

  for (chan = 0; chan < synth->midi_channels; chan++) {
    channel = synth->channel[chan];
    fluid_channel_set_preset(channel,
                             fluid_synth_get_preset(synth,
                                 fluid_channel_get_sfontnum(channel),
                                 fluid_channel_get_banknum(channel),
                                 fluid_channel_get_prognum(channel)));
  }
}

int fluid_synth_update_gain(fluid_synth_t* synth, char* name, fluid_real_t value)
{
  fluid_synth_set_gain(synth, value);
  return 0;
}

void fluid_synth_set_gain(fluid_synth_t* synth, fluid_real_t gain)
{
  int i;

  fluid_clip(gain, 0.0f, 10.0f);
  synth->gain = gain;

  for (i = 0; i < synth->polyphony; i++) {
    fluid_voice_t* voice = synth->voice[i];
    if (_PLAYING(voice)) {
      fluid_voice_set_gain(voice, gain);
    }
  }
}

fluid_real_t fluid_synth_get_gain(fluid_synth_t* synth)
{
  return synth->gain;
}

int fluid_synth_update_polyphony(fluid_synth_t* synth, char* name, int value)
{
  fluid_synth_set_polyphony(synth, value);
  return 0;
}

int fluid_synth_set_polyphony(fluid_synth_t* synth, int polyphony)
{
  int i;

  if (polyphony < 1 || polyphony > synth->nvoice) {
    return FLUID_FAILED;
  }

  /* turn off any voices above the new limit */
  for (i = polyphony; i < synth->nvoice; i++) {
    fluid_voice_t* voice = synth->voice[i];
    if (_PLAYING(voice)) {
      fluid_voice_off(voice);
    }
  }

  synth->polyphony = polyphony;

  return FLUID_OK;
}

int fluid_synth_get_polyphony(fluid_synth_t* synth)
{
  return synth->polyphony;
}

int fluid_synth_get_internal_bufsize(fluid_synth_t* synth)
{
  return FLUID_BUFSIZE;
}

/*
 * fluid_synth_program_reset
 *
 * Resend a bank select and a program change for every channel. This
 * function is called mainly after a SoundFont has been loaded,
 * unloaded or reloaded.  */
int
fluid_synth_program_reset(fluid_synth_t* synth)
{
  int i;
  /* try to set the correct presets */
  for (i = 0; i < synth->midi_channels; i++) {
    fluid_synth_program_change(synth, i, fluid_channel_get_prognum(synth->channel[i]));
  }
  return FLUID_OK;
}

int fluid_synth_set_reverb_preset(fluid_synth_t* synth, int num)
{
  int i = 0;
  while (revmodel_preset[i].name != NULL) {
    if (i == num) {
      fluid_revmodel_setroomsize(synth->reverb, revmodel_preset[i].roomsize);
      fluid_revmodel_setdamp(synth->reverb, revmodel_preset[i].damp);
      fluid_revmodel_setwidth(synth->reverb, revmodel_preset[i].width);
      fluid_revmodel_setlevel(synth->reverb, revmodel_preset[i].level);
      return FLUID_OK;
    }
    i++;
  }
  return FLUID_FAILED;
}

void fluid_synth_set_reverb(fluid_synth_t* synth, double roomsize, double damping,
                            double width, double level)
{
  /*   fluid_mutex_lock(synth->busy); /\* Don't interfere with the audio thread *\/ */
  /*   fluid_mutex_unlock(synth->busy); */

  fluid_revmodel_setroomsize(synth->reverb, roomsize);
  fluid_revmodel_setdamp(synth->reverb, damping);
  fluid_revmodel_setwidth(synth->reverb, width);
  fluid_revmodel_setlevel(synth->reverb, level);
}

void fluid_synth_set_chorus(fluid_synth_t* synth, int nr, double level,
                            double speed, double depth_ms, int type)
{
  /*   fluid_mutex_lock(synth->busy); /\* Don't interfere with the audio thread *\/ */
  /*   fluid_mutex_unlock(synth->busy); */

  fluid_chorus_set_nr(synth->chorus, nr);
  fluid_chorus_set_level(synth->chorus, (fluid_real_t)level);
  fluid_chorus_set_speed_Hz(synth->chorus, (fluid_real_t)speed);
  fluid_chorus_set_depth_ms(synth->chorus, (fluid_real_t)depth_ms);
  fluid_chorus_set_type(synth->chorus, type);
  fluid_chorus_update(synth->chorus);
}

int fluid_synth_write_float(fluid_synth_t* synth, int len,
                        void* lout, int loff, int lincr,
                        void* rout, int roff, int rincr)
{
  int i, j, k, l;
  float* left_out = (float*) lout;
  float* right_out = (float*) rout;
  fluid_buf_t* left_in = synth->left_buf[0];
  fluid_buf_t* right_in = synth->right_buf[0];
  double time = fluid_utime();

  /* make sure we're playing */
  if (synth->state != FLUID_SYNTH_PLAYING) {
    return 0;
  }

  l = synth->cur;

  for (i = 0, j = loff, k = roff; i < len; i++, l++, j += lincr, k += rincr) {
    /* fill up the buffers as needed */
    if (l == FLUID_BUFSIZE) {
      fluid_synth_one_block(synth, 0);
      l = 0;
    }

    left_out[j] = (float) FLUID_BUF_FLOAT(left_in[l]);
    right_out[k] = (float) FLUID_BUF_FLOAT(right_in[l]);
  }

  synth->cur = l;

  time = fluid_utime() - time;
  synth->cpu_load = 0.5 * (synth->cpu_load +
                           time * synth->sample_rate / len / 10000.0);
  /*   printf("CPU: %.2f\n", synth->cpu_load); */

  return 0;
}

int fluid_synth_write_s16(fluid_synth_t* synth, int len,
                      void* lout, int loff, int lincr,
                      void* rout, int roff, int rincr)
{
  int i, j, k, l;
  int16_t* left_out = (int16_t*) lout;
  int16_t* right_out = (int16_t*) rout;
  fluid_buf_t* left_in = synth->left_buf[0];
  fluid_buf_t* right_in = synth->right_buf[0];
  fluid_buf_t left_sample;
  fluid_buf_t right_sample;

  /* make sure we're playing */
  if (synth->state != FLUID_SYNTH_PLAYING) {
    return 0;
  }

  l = synth->cur;

  for (i = 0, j = loff, k = roff; i < len; i++, l++, j += lincr, k += rincr) {

    /* fill up the buffers as needed */
    if (l == FLUID_BUFSIZE) {
      fluid_synth_one_block(synth, 0);
      l = 0;
    }

    left_sample = left_in[l];
    right_sample = right_in[l];

    /* digital clipping */
    /*    if(left_sample > 1.0f) left_sample = 1.0f;
        if(left_sample < -1.0f) left_sample = -1.0f;
        if(right_sample > 1.0f) right_sample = 1.0f;
        if(right_sample < -1.0f) right_sample = -1.0f;
    */
    left_sample =  (int16_t) FLUID_BUF_SAT(FLUID_BUF_S16(left_sample));
    right_sample =  (int16_t) FLUID_BUF_SAT(FLUID_BUF_S16(right_sample));

    left_out[j] = (left_sample);
    right_out[k] = (right_sample);
  }

  synth->cur = l;

  return 0;
}

int fluid_synth_write_s32(fluid_synth_t* synth, int len,
                      void* lout, int loff, int lincr,
                      void* rout, int roff, int rincr)
{
  int i, j, k, l;
  int32_t* left_out = (int32_t*) lout;
  int32_t* right_out = (int32_t*) rout;
  fluid_buf_t* left_in = synth->left_buf[0];
  fluid_buf_t* right_in = synth->right_buf[0];
  fluid_buf_t left_sample;
  fluid_buf_t right_sample;

  /* make sure we're playing */
  if (synth->state != FLUID_SYNTH_PLAYING) {
    return 0;
  }

  l = synth->cur;

  for (i = 0, j = loff, k = roff; i < len; i++, l++, j += lincr, k += rincr) {

    /* fill up the buffers as needed */
    if (l == FLUID_BUFSIZE) {
      fluid_synth_one_block(synth, 0);
      l = 0;
    }

    left_sample = left_in[l];
    right_sample = right_in[l];

    /* digital clipping */
    /*    if(left_sample > 1.0f) left_sample = 1.0f;
        if(left_sample < -1.0f) left_sample = -1.0f;
        if(right_sample > 1.0f) right_sample = 1.0f;
        if(right_sample < -1.0f) right_sample = -1.0f;
    */
    left_sample =  (left_sample * 2147483646.0f);
    right_sample =  (right_sample * 2147483646.0f);

    left_out[j] = (int32_t) left_sample;
    right_out[k] = (int32_t) right_sample;
  }

  synth->cur = l;

  return 0;
}

int fluid_synth_one_block(fluid_synth_t* synth, int do_not_mix_fx_to_out)
{
  int i, auchan;
  fluid_voice_t* voice;
  fluid_buf_t* left_buf;
  fluid_buf_t* right_buf;
  fluid_buf_t* reverb_buf;
  fluid_buf_t* chorus_buf;
  int byte_size = FLUID_BUFSIZE * sizeof(fluid_buf_t);

  /* clean the audio buffers */
  for (i = 0; i < synth->nbuf; i++) {
    FLUID_MEMSET(synth->left_buf[i], 0, byte_size);
    FLUID_MEMSET(synth->right_buf[i], 0, byte_size);
  }

  for (i = 0; i < synth->effects_channels; i++) {
    FLUID_MEMSET(synth->fx_left_buf[i], 0, byte_size);
    FLUID_MEMSET(synth->fx_right_buf[i], 0, byte_size);
  }

  /* Set up the reverb / chorus buffers only, when the effect is
   * enabled on synth level.  Nonexisting buffers are detected in the
   * DSP loop. Not sending the reverb / chorus signal saves some time
   * in that case. */
  reverb_buf = synth->with_reverb ? synth->fx_left_buf[0] : NULL;
  chorus_buf = synth->with_chorus ? synth->fx_left_buf[1] : NULL;

//  reverb_buf = synth->fx_left_buf[0];
//  chorus_buf = synth->fx_left_buf[1];

  /* call all playing synthesis processes */
  for (i = 0; i < synth->polyphony; i++) {
    voice = synth->voice[i];

    if (_PLAYING(voice)) {
      /* The output associated with a MIDI channel is wrapped around
       * using the number of audio groups as modulo divider.  This is
       * typically the number of output channels on the 'sound card',
       * as long as the LADSPA Fx unit is not used. In case of LADSPA
       * unit, think of it as subgroups on a mixer.
       *
       * For example: Assume that the number of groups is set to 2.
       * Then MIDI channel 1, 3, 5, 7 etc. go to output 1, channels 2,
       * 4, 6, 8 etc to output 2.  Or assume 3 groups: Then MIDI
       * channels 1, 4, 7, 10 etc go to output 1; 2, 5, 8, 11 etc to
       * output 2, 3, 6, 9, 12 etc to output 3.
       */
      auchan = fluid_channel_get_num(fluid_voice_get_channel(voice));
      auchan %= synth->audio_groups;
      left_buf = synth->left_buf[auchan];
      right_buf = synth->right_buf[auchan];

      fluid_voice_write(voice, left_buf, right_buf, reverb_buf, chorus_buf);
    }
  }

  /* if multi channel output, don't mix the output of the chorus and
     reverb in the final output. The effects outputs are send
     separately. */

  /* send to reverb */
  if (synth->with_reverb) {
    if (reverb_buf != NULL) {
      fluid_revmodel_processmix(synth->reverb, reverb_buf,
                                synth->left_buf[0], synth->right_buf[0]);
    }
  }

  /* send to chorus */
  if (synth->with_chorus) {

    if (chorus_buf != NULL) {
      fluid_chorus_processmix(synth->chorus, chorus_buf,
                              synth->left_buf[0], synth->right_buf[0]);
    }
  }

  synth->ticks += FLUID_BUFSIZE;

  return 0;
}

/*
 * fluid_synth_free_voice_by_kill
 *
 * selects a voice for killing. the selection algorithm is a refinement
 * of the algorithm previously in fluid_synth_alloc_voice.
 */
fluid_voice_t* fluid_synth_free_voice_by_kill(fluid_synth_t* synth)
{
  int i;
  fluid_real_t best_prio = 100.;
  fluid_real_t this_voice_prio;
  fluid_voice_t* voice;
  int best_voice_index = -1;

  /*   fluid_mutex_lock(synth->busy); /\* Don't interfere with the audio thread *\/ */
  /*   fluid_mutex_unlock(synth->busy); */

  for (i = 0; i < synth->polyphony; i++) {

    voice = synth->voice[i];

    /* safeguard against an available voice. */
    if (_AVAILABLE(voice)) {
      return voice;
    }

    /* Determine, how 'important' a voice is.
     * Start with an arbitrary number */
    this_voice_prio = 100.;

    /* Is this voice on the drum channel?
     * Then it is very important.
     * Also, forget about the released-note condition:
     * Typically, drum notes are triggered only very briefly, they run most
     * of the time in release phase.
     */
    if (voice->chan == 9) {
      this_voice_prio += 50.;

    } else if (_RELEASED(voice)) {
      /* The key for this voice has been released. Consider it much less important
       * than a voice, which is still held.
       */
      this_voice_prio -= 20.;
    }

    if (_SUSTAINED(voice)) {
      /* The sustain pedal is held down on this channel.
       * Consider it less important than non-sustained channels.
       * This decision is somehow subjective. But usually the sustain pedal
       * is used to play 'more-voices-than-fingers', so it shouldn't hurt
       * if we kill one voice.
       */
      this_voice_prio -= 10.;
    }

    /* We are not enthusiastic about releasing voices, which have just been started.
     * Otherwise hitting a chord may result in killing notes belonging to that very same
     * chord.
     * So subtract the age of the voice from the priority - an older voice is just a little
     * bit less important than a younger voice.
     * This is a number between roughly 0 and 100.*/
//     printf("AGE %d\n",(synth->noteid - fluid_voice_get_id(voice)));
    this_voice_prio -= (synth->noteid - fluid_voice_get_id(voice));

    /* take a rough estimate of loudness into account. Louder voices are more important. */
    if (voice->volenv_section != FLUID_VOICE_ENVATTACK) {
      this_voice_prio += voice->volenv_val * 10.;
    }

    /* check if this voice has less priority than the previous candidate. */
    if (this_voice_prio < best_prio) {
      best_voice_index = i,
      best_prio = this_voice_prio;
    }
  }

  if (best_voice_index < 0) {
    return NULL;
  }

//  printf("KILL VOICE %d/%f\n",best_voice_index,best_prio);
  voice = synth->voice[best_voice_index];

  fluid_voice_off(voice);
  // kill instead of release
//        fluid_voice_kill_excl(voice);

  return voice;
}

/*
 * fluid_synth_alloc_voice
 */
fluid_voice_t* fluid_synth_alloc_voice(fluid_synth_t* synth, fluid_sample_t* sample, int chan, int key, int vel)
{
  int i, k;
  fluid_voice_t* voice = NULL;
  fluid_channel_t* channel = NULL;

  /*   fluid_mutex_lock(synth->busy); /\* Don't interfere with the audio thread *\/ */
  /*   fluid_mutex_unlock(synth->busy); */

  /* check if there's an available synthesis process */
  for (i = 0; i < synth->polyphony; i++) {
    if (_AVAILABLE(synth->voice[i])) {
      voice = synth->voice[i];
      break;
    }
  }

  /* No success yet? Then stop a running voice. */
  if (voice == NULL) {
    voice = fluid_synth_free_voice_by_kill(synth);
  }

  if (voice == NULL) {
    FLUID_LOG(FLUID_WARN, "Failed to allocate a synthesis process. (chan=%d,key=%d)", chan, key);
    return NULL;
  }

  if (synth->verbose) {
    k = 0;
    for (i = 0; i < synth->polyphony; i++) {
      if (!_AVAILABLE(synth->voice[i])) {
        k++;
      }
    }

    FLUID_LOG(FLUID_INFO, "noteon\t%d\t%d\t%d\t%05d\t%.3f\t%.3f\t%.3f\t%d",
              chan, key, vel, synth->storeid,
              (float) synth->ticks / 44100.0f,
              (fluid_curtime() - synth->start) / 1000.0f,
              0.0f,
              k);
  }

  if (chan >= 0) {
    channel = synth->channel[chan];
  }

  if (fluid_voice_init(voice, sample, channel, key, vel,
                       synth->storeid, synth->ticks, synth->gain) != FLUID_OK) {
    FLUID_LOG(FLUID_WARN, "Failed to initialize voice");
    return NULL;
  }

  /* add the default modulators to the synthesis process. */
  fluid_voice_add_mod(voice, &default_vel2att_mod, FLUID_VOICE_DEFAULT);    /* SF2.01 $8.4.1  */
  fluid_voice_add_mod(voice, &default_vel2filter_mod, FLUID_VOICE_DEFAULT); /* SF2.01 $8.4.2  */
  fluid_voice_add_mod(voice, &default_at2viblfo_mod, FLUID_VOICE_DEFAULT);  /* SF2.01 $8.4.3  */
  fluid_voice_add_mod(voice, &default_mod2viblfo_mod, FLUID_VOICE_DEFAULT); /* SF2.01 $8.4.4  */
  fluid_voice_add_mod(voice, &default_att_mod, FLUID_VOICE_DEFAULT);        /* SF2.01 $8.4.5  */
  fluid_voice_add_mod(voice, &default_pan_mod, FLUID_VOICE_DEFAULT);        /* SF2.01 $8.4.6  */
  fluid_voice_add_mod(voice, &default_expr_mod, FLUID_VOICE_DEFAULT);       /* SF2.01 $8.4.7  */
  fluid_voice_add_mod(voice, &default_reverb_mod, FLUID_VOICE_DEFAULT);     /* SF2.01 $8.4.8  */
  fluid_voice_add_mod(voice, &default_chorus_mod, FLUID_VOICE_DEFAULT);     /* SF2.01 $8.4.9  */
  fluid_voice_add_mod(voice, &default_pitch_bend_mod, FLUID_VOICE_DEFAULT); /* SF2.01 $8.4.10 */

  return voice;
}

/*
 * fluid_synth_kill_by_exclusive_class
 */
void fluid_synth_kill_by_exclusive_class(fluid_synth_t* synth, fluid_voice_t* new_voice)
{
  /** Kill all voices on a given channel, which belong into
      excl_class.  This function is called by a SoundFont's preset in
      response to a noteon event.  If one noteon event results in
      several voice processes (stereo samples), ignore_ID must name
      the voice ID of the first generated voice (so that it is not
      stopped). The first voice uses ignore_ID=-1, which will
      terminate all voices on a channel belonging into the exclusive
      class excl_class.
  */

  int i;
  int excl_class = _GEN(new_voice, GEN_EXCLUSIVECLASS);

  /* Check if the voice belongs to an exclusive class. In that case,
     previous notes from the same class are released. */

  /* Excl. class 0: No exclusive class */
  if (excl_class == 0) {
    return;
  }

  //  FLUID_LOG(FLUID_INFO, "Voice belongs to exclusive class (class=%d, ignore_id=%d)", excl_class, ignore_ID);

  /* Kill all notes on the same channel with the same exclusive class */

  for (i = 0; i < synth->polyphony; i++) {
    fluid_voice_t* existing_voice = synth->voice[i];

    /* Existing voice does not play? Leave it alone. */
    if (!_PLAYING(existing_voice)) {
      continue;
    }

    /* An exclusive class is valid for a whole channel (or preset).
     * Is the voice on a different channel? Leave it alone. */
    if (existing_voice->chan != new_voice->chan) {
      continue;
    }

    /* Existing voice has a different (or no) exclusive class? Leave it alone. */
    if ((int)_GEN(existing_voice, GEN_EXCLUSIVECLASS) != excl_class) {
      continue;
    }

    /* Existing voice is a voice process belonging to this noteon
     * event (for example: stereo sample)?  Leave it alone. */
    if (fluid_voice_get_id(existing_voice) == fluid_voice_get_id(new_voice)) {
      continue;
    }

    //    FLUID_LOG(FLUID_INFO, "Releasing previous voice of exclusive class (class=%d, id=%d)",
    //     (int)_GEN(existing_voice, GEN_EXCLUSIVECLASS), (int)fluid_voice_get_id(existing_voice));

    fluid_voice_kill_excl(existing_voice);
  };
};

/*
 * fluid_synth_start_voice
 */
void fluid_synth_start_voice(fluid_synth_t* synth, fluid_voice_t* voice)
{
  /*   fluid_mutex_lock(synth->busy); /\* Don't interfere with the audio thread *\/ */
  /*   fluid_mutex_unlock(synth->busy); */

  /* Find the exclusive class of this voice. If set, kill all voices
   * that match the exclusive class and are younger than the first
   * voice process created by this noteon event. */
  fluid_synth_kill_by_exclusive_class(synth, voice);

  /* Start the new voice */

  fluid_voice_start(voice);
}

/*
 * fluid_synth_add_sfloader
 */
void fluid_synth_add_sfloader(fluid_synth_t* synth, fluid_sfloader_t* loader)
{
  synth->loaders = fluid_list_prepend(synth->loaders, loader);
}


/*
 * fluid_synth_sfload
 */
int fluid_synth_sfload(fluid_synth_t* synth, const char* filename, int reset_presets)
{
  fluid_sfont_t* sfont;
  fluid_list_t* list;
  fluid_sfloader_t* loader;

  if (filename == NULL) {
    FLUID_LOG(FLUID_ERR, "Invalid filename");
    return FLUID_FAILED;
  }

  for (list = synth->loaders; list; list = fluid_list_next(list)) {
    loader = (fluid_sfloader_t*) fluid_list_get(list);

    sfont = fluid_sfloader_load(loader, filename);

    if (sfont != NULL) {

      sfont->id = ++synth->sfont_id;

      /* insert the sfont as the first one on the list */
      synth->sfont = fluid_list_prepend(synth->sfont, sfont);

      /* reset the presets for all channels */
      if (reset_presets) {
        fluid_synth_program_reset(synth);
      }

      return (int) sfont->id;
    }
  }

  FLUID_LOG(FLUID_ERR, "Failed to load SoundFont \"%s\"", filename);
  return -1;
}

/*
 * fluid_synth_sfunload_callback
 */
static int fluid_synth_sfunload_callback(void* data, unsigned int msec)
{
  fluid_sfont_t* sfont = (fluid_sfont_t*) data;
  int r = delete_fluid_sfont(sfont);
  if (r == 0) {
    FLUID_LOG(FLUID_DBG, "Unloaded SoundFont");
  }
  return r != 0;
}

/*
 * fluid_synth_sfunload
 */
int fluid_synth_sfunload(fluid_synth_t* synth, unsigned int id, int reset_presets)
{
  fluid_sfont_t* sfont = fluid_synth_get_sfont_by_id(synth, id);

  if (!sfont) {
    FLUID_LOG(FLUID_ERR, "No SoundFont with id = %d", id);
    return FLUID_FAILED;
  }

  /* remove the SoundFont from the list */
  synth->sfont = fluid_list_remove(synth->sfont, sfont);

  /* reset the presets for all channels */
  if (reset_presets) {
    fluid_synth_program_reset(synth);
  } else {
    fluid_synth_update_presets(synth);
  }

  if (delete_fluid_sfont(sfont) != 0) {
    /* spin off a timer thread to unload the sfont later */
    new_fluid_timer(100, fluid_synth_sfunload_callback, sfont, 1, 1);
  }

  return FLUID_OK;
}


/*
 * fluid_synth_add_sfont
 */
int fluid_synth_add_sfont(fluid_synth_t* synth, fluid_sfont_t* sfont)
{
  sfont->id = ++synth->sfont_id;

  /* insert the sfont as the first one on the list */
  synth->sfont = fluid_list_prepend(synth->sfont, sfont);

  /* reset the presets for all channels */
  fluid_synth_program_reset(synth);

  return sfont->id;
}


/*
 * fluid_synth_remove_sfont
 */
void fluid_synth_remove_sfont(fluid_synth_t* synth, fluid_sfont_t* sfont)
{
  int sfont_id = fluid_sfont_get_id(sfont);

  synth->sfont = fluid_list_remove(synth->sfont, sfont);

  /* remove a possible bank offset */
  fluid_synth_remove_bank_offset(synth, sfont_id);

  /* reset the presets for all channels */
  fluid_synth_program_reset(synth);
}

/* fluid_synth_sfcount
 *
 * Returns the number of loaded SoundFonts
 */
int
fluid_synth_sfcount(fluid_synth_t* synth)
{
  return fluid_list_size(synth->sfont);
}

/* fluid_synth_get_sfont
 *
 * Returns SoundFont num
 */
fluid_sfont_t*
fluid_synth_get_sfont(fluid_synth_t* synth, unsigned int num)
{
  return (fluid_sfont_t*) fluid_list_get(fluid_list_nth(synth->sfont, num));
}

/* fluid_synth_get_sfont_by_id
 *
 */
fluid_sfont_t* fluid_synth_get_sfont_by_id(fluid_synth_t* synth, unsigned int id)
{
  fluid_list_t* list = synth->sfont;
  fluid_sfont_t* sfont;

  while (list) {
    sfont = (fluid_sfont_t*) fluid_list_get(list);
    if (fluid_sfont_get_id(sfont) == id) {
      return sfont;
    }
    list = fluid_list_next(list);
  }
  return NULL;
}

/* fluid_synth_get_sfont_by_name
 *
 */
fluid_sfont_t* fluid_synth_get_sfont_by_name(fluid_synth_t* synth, char *name)
{
  fluid_list_t* list = synth->sfont;
  fluid_sfont_t* sfont;

  while (list) {
    sfont = (fluid_sfont_t*) fluid_list_get(list);
    if (FLUID_STRCMP(fluid_sfont_get_name(sfont), name) == 0) {
      return sfont;
    }
    list = fluid_list_next(list);
  }
  return NULL;
}

/*
 * fluid_synth_get_channel_preset
 */
fluid_preset_t*
fluid_synth_get_channel_preset(fluid_synth_t* synth, int chan)
{
  if ((chan >= 0) && (chan < synth->midi_channels)) {
    return fluid_channel_get_preset(synth->channel[chan]);
  }

  return NULL;
}

/*
 * fluid_synth_get_voicelist
 */
void
fluid_synth_get_voicelist(fluid_synth_t* synth, fluid_voice_t* buf[], int bufsize, int ID)
{
  int i;
  int count = 0;
  for (i = 0; i < synth->polyphony; i++) {
    fluid_voice_t* voice = synth->voice[i];
    if (count >= bufsize) {
      return;
    }

    if (_PLAYING(voice) && ((int)voice->id == ID || ID < 0)) {
      buf[count++] = voice;
    }
  }
  if (count >= bufsize) {
    return;
  }
  buf[count++] = NULL;
}

/* Purpose:
 * Turns on / off the reverb unit in the synth */
void fluid_synth_set_reverb_on(fluid_synth_t* synth, int on)
{
  synth->with_reverb = on;
}

/* Purpose:
 * Turns on / off the chorus unit in the synth */
void fluid_synth_set_chorus_on(fluid_synth_t* synth, int on)
{
  synth->with_chorus = on;
}

/* Purpose:
 * Reports the current setting of the chorus unit. */
int fluid_synth_get_chorus_nr(fluid_synth_t* synth)
{
  return fluid_chorus_get_nr(synth->chorus);
}

double fluid_synth_get_chorus_level(fluid_synth_t* synth)
{
  return (double)fluid_chorus_get_level(synth->chorus);
}

double fluid_synth_get_chorus_speed_Hz(fluid_synth_t* synth)
{
  return (double)fluid_chorus_get_speed_Hz(synth->chorus);
}

double fluid_synth_get_chorus_depth_ms(fluid_synth_t* synth)
{
  return (double)fluid_chorus_get_depth_ms(synth->chorus);
}

int fluid_synth_get_chorus_type(fluid_synth_t* synth)
{
  return fluid_chorus_get_type(synth->chorus);
}

/* Purpose:
 * Returns the current settings_old of the reverb unit */
double fluid_synth_get_reverb_roomsize(fluid_synth_t* synth)
{
  return (double)fluid_revmodel_getroomsize(synth->reverb);
}

double fluid_synth_get_reverb_damp(fluid_synth_t* synth)
{
  return (double) fluid_revmodel_getdamp(synth->reverb);
}

double fluid_synth_get_reverb_level(fluid_synth_t* synth)
{
  return (double) fluid_revmodel_getlevel(synth->reverb);
}

double fluid_synth_get_reverb_width(fluid_synth_t* synth)
{
  return (double) fluid_revmodel_getwidth(synth->reverb);
}

/* Purpose:
 *
 * If the same note is hit twice on the same channel, then the older
 * voice process is advanced to the release stage.  Using a mechanical
 * MIDI controller, the only way this can happen is when the sustain
 * pedal is held.  In this case the behaviour implemented here is
 * natural for many instruments.  Note: One noteon event can trigger
 * several voice processes, for example a stereo sample.  Don't
 * release those...
 */
void fluid_synth_release_voice_on_same_note(fluid_synth_t* synth, int chan, int key) {
  int i;
  fluid_voice_t* voice;

  /*   fluid_mutex_lock(synth->busy); /\* Don't interfere with the audio thread *\/ */
  /*   fluid_mutex_unlock(synth->busy); */

  for (i = 0; i < synth->polyphony; i++) {
    voice = synth->voice[i];
    if (_PLAYING(voice)
        && (voice->chan == chan)
        && (voice->key == key)
        && (fluid_voice_get_id(voice) != synth->noteid)) {
      fluid_voice_noteoff(voice);
      /* Kill it instead of release to avoid to many voices creation */
//      fluid_voice_kill_excl(voice);
    }
  }

}

/* Purpose:
 * Sets the interpolation method to use on channel chan.
 * If chan is < 0, then set the interpolation method on all channels.
 */
int fluid_synth_set_interp_method(fluid_synth_t* synth, int chan, int interp_method) {
  int i;
  for (i = 0; i < synth->midi_channels; i++) {
    if (synth->channel[i] == NULL) {
      FLUID_LOG(FLUID_ERR, "Channels don't exist (yet)!");
      return FLUID_FAILED;
    };
    if (chan < 0 || fluid_channel_get_num(synth->channel[i]) == chan) {
      fluid_channel_set_interp_method(synth->channel[i], interp_method);
    };
  };
  return FLUID_OK;
};

/* Purpose:
 * Returns the number of allocated midi channels
 */
int
fluid_synth_count_midi_channels(fluid_synth_t* synth)
{
  return synth->midi_channels;
}

/* Purpose:
 * Returns the number of allocated audio channels
 */
int
fluid_synth_count_audio_channels(fluid_synth_t* synth)
{
  return synth->audio_channels;
}

/* Purpose:
 * Returns the number of allocated audio channels
 */
int
fluid_synth_count_audio_groups(fluid_synth_t* synth)
{
  return synth->audio_groups;
}

/* Purpose:
 * Returns the number of allocated effects channels
 */
int
fluid_synth_count_effects_channels(fluid_synth_t* synth)
{
  return synth->effects_channels;
}

double fluid_synth_get_cpu_load(fluid_synth_t* synth)
{
  return synth->cpu_load;
}

static fluid_tuning_t*
fluid_synth_get_tuning(fluid_synth_t* synth, int bank, int prog)
{
  if ((bank < 0) || (bank >= 128)) {
    FLUID_LOG(FLUID_WARN, "Bank number out of range");
    return NULL;
  }
  if ((prog < 0) || (prog >= 128)) {
    FLUID_LOG(FLUID_WARN, "Program number out of range");
    return NULL;
  }
  if ((synth->tuning == NULL) ||
      (synth->tuning[bank] == NULL) ||
      (synth->tuning[bank][prog] == NULL)) {
    FLUID_LOG(FLUID_WARN, "No tuning at bank %d, prog %d", bank, prog);
    return NULL;
  }
  return synth->tuning[bank][prog];
}

static fluid_tuning_t*
fluid_synth_create_tuning(fluid_synth_t* synth, int bank, int prog, char* name)
{
  if ((bank < 0) || (bank >= 128)) {
    FLUID_LOG(FLUID_WARN, "Bank number out of range");
    return NULL;
  }
  if ((prog < 0) || (prog >= 128)) {
    FLUID_LOG(FLUID_WARN, "Program number out of range");
    return NULL;
  }
  if (synth->tuning == NULL) {
    synth->tuning = FLUID_ARRAY(fluid_tuning_t**, 128);
    if (synth->tuning == NULL) {
      FLUID_LOG(FLUID_PANIC, "Out of memory");
      return NULL;
    }
    FLUID_MEMSET(synth->tuning, 0, 128 * sizeof(fluid_tuning_t**));
  }

  if (synth->tuning[bank] == NULL) {
    synth->tuning[bank] = FLUID_ARRAY(fluid_tuning_t*, 128);
    if (synth->tuning[bank] == NULL) {
      FLUID_LOG(FLUID_PANIC, "Out of memory");
      return NULL;
    }
    FLUID_MEMSET(synth->tuning[bank], 0, 128 * sizeof(fluid_tuning_t*));
  }

  if (synth->tuning[bank][prog] == NULL) {
    synth->tuning[bank][prog] = new_fluid_tuning(name, bank, prog);
    if (synth->tuning[bank][prog] == NULL) {
      return NULL;
    }
  }

  if ((fluid_tuning_get_name(synth->tuning[bank][prog]) == NULL)
      || (FLUID_STRCMP(fluid_tuning_get_name(synth->tuning[bank][prog]), name) != 0)) {
    fluid_tuning_set_name(synth->tuning[bank][prog], name);
  }

  return synth->tuning[bank][prog];
}

int fluid_synth_create_key_tuning(fluid_synth_t* synth,
                                  int bank, int prog,
                                  char* name, double* pitch)
{
  fluid_tuning_t* tuning = fluid_synth_create_tuning(synth, bank, prog, name);
  if (tuning == NULL) {
    return FLUID_FAILED;
  }
  if (pitch) {
    fluid_tuning_set_all(tuning, pitch);
  }
  return FLUID_OK;
}


int fluid_synth_create_octave_tuning(fluid_synth_t* synth,
                                     int bank, int prog,
                                     char* name, double* pitch)
{
  fluid_tuning_t* tuning = fluid_synth_create_tuning(synth, bank, prog, name);
  if (tuning == NULL) {
    return FLUID_FAILED;
  }
  fluid_tuning_set_octave(tuning, pitch);
  return FLUID_OK;
}

int fluid_synth_tune_notes(fluid_synth_t* synth, int bank, int prog,
                           int len, int *key, double* pitch, int apply)
{
  fluid_tuning_t* tuning = fluid_synth_get_tuning(synth, bank, prog);
  int i;

  if (tuning == NULL) {
    return FLUID_FAILED;
  }

  for (i = 0; i < len; i++) {
    fluid_tuning_set_pitch(tuning, key[i], pitch[i]);
  }

  return FLUID_OK;
}

int fluid_synth_select_tuning(fluid_synth_t* synth, int chan,
                              int bank, int prog)
{
  fluid_tuning_t* tuning = fluid_synth_get_tuning(synth, bank, prog);

  if (tuning == NULL) {
    return FLUID_FAILED;
  }
  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return FLUID_FAILED;
  }

  fluid_channel_set_tuning(synth->channel[chan], synth->tuning[bank][prog]);

  return FLUID_OK;
}

int fluid_synth_reset_tuning(fluid_synth_t* synth, int chan)
{
  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return FLUID_FAILED;
  }

  fluid_channel_set_tuning(synth->channel[chan], NULL);

  return FLUID_OK;
}

void fluid_synth_tuning_iteration_start(fluid_synth_t* synth)
{
  synth->cur_tuning = NULL;
}

int fluid_synth_tuning_iteration_next(fluid_synth_t* synth, int* bank, int* prog)
{
  int b = 0, p = 0;

  if (synth->tuning == NULL) {
    return 0;
  }

  if (synth->cur_tuning != NULL) {
    /* get the next program number */
    b = fluid_tuning_get_bank(synth->cur_tuning);
    p = 1 + fluid_tuning_get_prog(synth->cur_tuning);
    if (p >= 128) {
      p = 0;
      b++;
    }
  }

  while (b < 128) {
    if (synth->tuning[b] != NULL) {
      while (p < 128) {
        if (synth->tuning[b][p] != NULL) {
          synth->cur_tuning = synth->tuning[b][p];
          *bank = b;
          *prog = p;
          return 1;
        }
        p++;
      }
    }
    p = 0;
    b++;
  }

  return 0;
}


fluid_settings_t* fluid_synth_get_settings(fluid_synth_t* synth)
{
  return synth->settings;
}

#ifndef FLUID_NO_NRPN_EXT

int
fluid_synth_set_gen(fluid_synth_t* synth, int chan, int param, fluid_real_t value)
{
  int i;
  fluid_voice_t* voice;

  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return FLUID_FAILED;
  }

  if ((param < 0) || (param >= GEN_LAST)) {
    FLUID_LOG(FLUID_WARN, "Parameter number out of range");
    return FLUID_FAILED;
  }

  fluid_channel_set_gen(synth->channel[chan], param, value, 0);

  for (i = 0; i < synth->polyphony; i++) {
    voice = synth->voice[i];
    if (voice->chan == chan) {
      fluid_voice_set_param(voice, param, value, 0);
    }
  }

  return FLUID_OK;
}

/** Change the value of a generator. This function allows to control
    all synthesis parameters in real-time. The changes are additive,
    i.e. they add up to the existing parameter value. This function is
    similar to sending an NRPN message to the synthesizer. The
    function accepts a float as the value of the parameter. The
    parameter numbers and ranges are described in the SoundFont 2.01
    specification, paragraph 8.1.3, page 48. See also
    'fluid_gen_type'.

    Using the fluid_synth_set_gen2() function, it is possible to set
    the absolute value of a generator. This is an extension to the
    SoundFont standard. If 'absolute' is non-zero, the value of the
    generator specified in the SoundFont is completely ignored and the
    generator is fixed to the value passed as argument. To undo this
    behavior, you must call fluid_synth_set_gen2 again, with
    'absolute' set to 0 (and possibly 'value' set to zero).

    If 'normalized' is non-zero, the value is supposed to be
    normalized between 0 and 1. Before applying the value, it will be
    scaled and shifted to the range defined in the SoundFont
    specifications.

 */
int
fluid_synth_set_gen2(fluid_synth_t* synth, int chan, int param,
                     float value, int absolute, int normalized)
{
  int i;
  fluid_voice_t* voice;
  float v;

  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return FLUID_FAILED;
  }

  if ((param < 0) || (param >= GEN_LAST)) {
    FLUID_LOG(FLUID_WARN, "Parameter number out of range");
    return FLUID_FAILED;
  }

  v = (normalized) ? fluid_gen_scale(param, value) : value;

  fluid_channel_set_gen(synth->channel[chan], param, v, absolute);

  for (i = 0; i < synth->polyphony; i++) {
    voice = synth->voice[i];
    if (voice->chan == chan) {
      fluid_voice_set_param(voice, param, v, absolute);
    }
  }

  return FLUID_OK;
}
#endif

float fluid_synth_get_gen(fluid_synth_t* synth, int chan, int param)
{
  if ((chan < 0) || (chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return 0.0;
  }

  if ((param < 0) || (param >= GEN_LAST)) {
    FLUID_LOG(FLUID_WARN, "Parameter number out of range");
    return 0.0;
  }

  return fluid_channel_get_gen(synth->channel[chan], param);
}


int fluid_synth_start(fluid_synth_t* synth, unsigned int id, fluid_preset_t* preset,
                      int audio_chan, int midi_chan, int key, int vel)
{
  int r;

  /* check the ranges of the arguments */
  if ((midi_chan < 0) || (midi_chan >= synth->midi_channels)) {
    FLUID_LOG(FLUID_WARN, "Channel out of range");
    return FLUID_FAILED;
  }

  if ((key < 0) || (key >= 128)) {
    FLUID_LOG(FLUID_WARN, "Key out of range");
    return FLUID_FAILED;
  }

  if ((vel <= 0) || (vel >= 128)) {
    FLUID_LOG(FLUID_WARN, "Velocity out of range");
    return FLUID_FAILED;
  }

  fluid_mutex_lock(synth->busy); /* One at a time, please */

  synth->storeid = id;
  r = fluid_preset_noteon(preset, synth, midi_chan, key, vel);

  fluid_mutex_unlock(synth->busy);

  return r;
}

int fluid_synth_stop(fluid_synth_t* synth, unsigned int id)
{
  int i;
  fluid_voice_t* voice;
  int status = FLUID_FAILED;
  int count = 0;

  for (i = 0; i < synth->polyphony; i++) {

    voice = synth->voice[i];

    if (_ON(voice) && (fluid_voice_get_id(voice) == id)) {
      count++;
      fluid_voice_noteoff(voice);
      status = FLUID_OK;
    }
  }

  return status;
}

fluid_bank_offset_t*
fluid_synth_get_bank_offset0(fluid_synth_t* synth, int sfont_id)
{
  fluid_list_t* list = synth->bank_offsets;
  fluid_bank_offset_t* offset;

  while (list) {

    offset = (fluid_bank_offset_t*) fluid_list_get(list);
    if (offset->sfont_id == sfont_id) {
      return offset;
    }

    list = fluid_list_next(list);
  }

  return NULL;
}

int
fluid_synth_set_bank_offset(fluid_synth_t* synth, int sfont_id, int offset)
{
  fluid_bank_offset_t* bank_offset;

  bank_offset = fluid_synth_get_bank_offset0(synth, sfont_id);

  if (bank_offset == NULL) {
    bank_offset = FLUID_NEW(fluid_bank_offset_t);
    if (bank_offset == NULL) {
      return -1;
    }
    bank_offset->sfont_id = sfont_id;
    bank_offset->offset = offset;
    synth->bank_offsets = fluid_list_prepend(synth->bank_offsets, bank_offset);
  } else {
    bank_offset->offset = offset;
  }

  return 0;
}

int
fluid_synth_get_bank_offset(fluid_synth_t* synth, int sfont_id)
{
  fluid_bank_offset_t* bank_offset;

  bank_offset = fluid_synth_get_bank_offset0(synth, sfont_id);
  return (bank_offset == NULL) ? 0 : bank_offset->offset;
}

void
fluid_synth_remove_bank_offset(fluid_synth_t* synth, int sfont_id)
{
  fluid_bank_offset_t* bank_offset;

  bank_offset = fluid_synth_get_bank_offset0(synth, sfont_id);
  if (bank_offset != NULL) {
    synth->bank_offsets = fluid_list_remove(synth->bank_offsets, bank_offset);
  }
}
